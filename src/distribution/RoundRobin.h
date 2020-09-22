/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_ROUNDROBIN_H_
#define DISTRIBUTION_ROUNDROBIN_H_

#include <vector>

#include "eckit/mpi/Comm.h"
#include "oops/util/Logger.h"

#include "ioda/distribution/Distribution.h"

namespace ioda {

// ---------------------------------------------------------------------
/*!
 * \brief Round robin distribution
 *
 * \details This class implements a round-robin style of distribution which
 *          optimzes load balancing.
 *
 * \author Xin Zhang (JCSDA)
 */
class RoundRobin: public Distribution {
 public:
     explicit RoundRobin(const eckit::mpi::Comm & Comm);
     ~RoundRobin();
     bool isMyRecord(std::size_t RecNum) const override;
     bool isDistributed() const override { return true; }

     std::size_t sum(std::size_t x) override; 

};

}  // namespace ioda

#endif  // DISTRIBUTION_ROUNDROBIN_H_
