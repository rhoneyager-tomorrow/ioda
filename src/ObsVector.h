/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef OBSVECTOR_H_
#define OBSVECTOR_H_

#include <Eigen/Dense>
#include <ostream>
#include <string>
#include <vector>

#include "ioda/ObsSpace.h"
#include "oops/base/Variables.h"
#include "oops/util/ObjectCounter.h"
#include "oops/util/Printable.h"

namespace ioda {
  template <typename DATATYPE> class ObsDataVector;

//-----------------------------------------------------------------------------
/*! \brief ObsVector class to handle vectors in observation space for IODA
 *
 *  \details This class holds observation vector data. Examples of an obs vector
 *           are the y vector and the H(x) vector. The methods of this class
 *           that implement vector operations (e.g., bitwise add, bitwise subtract,
 *           dot product) are capable of handling missing values in the obs data.
 */

class ObsVector : public util::Printable,
                  private util::ObjectCounter<ObsVector> {
 public:
  static const std::string classname() {return "ioda::ObsVector";}

  explicit ObsVector(ObsSpace &, const std::string & name = "");
  ObsVector(const ObsVector &);
  ~ObsVector();

  ObsVector & operator = (const ObsVector &);
  ObsVector & operator*= (const double &);
  ObsVector & operator+= (const ObsVector &);
  ObsVector & operator-= (const ObsVector &);
  ObsVector & operator*= (const ObsVector &);
  ObsVector & operator/= (const ObsVector &);

  ObsVector & operator = (const ObsDataVector<float> &);

  void zero();
  /// set all elements to one (used in tests)
  void ones();
  /// adds \p beta * \p y to the current vector
  void axpy(const double & beta, const ObsVector & y);
  /// adds \p beta[ivar] * \p y[ivar] for each variable in the current
  /// vector. \p beta has to be size of variables
  void axpy(const std::vector<double> & beta, const ObsVector & y);

  void invert();
  void random();

  /// global (across all MPI tasks) dot product of this with \p other
  double dot_product_with(const ObsVector & other) const;
  /// global (across all MPI tasks) dot product of this with \p other,
  /// variable by variable. Returns vectors size of nvars_
  std::vector<double> multivar_dot_product_with(const ObsVector & other) const;

  double rms() const;

  std::size_t size() const {return values_.size();}  // Size of vector in local memory
  const double & operator[](const std::size_t ii) const {return values_.at(ii);}
  double & operator[](const std::size_t ii) {return values_.at(ii);}

  /// Number of active observations (missing values not included) across all MPI tasks
  unsigned int nobs() const;

  /// Pack observations local to this MPI task into an Eigen vector
  /// (excluding vector elements that are missing values, or where mask is equal to
  /// missing values
  Eigen::VectorXd packEigen(const ObsVector & mask) const;
  /// Number of non-masked out observations local to this MPI task
  /// (size of an Eigen vector returned by `packEigen`)
  size_t packEigenSize(const ObsVector & mask) const;

  const double & toFortran() const;
  double & toFortran();

  ObsSpace & space() {return obsdb_;}
  const ObsSpace & space() const {return obsdb_;}
  std::vector<double> data() const {return values_;}
  const std::string & obstype() const {return obsdb_.obsname();}
  const oops::Variables & varnames() const {return obsvars_;}
  std::size_t nvars() const {return nvars_;}
  std::size_t nlocs() const {return nlocs_;}

  /// Set this ObsVector values to missing where \p mask has missing values
  void mask(const ObsVector & mask);

  bool has(const std::string & var) const {return obsvars_.has(var);}

  int64_t getSeed() const {return obsdb_.getSeed();}

// I/O
  void save(const std::string &) const;
  void read(const std::string &);

 private:
  void print(std::ostream &) const;

  /*! \brief Associate ObsSpace object */
  ObsSpace & obsdb_;

  /*! \brief Variables */
  oops::Variables obsvars_;

  /*! \brief Number of variables */
  std::size_t nvars_;
  std::size_t nlocs_;

  /*! \brief Vector data */
  std::vector<double> values_;

  /*! \brief Missing data mark */
  const double missing_;
};
// -----------------------------------------------------------------------------

}  // namespace ioda

#endif  // OBSVECTOR_H_
