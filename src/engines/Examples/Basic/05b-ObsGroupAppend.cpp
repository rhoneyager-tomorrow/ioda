/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_ex_basic
 *
 * @{
 *
 * \defgroup ioda_cxx_ex_5b Ex 5b: Appending to ObsGroups
 * \brief Appending to ObsGroups
 * \see 05b-ObsGroupAppend.cpp for comments and the walkthrough.
 *
 * @{
 *
 * \file 05b-ObsGroupAppend.cpp
 * \brief ObsGroup
 *
 * The ObsGroup class is derived from the Group class and provides some help in
 * organizing your groups, variables, attributes and dimension scales into a cohesive
 * structure intended to house observation data. In this case "structure" refers to the
 * hierarchical layout of the groups and the proper management of dimension scales
 * associated with the variables.
 *
 * The ObsGroup and underlying layout policies (internal to ioda-engines) present a stable
 * logical group hierarchical layout to the client while keeping the actual layout implemented
 * in the backend open to change. The logical "frontend" layout appears to the client to be
 * as shown below:
 *
 *   layout                                    notes
 *
 *     /                                   top-level group
 *      Location                           dimension scales (variables, coordinate values)
 *      Channel
 *      ...
 *      ObsValue/                          group: observational measurement values
 *               brightnessTemperature    variable: Tb, 2D, Location X Channel
 *               air_temperature           variable: T, 1D, Location
 *               ...
 *      ObsError/                          group: observational error estimates
 *               brightnessTemperature
 *               air_temperature
 *               ...
 *      PreQC/                             group: observational QC marks from data provider
 *               brightnessTemperature
 *               air_temperature
 *               ...
 *      MetaData/                          group: meta data associated with locations
 *               latitude
 *               longitude
 *               datetime
 *               ...
 *      ...
 *
 * It is intended to keep this layout stable so that the client interface remains stable.
 * The actual layout used in the various backends can optionally be organized differently
 * according to their needs.
 *
 * The ObsGroup class also assists with the management of dimension scales. For example, if
 * a dimension is resized, the ObsGroup::resize function will resize the dimension scale
 * along with all variables that use that dimension scale.
 *
 * The basic ideas is to dimension observation data with Location as the first dimension, and
 * allow Location to be resizable so that it's possible to incrementally append data along
 * the Location (1st) dimension. For data that have rank > 1, the second through nth dimensions
 * are of fixed size. For example, brightnessTemperature can be store as 2D data with
 * dimensions (Location, Channel).
 *
 * \author Stephen Herbener (stephenh@ucar.edu), Ryan Honeyager (honeyage@ucar.edu)
 **/

#include <array>     // Arrays are fixed-length vectors.
#include <iomanip>   // std::setw
#include <iostream>  // We want I/O.
#include <numeric>   // std::iota
#include <string>    // We want strings
#include <valarray>  // Like a vector, but can also do basic element-wise math.
#include <vector>    // We want vectors

#include "Eigen/Dense"             // Eigen Arrays and Matrices
#include "ioda/Engines/EngineUtils.h"  // Used to kickstart the Group engine.
#include "ioda/Exception.h"        // Exceptions and debugging
#include "ioda/Group.h"            // Groups have attributes.
#include "ioda/ObsGroup.h"
#include "unsupported/Eigen/CXX11/Tensor"  // Eigen Tensors

int main(int argc, char** argv) {
  using namespace ioda;  // All of the ioda functions are here.
  using std::cerr;
  using std::endl;
  using std::string;
  using std::vector;
  try {
    // It's possible to transfer data in smaller pieces so you can, for example, avoid
    // reading the whole input file into memory. Transferring by pieces can also be useful
    // when you don't know a priori how many locations are going to be read in. To accomplish
    // this, you set the maximum size of the Location dimension to Unlimited and use the
    // ObsSpace::generate function to allocate more space at the end of each variable for
    // the incoming section.
    //
    // For this example, we'll use the same data as in example 05a, but transfer it to
    // the backend in four pieces, 10 locations at a time.

    // Create the backend. For this code we are using a factory function,
    // constructFromCmdLine, made for testing purposes, which allows one to specify
    // a backend from the command line using the "--ioda-engine-options" option.
    //
    // There exists another factory function, constructBackend, which allows you
    // to create a backend without requiring the command line option. The signature
    // for this function is:
    //
    //        constructBackend(BackendNames, BackendCreationParameters &);
    //
    //
    //        BackendNames is an enum type with values:
    //               Hdf5File - file backend using HDF5 file
    //               ObsStore - in-memory backend
    //
    //        BackendCreationParameters is a C++ structure with members:
    //               fileName - string, used for file backend
    //
    //               actions - enum BackendFileActions type:
    //                    Create - create a new file
    //                    Open   - open an existing file
    //
    //               createMode - enum BackendCreateModes type:
    //                    Truncate_If_Exists - overwrite existing file
    //                    Fail_If_Exists     - throw exception if file exists
    //
    //               openMode - enum BackendOpenModes types:
    //                    Read_Only  - open in read only mode
    //                    Read_Write - open in modify mode
    //
    // Here are some code examples:
    //
    // Create backend using an hdf5 file for writing:
    //
    //        Engines::BackendNames backendName;
    //        backendName = Engines::BackendNames::Hdf5File;
    //
    //        Engines::BackendCreationParameters backendParams;
    //        backendParams.fileName = fileName;
    //        backendParams.action = Engines::BackendFileActions::Create;
    //        backendParams.createMode = Engines::BackendCreateModes::Truncate_If_Exists;
    //
    //        Group g = constructBackend(backendName, backendParams);
    //
    // Create backend using an hdf5 file for reading:
    //
    //        Engines::BackendNames backendName;
    //        backendName = Engines::BackendNames::Hdf5File;
    //
    //        Engines::BackendCreationParameters backendParams;
    //        backendParams.fileName = fileName;
    //        backendParams.action = Engines::BackendFileActions::Open;
    //        backendParams.openMode = Engines::BackendOpenModes::Read_Only;
    //
    //        Group g = constructBackend(backendName, backendParams);
    //
    // Create an in-memory backend:
    //
    //        Engines::BackendNames backendName;
    //        backendName = Engines::BackendNames::ObsStore;
    //
    //        Engines::BackendCreationParameters backendParams;
    //
    //        Group g = constructBackend(backendName, backendParams);
    //

    // Create the backend using the command line construct function
    Group g = Engines::constructFromCmdLine(argc, argv, "Example-05b.hdf5");

    // Create an ObsGroup object using the ObsGroup::generate function. This function
    // takes a Group arguemnt (the backend we just created above) and a vector of dimension
    // creation specs.
    const int numLocs     = 40;
    const int numChans    = 30;
    const int sectionSize = 10;  // experiment with different sectionSize values

    // The NewDimensionsScales_t is a vector, that holds specs for one dimension scale
    // per element. An individual dimension scale spec is held in a NewDimensionsScale
    // object, whose constructor arguments are:
    //       1st - dimension scale name
    //       2nd - size of dimension. May be zero.
    //       3rd - maximum size of dimension
    //              resizeable dimensions are said to have "unlimited" size, so there
    //              is a built-in variable ("Unlimited") that can be used to denote
    //              unlimited size. If Unspecified (the default), we assume that the
    //              maximum size is the same as the initial size (the previous parameter).
    //       4th - suggested chunk size for dimension (and associated variables).
    //              This defaults to the initial size. This parameter must be nonzero. If
    //              the initial size is zero, it must be explicitly specified.
    //
    // For transferring data in pieces, make sure that Location maximum dimension size is
    // set to Unlimited. We'll set the initial size of Location to the sectionSize (10).
    ioda::NewDimensionScales_t newDims{
      NewDimensionScale<int>("Location", sectionSize, Unlimited),
      NewDimensionScale<int>("Channel", numChans)
    };

    // Construct an ObsGroup object, with 2 dimensions Location, Channel, and attach
    // the backend we constructed above. Under the hood, the ObsGroup::generate function
    // initializes the dimension coordinate values to index numbering 1..n. This can be
    // overwritten with other coordinate values if desired.
    ObsGroup og = ObsGroup::generate(g, newDims);

    // We now have the top-level group containing the two dimension scales. We need
    // Variable objects for these dimension scales later on for creating variables so
    // build those now.
    ioda::Variable LocationVar  = og.vars["Location"];
    ioda::Variable ChannelVar = og.vars["Channel"];

    // Next let's create the variables. The variable names should be specified using the
    // hierarchy as described above. For example, the variable brightnessTemperature
    // in the group ObsValue is specified in a string as "ObsValue/brightnessTemperature".
    string tbName  = "ObsValue/brightnessTemperature";
    string latName = "MetaData/latitude";
    string lonName = "MetaData/longitude";

    // Set up the creation parameters for the variables. All three variables in this case
    // are float types, so they can share the same creation parameters object.
    ioda::VariableCreationParameters float_params;
    float_params.chunk = true;               // allow chunking
    float_params.compressWithGZIP();         // compress using gzip
    float_params.setFillValue<float>(-999);  // set the fill value to -999

    // Create the variables. Note the use of the createWithScales function. This should
    // always be used when working with an ObsGroup object.
    Variable tbVar  = og.vars.createWithScales<float>(tbName, {LocationVar, ChannelVar}, float_params);
    Variable latVar = og.vars.createWithScales<float>(latName, {LocationVar}, float_params);
    Variable lonVar = og.vars.createWithScales<float>(lonName, {LocationVar}, float_params);

    // Add attributes to variables. In this example, we are adding enough attribute
    // information to allow Panoply to be able to plot the ObsValue/brightnessTemperature
    // variable. Note the "coordinates" attribute on tbVar. It is sufficient to just
    // give the variable names (without the group structure) to Panoply (which apparently
    // searches the entire group structure for these names). If you want to follow this
    // example in your code, just give the variable names without the group prefixes
    // to insulate your code from any subsequent group structure changes that might occur.
    tbVar.atts.add<std::string>("coordinates", {"longitude latitude Channel"}, {1})
      .add<std::string>("long_name", {"ficticious brightness temperature"}, {1})
      .add<std::string>("units", {"K"}, {1})
      .add<float>("valid_range", {100.0, 400.0}, {2});
    latVar.atts.add<std::string>("long_name", {"latitude"}, {1})
      .add<std::string>("units", {"degrees_north"}, {1})
      .add<float>("valid_range", {-90.0, 90.0}, {2});
    lonVar.atts.add<std::string>("long_name", {"longitude"}, {1})
      .add<std::string>("units", {"degrees_east"}, {1})
      .add<float>("valid_range", {-360.0, 360.0}, {2});

    // Let's create some data for this example.
    Eigen::ArrayXXf tbData(numLocs, numChans);
    std::vector<float> lonData(numLocs);
    std::vector<float> latData(numLocs);
    float midLoc  = static_cast<float>(numLocs) / 2.0f;
    float midChan = static_cast<float>(numChans) / 2.0f;
    for (std::size_t i = 0; i < numLocs; ++i) {
      lonData[i] = static_cast<float>(i % 8) * 3.0f;
      // We use static code analysis tools to check for potential bugs
      // in our source code. On the next line, the clang-tidy tool warns about
      // our use of integer division before casting to a float. Since there is
      // no actual bug, we indicate this with NOLINT.
      latData[i] = static_cast<float>(i / 8) * 3.0f;  // NOLINT(bugprone-integer-division)
      for (std::size_t j = 0; j < numChans; ++j) {
        float del_i  = static_cast<float>(i) - midLoc;
        float del_j  = static_cast<float>(j) - midChan;
        tbData(i, j) = 250.0f + sqrt(del_i * del_i + del_j * del_j);
      }
    }

    // Transfer the data piece by piece. In this case we are moving consecutive,
    // contiguous pieces from the source to the backend.
    //
    // Things to consider:
    //     If numLocs/sectionSize has a remainder, then the final section needs to be
    //     smaller to match up.
    //
    //     The new size for resizing the variables needs to be the current size
    //     plus the count for this section.
    std::size_t numLocsTransferred = 0;
    std::size_t isection           = 1;
    int fwidth                     = 10;
    std::cout << "Transferring data in sections to backend:" << std::endl << std::endl;
    std::cout << std::setw(fwidth) << "Section" << std::setw(fwidth) << "Start" << std::setw(fwidth)
              << "Count" << std::setw(fwidth) << "Resize" << std::endl;
    while (numLocsTransferred < numLocs) {
      // Figure out the starting point and size (count) for the current piece.
      std::size_t sectionStart = numLocsTransferred;
      std::size_t sectionCount = sectionSize;
      if ((sectionStart + sectionCount) > numLocs) {
        sectionCount = numLocs - sectionStart;
      }

      // Figure out the new size for the Location dimension
      Dimensions LocationDims = LocationVar.getDimensions();
      Dimensions_t LocationNewSize
        = (isection == 1) ? sectionCount : LocationDims.dimsCur[0] + sectionCount;

      // Print out stats so you can see what's going on
      std::cout << std::setw(fwidth) << isection << std::setw(fwidth) << sectionStart
                << std::setw(fwidth) << sectionCount << std::setw(fwidth) << LocationNewSize
                << std::endl;

      // Resize the Location dimension
      og.resize({std::pair<Variable, Dimensions_t>(LocationVar, LocationNewSize)});

      // Create selection objects for transferring the data
      // We'll use the HDF5 hyperslab style of selection which denotes a start index
      // and count for each dimension. The start and count values need to be vectors
      // where the ith entry corresponds to the ith dimension of the variable. Latitue
      // and longitude are 1D and Tb is 2D. Start with 1D starts and counts denoting
      // the sections to transfer for lat and lon, then add the start and count values
      // for channels and transfer Tb.

      // starts and counts for lat and lon
      std::vector<Dimensions_t> starts(1, sectionStart);
      std::vector<Dimensions_t> counts(1, sectionCount);

      Selection feSelect;
      feSelect.extent({LocationNewSize}).select({SelectionOperator::SET, starts, counts});
      Selection beSelect;
      beSelect.select({SelectionOperator::SET, starts, counts});

      latVar.write<float>(latData, feSelect, beSelect);
      lonVar.write<float>(lonData, feSelect, beSelect);

      // Add the start and count values for the channels dimension. We will select
      // all channels, so start is zero, and count is numChans
      starts.push_back(0);
      counts.push_back(numChans);

      Selection feSelect2D;
      feSelect2D.extent({LocationNewSize, numChans}).select({SelectionOperator::SET, starts, counts});
      Selection beSelect2D;
      beSelect2D.select({SelectionOperator::SET, starts, counts});

      tbVar.writeWithEigenRegular(tbData, feSelect2D, beSelect2D);

      numLocsTransferred += sectionCount;
      isection++;
    }

    // The ObsGroup::generate program has, under the hood, automatically assigned
    // the coordinate values for Location and Channel dimension scale variables. The
    // auto-assignment uses the values 1..n upon creation. Since we resized Location,
    // the coordinates at this point will be set to 1..sectionSize followed by all
    // zeros to the end of the variable. This can be addressed two ways:
    //
    //    1. In the above loop, add a write to the Location variable with the corresponding
    //       coordinate values for each section.
    //    2. In the case where you simply want 1..n as the coordinate values, wait
    //       until transferring all the sections of variable data, check the size
    //       of the Location variable, and write the entire 1..n values to the variable.
    //
    // We'll do option 2 here
    int LocationSize = gsl::narrow<int>(LocationVar.getDimensions().dimsCur[0]);
    std::vector<int> LocationVals(LocationSize);
    std::iota(LocationVals.begin(), LocationVals.end(), 1);
    LocationVar.write(LocationVals);

    // Done!
  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}
