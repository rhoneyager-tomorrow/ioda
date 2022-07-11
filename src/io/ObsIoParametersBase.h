/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIOPARAMETERSBASE_H_
#define IO_OBSIOPARAMETERSBASE_H_

#include <string>
#include <vector>

#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"

namespace ioda {

enum class MissingSortValueTreatment {
  SORT, NO_SORT, IGNORE_MISSING
};

struct MissingSortValueTreatmentParameterTraitsHelper {
  typedef MissingSortValueTreatment EnumType;
  static constexpr char enumTypeName[] = "MissingSortValueTreatment";
  static constexpr util::NamedEnumerator<MissingSortValueTreatment> namedValues[] = {
    { MissingSortValueTreatment::SORT, "sort" },
    { MissingSortValueTreatment::NO_SORT, "do not sort" },
    { MissingSortValueTreatment::IGNORE_MISSING, "ignore missing" }
  };
};

}  // namespace ioda


namespace oops {

template <>
struct ParameterTraits<ioda::MissingSortValueTreatment> :
    public EnumParameterTraits<ioda::MissingSortValueTreatmentParameterTraitsHelper>
{};

}  // namespace oops


namespace ioda {

constexpr int DEFAULT_FRAME_SIZE = 10000;

/// \brief Options controlling the manner in which observations are grouped into records.
class ObsGroupingParameters : public oops::Parameters {
    OOPS_CONCRETE_PARAMETERS(ObsGroupingParameters, Parameters)

 public:
    /// variable of which to base obs record grouping
    oops::Parameter<std::vector<std::string>> obsGroupVars{"group variables", {}, this};

    /// variable of which to base obs record sorting
    oops::Parameter<std::string> obsSortVar{"sort variable", "", this};

    /// name of group of which to base obs record sorting
    oops::Parameter<std::string> obsSortGroup{"sort group", "MetaData", this};

    /// direction for sort
    oops::Parameter<std::string> obsSortOrder{"sort order", "ascending", this};

    /// treatment of missing sort values
    oops::Parameter<MissingSortValueTreatment> missingSortValueTreatment{
      "missing sort value treatment", MissingSortValueTreatment::SORT, this};
};

/// \brief Base of classes storing the configuration parameters of ObsIo subclasses.
class ObsIoParametersBase : public oops::Parameters {
    OOPS_ABSTRACT_PARAMETERS(ObsIoParametersBase, Parameters)

 public:
    /// \brief Identifies the ObsIo subclass to use.
    ///
    /// \note This parameter is marked as optional because it is only required in certain
    /// circumstances (e.g. when ObsIo parameters are deserialized into an ObsIoParametersWrapper
    /// and used by ObsIoFactory to instantiate an ObsIo implementation whose type is determined at
    /// runtime), but not others (e.g. in tests written with a particular ObsIo subclass in mind).
    /// ObsIoParametersWrapper will throw an exception if this parameter is not provided.
    oops::OptionalParameter<std::string> type{"type", this};

    /// options controlling obs record grouping
    oops::Parameter<ObsGroupingParameters> obsGrouping{"obsgrouping", {}, this};

    /// maximum frame size
    oops::Parameter<int> maxFrameSize{"max frame size", DEFAULT_FRAME_SIZE, this};
};

}  // namespace ioda

#endif  // IO_OBSIOPARAMETERSBASE_H_
