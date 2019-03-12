/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "fileio/NetcdfIO.h"

#include "netcdf.h"

#include <iostream>
#include <cmath>
#include <memory>
#include <string>
#include <typeinfo>

#include "distribution/DistributionFactory.h"
#include "oops/parallel/mpi/mpi.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/datetime_f.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"
#include "oops/util/missingValues.h"

////////////////////////////////////////////////////////////////////////
// Implementation of IodaIO for netcdf.
////////////////////////////////////////////////////////////////////////

namespace ioda {

// -----------------------------------------------------------------------------
static const double missingthreshold = 1.0e08;
// -----------------------------------------------------------------------------
/*!
 * \details This constructor will open the netcdf file. If opening in read
 *          mode, the parameters nlocs, nrecs and nvars will be set
 *          by querying the size of dimensions of the same names in the input
 *          file. If opening in write mode, the parameters will be set from the
 *          same named arguements to this constructor.
 *
 * \param[in]  FileName Path to the netcdf file
 * \param[in]  FileMode "r" for read, "w" for overwrite to an existing file
 *                      and "W" for create and write to a new file.
 * \param[in]  Nlocs Number of unique locations in the obs data.
 * \param[in]  Nrecs Number of unique records in the obs data. Records are
 *                   atomic units that will remain intact when obs are
 *                   distributed across muliple process elements. A single
 *                   radiosonde sounding would be an example.
 * \param[in]  Nvars Number of unique varibles in the obs data.
 */

NetcdfIO::NetcdfIO(const std::string & FileName, const std::string & FileMode,
                   const std::size_t & Nlocs, const std::size_t & Nrecs,
                   const std::size_t & Nvars) {
  int retval_;
  std::string ErrorMsg;

  // Set the data members to the file name, file mode and provide a trace message.
  fname_ = FileName;
  fmode_ = FileMode;
  nlocs_ = Nlocs;
  nrecs_ = Nrecs;
  nvars_ = Nvars;
  oops::Log::trace() << __func__ << " fname_: " << fname_ << " fmode_: " << fmode_ << std::endl;

  // Open the file. The fmode_ values that are recognized are:
  //    "r" - read
  //    "w" - write, disallow overriting an existing file
  //    "W" - write, allow overwriting an existing file
  if (fmode_ == "r") {
    retval_ = nc_open(fname_.c_str(), NC_NOWRITE, &ncid_);
  } else if (fmode_ == "w") {
    retval_ = nc_create(fname_.c_str(), NC_NOCLOBBER|NC_NETCDF4, &ncid_);
  } else if (fmode_ == "W") {
    retval_ = nc_create(fname_.c_str(), NC_CLOBBER|NC_NETCDF4, &ncid_);
  } else {
    oops::Log::error() << __func__ << ": Unrecognized FileMode: " << fmode_ << std::endl;
    oops::Log::error() << __func__ << ": Must use one of: 'r', 'w', 'W'" << std::endl;
    ABORT("Unrecognized file mode for NetcdfIO constructor");
  }

  // Abort if open failed
  if (retval_ != NC_NOERR) {
    oops::Log::error() << __func__ << ": Unable to open file '" << fname_
                       << "' in mode: " << fmode_ << std::endl;
    ABORT("Unable to open file");
  }

  // When in read mode, the constructor is responsible for setting
  // the data members nlocs_, nrecs_, nvars_ and grp_var_info_.
  //
  // The files have nlocs, nrecs, nvars.
  //
  // The way to collect the VALID variable names is controlled by developers.
  //

  if (fmode_ == "r") {
    int NcNdims;
    int NcNvars;
    int NcNatts;

    // Find counts of objects in the file
    ErrorMsg = "NetcdfIO::NetcdfIO: Unable to read file object counts";
    CheckNcCall(nc_inq(ncid_, &NcNdims, &NcNvars, &NcNatts, NULL), ErrorMsg);

    // Record the dimension id numbers and sizes in the dim_list_ container.
    // Save nlocs, nrecs and nvars in data members.
    for (std::size_t i = 0; i < NcNdims; i++) {
      char NcName[MAX_NC_NAME];
      std::size_t NcSize;
      ErrorMsg = "NetcdfIO::NetcdfIO: Unable to read dimension number: " + std::to_string(i);
      CheckNcCall(nc_inq_dim(ncid_, i, NcName, &NcSize), ErrorMsg);
      dim_list_.push_back(std::make_tuple(NcName, NcSize));

      if (strcmp(NcName, "nlocs") == 0) {
        nlocs_id_ = i;
        nlocs_ = NcSize;
      } else if (strcmp(NcName, "nrecs") == 0) {
        nrecs_id_ = i;
        nrecs_ = NcSize;
      } else if (strcmp(NcName, "nvars") == 0) {
        nvars_id_ = i;
        nvars_ = NcSize;
      }
    }

    // Walk through the variables and record the group and variable information
    for (std::size_t ivar=0; ivar < NcNvars; ++ivar) {
      // nc variable dimension and type information
      char NcVname[NC_MAX_NAME+1];
      nc_type NcDtype;
      int NcVarId;
      int NcNdims;
      int NcDimIds[NC_MAX_VAR_DIMS];
      ErrorMsg = "NetcdfIO::NetcdfIO: Unable to read information for variable number: " + std::to_string(ivar);
      CheckNcCall(nc_inq_var(ncid_, ivar, NcVname, &NcDtype, &NcNdims, NcDimIds, 0), ErrorMsg);
      CheckNcCall(nc_inq_varid(ncid_, NcVname, &NcVarId), ErrorMsg);

      // nc type name
      char NcDtypeName[NC_MAX_NAME+1];
      std::size_t NcDtypeSize;
      ErrorMsg = "NetcdfIO::NetcdfIO: Unable to look up type name";
      CheckNcCall(nc_inq_type(ncid_, NcDtype, NcDtypeName, &NcDtypeSize), ErrorMsg);

      // Collect the sizes for dimensions from the dim_list_ container.
      std::vector<std::size_t> NcDimSizes;
      std::size_t NcVarSize = 1;
      for (std::size_t j = 0; j < NcNdims; j++) {
          NcDimSizes.push_back(std::get<1>(dim_list_[NcDimIds[j]]));
          NcVarSize *= NcDimSizes[j];
      }

      // Record the variable info in the grp_var_into_ container.
      std::string VarName{NcVname};
      std::string GroupName{"GroupUndefined"};
      std::size_t Spos = VarName.find("@");
      if (Spos != VarName.npos) {
        GroupName = VarName.substr(Spos+1);
        VarName = VarName.substr(0, Spos);
      }
      grp_var_info_[GroupName][VarName].var_id = NcVarId;
      grp_var_info_[GroupName][VarName].dtype = NcDtypeName;
      grp_var_info_[GroupName][VarName].shape = NcDimSizes;
      grp_var_info_[GroupName][VarName].size = NcVarSize;
    }

    for (IodaIO::GroupIter igrp = grp_var_info_.begin();
                           igrp != grp_var_info_.end(); igrp++) {
      std::string GroupName = igrp->first;
      for (IodaIO::VarIter ivar = grp_var_info_[GroupName].begin();
                           ivar != grp_var_info_[GroupName].end(); ivar++) {
        std::string VarName = ivar->first;
        std::cout << "DEBUG: grp_var_info_: " << GroupName << ", " << VarName << ", "
                  << grp_var_info_[GroupName][VarName].var_id << ", "
                  << grp_var_info_[GroupName][VarName].dtype << ", "
                  << grp_var_info_[GroupName][VarName].shape << ", "
                  << grp_var_info_[GroupName][VarName].size << std::endl;
      }
    }

  }

  // When in write mode, create dimensions in the output file based on
  // nlocs_, nrecs_, nvars_.
  if ((fmode_ == "W") || (fmode_ == "w")) {
    ErrorMsg = "NetcdfIO::NetcdfIO: Unable to create dimension: nlocs";
    CheckNcCall(nc_def_dim(ncid_, "nlocs", Nlocs, &nlocs_id_), ErrorMsg);
    ErrorMsg = "NetcdfIO::NetcdfIO: Unable to create dimension: nrecs";
    CheckNcCall(nc_def_dim(ncid_, "nrecs", Nrecs, &nrecs_id_), ErrorMsg);
    ErrorMsg = "NetcdfIO::NetcdfIO: Unable to create dimension: nvars";
    CheckNcCall(nc_def_dim(ncid_, "nvars", Nvars, &nvars_id_), ErrorMsg);
    }
  }

// -----------------------------------------------------------------------------

NetcdfIO::~NetcdfIO() {
  oops::Log::trace() << __func__ << " fname_: " << fname_ << std::endl;

  nc_close(ncid_);
  }

// -----------------------------------------------------------------------------
/*!
 * \brief Read data from netcdf file to memory
 *
 * \details The three ReadVar methods are the same with the exception of the
 *          datatype that is being read (integer, float, char). It is the
 *          caller's responsibility to allocate memory to hold the data being
 *          read. The caller then passes a pointer to that memory for the VarData
 *          argument.
 *
 * \param[in]  GroupName Name of ObsSpace group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  VarName Name of ObsSpace variable
 * \param[in]  VarSize Number of elements that have been allocated for VarData
 * \param[out] VarData Pointer to memory that will receive the file data
 */

void NetcdfIO::ReadVar(const std::string & GroupName, const std::string & VarName,
                       const int & VarSize, int * VarData) {
  ReadVar_helper<int>(GroupName, VarName, VarSize, VarData);
}

void NetcdfIO::ReadVar(const std::string & GroupName, const std::string & VarName,
                       const int & VarSize, float * VarData) {
  ReadVar_helper<float>(GroupName, VarName, VarSize, VarData);
}

void NetcdfIO::ReadVar(const std::string & GroupName, const std::string & VarName,
                       const int & VarSize, char * VarData) {
  ReadVar_helper<char>(GroupName, VarName, VarSize, VarData);
}

template <typename DataType>
void NetcdfIO::ReadVar_helper(const std::string & GroupName, const std::string & VarName,
                              const int & VarSize, DataType * VarData) {
  std::string NcVarName = GetNcVarName(GroupName, VarName);
  std::string NcVarType = grp_var_info_[GroupName][VarName].dtype;

  // Read in the variable values. The netcdf interface has a generic get var (nc_get_var)
  // routine, but calling this interface causes a crash to occur. There also exist type
  // specific get var routines (nc_get_var_int, nc_get_var_float, etc.). These work okay,
  // but the compile fails when VarData comes in as a int pointer, and you VarData to the
  // nc_get_var_float routine which expects a float pointer. This situation is resolved
  // by putting in casts to the expected pointer type.
  //
  // Normally, forcing the pointer to be the expected type is a dangerous practice,
  // but in this case it is controlled well. The if statment below ensures that the
  // casting is appropriate for the given type that is being read. For example, checking
  // that VarType is "int" occurs only in the case where VarData is an int *.
  //
  // Note in the if statment below, nc_get_var_float handles the conversion from
  // double to float in the case where the netcdf variable is type double.
  //
  const std::type_info & VarType = typeid(DataType); // this matches type of VarData
  int NcVarId = grp_var_info_[GroupName][VarName].var_id;
  std::string ErrorMsg = "NetcdfIO::ReadVar: Unable to read netcdf variable: " + NcVarName;
  if (VarType == typeid(int)) {
    CheckNcCall(nc_get_var_int(ncid_, NcVarId, (int *) VarData), ErrorMsg);
  } else if (VarType == typeid(float)) {
    CheckNcCall(nc_get_var_float(ncid_, NcVarId, (float *) VarData), ErrorMsg);
  } else if (VarType == typeid(char)) {
    CheckNcCall(nc_get_var_text(ncid_, NcVarId, (char *) VarData), ErrorMsg);
  } else {
    oops::Log::warning() <<  "NetcdfIO::ReadVar: Unable to read dataset: "
                         << " VarName: " << NcVarName << " with NetCDF type :"
                         << NcVarType << std::endl;
  }

  // Add in the missing data marks.
  if ((VarType == typeid(int)) or (VarType == typeid(float))) {
    const DataType missing_value = util::missingValue(missing_value);
    for (std::size_t i = 0; i < VarSize; i++) {
      // For now use a large number as an indicator of a missing value. This is not
      // as safe as it should be. In the future, use the netcdf default fill value
      // as the missing value indicator.
      if (abs(VarData[i]) > missingthreshold) {
        VarData[i] = missing_value;
      }
    }
  }
}

// -----------------------------------------------------------------------------
/*!
 * \brief Write data from memory to netcdf file
 *
 * \details The three WriteVar methods are the same with the exception of the
 *          datatype that is being written (integer, float, char). It is the
 *          caller's responsibility to allocate and assign memory to the data
 *          that are to be written. The caller then passes a pointer to that
 *          memory for the VarData argument.
 *
 * \param[in]  GroupName Name of ObsSpace group (ObsValue, ObsError, MetaData, etc.)
 * \param[in]  VarName Name of ObsSpace variable
 * \param[in]  VarSize Number of elements that have been allocated for VarData
 * \param[in]  VarData Pointer to memory that will be written into the file
 */

void NetcdfIO::WriteVar(const std::string & GroupName, const std::string & VarName,
                        const int & VarSize, int * VarData) {
  WriteVar_helper<int>(GroupName, VarName, VarSize, VarData);
}

void NetcdfIO::WriteVar(const std::string & GroupName, const std::string & VarName,
                        const int & VarSize, float * VarData) {
  WriteVar_helper<float>(GroupName, VarName, VarSize, VarData);
}

void NetcdfIO::WriteVar(const std::string & GroupName, const std::string & VarName,
                        const int & VarSize, char * VarData) {
  WriteVar_helper<char>(GroupName, VarName, VarSize, VarData);
}

template <typename DataType>
void NetcdfIO::WriteVar_helper(const std::string & GroupName, const std::string & VarName,
                               const int & VarSize, DataType * VarData) {
  std::string ErrorMsg;
  const std::type_info & VarType = typeid(DataType);
  nc_type NcVarType;
  int NcVarId;
  std::string NcVarName;

  if (GroupName.compare("GroupUndefined") != 0) {
    NcVarName = VarName + "@" + GroupName;
  } else {
    NcVarName = VarName;
  }

  // Limit types to int, float and double for now
  if (VarType == typeid(int)) {
    NcVarType = NC_INT;
  } else if (VarType == typeid(float)) {
    NcVarType = NC_FLOAT;
  } else if (VarType == typeid(double)) {
    NcVarType = NC_DOUBLE;
  } else {
    oops::Log::warning() <<  "NetcdfIO::WriteVar: Unable to write dataset: "
                         << " VarName: " << NcVarName << " with NetCDF type :"
                         << VarType.name() << std::endl;
  }

  // If var doesn't exist in the file, then create it
  if (nc_inq_varid(ncid_, NcVarName.c_str(), &NcVarId) != NC_NOERR) {
    ErrorMsg = "NetcdfIO::WriteVar: Unable to create variable dataset: " + NcVarName;
    CheckNcCall(nc_def_var(ncid_, NcVarName.c_str(), NcVarType, 1, &nlocs_id_, &NcVarId),
                ErrorMsg);
  }

  // Write the data into the file according to type
  ErrorMsg = "NetcdfIO::WriteVar: Unable to write dataset: " + NcVarName;
  if (NcVarType == NC_INT) {
    std::unique_ptr<int[]> iData{new int[nlocs()]};
    for (std::size_t ii = 0; ii < nlocs(); ++ii)
      iData.get()[ii] = boost::any_cast<int>(VarData[ii]);
    CheckNcCall(nc_put_var_int(ncid_, NcVarId, iData.get()), ErrorMsg);
  } else if (NcVarType == NC_FLOAT) {
    std::unique_ptr<float[]> fData{new float[nlocs()]};
    for (std::size_t ii = 0; ii < nlocs(); ++ii)
      fData.get()[ii] = boost::any_cast<float>(VarData[ii]);
    CheckNcCall(nc_put_var_float(ncid_, NcVarId, fData.get()), ErrorMsg);
  } else if (NcVarType == NC_DOUBLE) {
    std::unique_ptr<double[]> dData{new double[nlocs()]};
    for (std::size_t ii = 0; ii < nlocs(); ++ii)
      dData.get()[ii] = boost::any_cast<double>(VarData[ii]);
    CheckNcCall(nc_put_var_double(ncid_, NcVarId, dData.get()), ErrorMsg);
  }
}

// -----------------------------------------------------------------------------
/*!
 * \brief Read and format the date, time values
 *
 * \details This method will read in the date and time information (timestamp)
 *          from the netcdf file, and convert them into a convenient format for
 *          usage by the JEDI system. Currently, the netcdf files contain an
 *          attribute called "date_time" that holds the analysis time for
 *          the obs data in the format yyyymmddhh. For example April 15, 2018
 *          at 00Z is recorded as 2018041500. The netcdf file also contains
 *          a time variable (float) which is the offset from the date_time
 *          value in hours. This method will convert the date time information to two
 *          integer vectors. The first is the date (yyyymmdd) and the second
 *          is the time (hhmmss). With the above date_time example combined
 *          with a time value of -3.5 (hours), the resulting date and time entries
 *          in the output vectors will be date = 20180414 and time = 233000.
 *
 *          Eventually, the yyyymmdd and hhmmss values can be recorded in the
 *          netcdf file as thier own datasets and this method could be removed.
 *
 * \param[out] VarDate Date portion of the timestamp values (yyyymmdd)
 * \param[out] VarTime Time portion of the timestamp values (hhmmss)
 */

void NetcdfIO::ReadDateTime(uint64_t * VarDate, int * VarTime) {
  int Year;
  int Month;
  int Day;
  int Hour;
  int Minute;
  int Second;

  std::string ErrorMsg;

  oops::Log::trace() << __func__ << std::endl;

  // Read in the date_time attribute which is in the form: yyyymmddhh
  // Convert the date_time to a Datetime object.
  int dtvals_;
  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to read attribute: date_time";
  CheckNcCall(nc_get_att_int(ncid_, NC_GLOBAL, "date_time", &dtvals_), ErrorMsg);

  util::DateTime refdt_;
  datetime_setints_f(&refdt_, dtvals_/100, dtvals_%100 * 3600);

  // Read in the time variable and convert to a Duration object. Time is an
  // offset from the date_time attribute. This fits in nicely with a Duration
  // object.
  // Look for "time" and "Obs_Time" for the time variable.
  int nc_varid_;
  if (nc_inq_varid(ncid_, "time", &nc_varid_) != NC_NOERR) {
    ErrorMsg = "NetcdfIO::ReadDateTime: Unable to find time variable: time OR Obs_Time";
    CheckNcCall(nc_inq_varid(ncid_, "time@MetaData", &nc_varid_), ErrorMsg);
  }

  int dimid_;
  std::unique_ptr<float[]> OffsetTime;
  std::size_t vsize_;

  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to find dimension of time variable";
  CheckNcCall(nc_inq_vardimid(ncid_, nc_varid_, &dimid_), ErrorMsg);

  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to find size of dimension of time variable";
  CheckNcCall(nc_inq_dimlen(ncid_, dimid_, &vsize_), ErrorMsg);

  OffsetTime.reset(new float[vsize_]);
  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to read time variable: ";
  CheckNcCall(nc_get_var_float(ncid_, nc_varid_, OffsetTime.get()), ErrorMsg);

  // Combine the refdate with the offset time, and convert to yyyymmdd and
  // hhmmss values.
  std::unique_ptr<util::DateTime> dt_(new util::DateTime[vsize_]);
  for (std::size_t i = 0; i < vsize_; ++i) {
    dt_.get()[i] = refdt_ + util::Duration(static_cast<int>(OffsetTime.get()[i] * 3600));
    dt_.get()[i].toYYYYMMDDhhmmss(Year, Month, Day, Hour, Minute, Second);

    VarDate[i] = Year*10000 + Month*100 + Day;
    VarTime[i] = Hour*10000 + Minute*100 + Second;
    }
  }

// -----------------------------------------------------------------------------

void NetcdfIO::ReadDateTime(util::DateTime VarDateTime[]) {
  std::string ErrorMsg;

  oops::Log::trace() << __func__ << std::endl;

  // Read in the date_time attribute which is in the form: yyyymmddhh
  // Convert the date_time to a Datetime object.
  int dtvals_;
  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to read attribute: date_time";
  CheckNcCall(nc_get_att_int(ncid_, NC_GLOBAL, "date_time", &dtvals_), ErrorMsg);

  util::DateTime refdt_;
  datetime_setints_f(&refdt_, dtvals_/100, dtvals_%100 * 3600);

  // Read in the time variable and convert to a Duration object. Time is an
  // offset from the date_time attribute. This fits in nicely with a Duration
  // object.
  // Look for "time" and "Obs_Time" for the time variable.
  int nc_varid_;
  if (nc_inq_varid(ncid_, "time", &nc_varid_) != NC_NOERR) {
    ErrorMsg = "NetcdfIO::ReadDateTime: Unable to find time variable: time OR Obs_Time";
    CheckNcCall(nc_inq_varid(ncid_, "time@MetaData", &nc_varid_), ErrorMsg);
  }

  int dimid_;
  std::unique_ptr<float[]> OffsetTime;
  std::size_t vsize_;

  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to find dimension of time variable";
  CheckNcCall(nc_inq_vardimid(ncid_, nc_varid_, &dimid_), ErrorMsg);

  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to find size of dimension of time variable";
  CheckNcCall(nc_inq_dimlen(ncid_, dimid_, &vsize_), ErrorMsg);

  OffsetTime.reset(new float[vsize_]);
  ErrorMsg = "NetcdfIO::ReadDateTime: Unable to read time variable: ";
  CheckNcCall(nc_get_var_float(ncid_, nc_varid_, OffsetTime.get()), ErrorMsg);

  // Combine the refdate with the offset time, and convert to yyyymmdd and
  // hhmmss values.
  for (std::size_t i = 0; i < vsize_; ++i) {
    VarDateTime[i] = refdt_ + util::Duration(static_cast<int>(OffsetTime.get()[i] * 3600));
    }
  }

// -----------------------------------------------------------------------------
/*!
 * \brief print method for stream output
 *
 * \details This method is supplied for the Printable base class. It defines
 *          how to print an object of this class in an output stream.
 */

void NetcdfIO::print(std::ostream & os) const {
  os << "Netcdf: In " << __FILE__ << " @ " << __LINE__ << std::endl;
  }

// -----------------------------------------------------------------------------
/*!
 * \brief check results of netcdf call
 *
 * \details This method will check the return code from a netcdf API call. 
 *          Successful completion of the call is indicated by the return
 *          code being equal to NC_NOERR. If the call was not successful,
 *          then the error message is written to the OOPS log, and is also
 *          sent to the OOPS ABORT call (execution is aborted).
 *
 * \param[in] RetCode Return code from netcdf call
 * \param[in] ErrorMsg Message for the OOPS error logger
 */

void NetcdfIO::CheckNcCall(int RetCode, std::string & ErrorMsg) {
  if (RetCode != NC_NOERR) {
    oops::Log::error() << ErrorMsg << " (" << RetCode << ")" << std::endl;
    ABORT(ErrorMsg);
  }
}

// -----------------------------------------------------------------------------

std::string NetcdfIO::GetNcVarName(const std::string & GroupName, const std::string & VarName) {
  // Construct the variable name found in the file. If group name is "GroupUndefined",
  // then the file variable name does not include the "@GroupName" suffix.
  std::string NcVarName;
  if (GroupName.compare("GroupUndefined") == 0) {
    // No suffix in file variable name
    NcVarName = VarName;
  } else {
    // File variable has suffix with group name
    NcVarName = VarName + "@" + GroupName;
  }

  return NcVarName;
}

}  // namespace ioda
