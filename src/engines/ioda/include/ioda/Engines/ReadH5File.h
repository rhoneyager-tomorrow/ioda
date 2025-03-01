#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "ioda/Engines/ReaderBase.h"

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------------------------
// ReadH5File
//----------------------------------------------------------------------------------------

// Parameters

class ReadH5FileParameters : public ReaderParametersBase {
    OOPS_CONCRETE_PARAMETERS(ReadH5FileParameters, ReaderParametersBase)

  public:
    /// \brief Path to input file
    oops::RequiredParameter<std::string> fileName{"obsfile", this};
};

// Classes

class ReadH5File: public ReaderBase {
 public:
  typedef ReadH5FileParameters Parameters_;

  // Constructor via parameters
  ReadH5File(const Parameters_ & params, const ReaderCreationParameters & createParams);

  void print(std::ostream & os) const override;

 private:
  std::string fileName_;
};

}  // namespace Engines
}  // namespace ioda
