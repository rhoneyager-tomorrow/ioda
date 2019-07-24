/*
 * (C) Copyright 2017 UCAR
 * 
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
 */

#include "ioda/obsspace_f.h"

#include <string>
#include <vector>

#include "eckit/exception/Exceptions.h"

#include "oops/util/DateTime.h"

#include "ioda/ObsSpace.h"

namespace ioda {

// -----------------------------------------------------------------------------
int obsspace_get_nlocs_f(const ObsSpace & obss) {
  return obss.nlocs();
}
// -----------------------------------------------------------------------------
bool obsspace_has_f(const ObsSpace & obss, const char * group, const char * vname) {
  return obss.has(std::string(group), std::string(vname));
}
// -----------------------------------------------------------------------------
void obsspace_get_int32_f(const ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int32_t* vec) {
  ASSERT(length >= obss.nlocs());
  obss.get_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_int64_f(const ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int64_t* vec) {
  ASSERT(length >= obss.nlocs());
//  obss.get_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_real32_f(const ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, float* vec) {
  ASSERT(length >= obss.nlocs());
//  obss.get_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_real64_f(const ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, double* vec) {
  ASSERT(length >= obss.nlocs());
  obss.get_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_get_datetime_f(const ObsSpace & obss, const char * group, const char * vname,
                             const std::size_t & length, int32_t* date, int32_t* time) {
  ASSERT(length >= obss.nlocs());

  // Load a DateTime vector from the database, then convert to a date and time
  // vector which are then returned.
  util::DateTime temp_dt("0000-01-01T00:00:00Z");
  std::vector<util::DateTime> dt_vect(length, temp_dt);
  obss.get_db(std::string(group), std::string(vname), length, dt_vect.data());

  // Convert to date and time values. The DateTime utilities can return year, month,
  // day, hour, minute second.
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  for (std::size_t i = 0; i < length; i++) {
    dt_vect[i].toYYYYMMDDhhmmss(year, month, day, hour, minute, second);
    date[i] = (year * 10000) + (month * 100) + day;
    time[i] = (hour * 10000) + (minute * 100) + second;
  }
}
// -----------------------------------------------------------------------------
void obsspace_put_int32_f(ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int32_t* vec) {
  ASSERT(length == obss.nlocs());
  obss.put_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_put_int64_f(ObsSpace & obss, const char * group, const char * vname,
                          const std::size_t & length, int64_t* vec) {
  ASSERT(length == obss.nlocs());
//  obss.put_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_put_real32_f(ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, float* vec) {
  ASSERT(length == obss.nlocs());
//  obss.put_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------
void obsspace_put_real64_f(ObsSpace & obss, const char * group, const char * vname,
                           const std::size_t & length, double* vec) {
  ASSERT(length == obss.nlocs());
  obss.put_db(std::string(group), std::string(vname), length, vec);
}
// -----------------------------------------------------------------------------

}  // namespace ioda
