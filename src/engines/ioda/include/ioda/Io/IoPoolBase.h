#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_io IoPoolBase
 * \brief Public API for ioda::IoPoolBase
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file IoPoolBase.h
 * \brief Interfaces for ioda::IoPoolBase and related classes.
 */

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "ioda/defs.h"
#include "ioda/Engines/ReaderBase.h"
#include "ioda/Engines/WriterBase.h"
#include "ioda/Group.h"
#include "ioda/Io/IoPoolParameters.h"

#include "oops/util/DateTime.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/RequiredPolymorphicParameter.h"
#include "oops/util/Printable.h"

namespace ioda {

/// \brief IO pool base class
/// \details This class holds a single io pool which consists of a small number of MPI tasks.
/// The tasks assigned to an io pool object are selected from the total MPI tasks working on
/// the DA run. The tasks in the pool are used to transfer data to/from memory from/to a
/// ioda file. Only the tasks in the pool interact with the file and the remaining tasks outside
/// the pool interact with the pool tasks to get their individual pieces of the data being
/// transferred.
/// \ingroup ioda_cxx_io
class IODA_DL IoPoolBase : public util::Printable {
public:
  /// \brief construct an IoPoolBase object
  /// \param ioPoolParams Parameters for this io pool
  /// \param commAll MPI "all" communicator group (all tasks in DA run)
  /// \param commTime MPI "time" communicator group (tasks in current time bin for 4DEnVar)
  /// \param winStart DA timing window start
  /// \param winEnd DA timing window end
  IoPoolBase(const oops::Parameter<IoPoolParameters> & ioPoolParams,
             const eckit::mpi::Comm & commAll, const eckit::mpi::Comm & commTime,
             const util::DateTime & winStart, const util::DateTime & winEnd);
  ~IoPoolBase();

  /// \brief return nlocs for this object
  int nlocs() const { return nlocs_; }

  /// \brief return the total nlocs for this rank
  int total_nlocs() const { return total_nlocs_; }

  /// \brief return the global nlocs in the pool
  int global_nlocs() const { return global_nlocs_; }

  /// \brief return the nlocs start position
  /// \details The nlocs start position refers to the position along the nlocs dimension
  /// in the output file (when writing a single output file) where this rank's data
  /// (collected from other non io pool MPI processes) goes. For example, io pool rank 0
  /// data goes at nlocs position 0 in the file. Then if io pool rank 0 data is 10 locations
  /// long, io pool rank 1 data goes in the file at nlocs position 10 and so forth. In other
  /// words, the io pool ranks are stacking their blocks of data together (in series)
  /// in the output file.
  int nlocs_start() const { return nlocs_start_; }

  /// \brief return the "all" mpi communicator
  const eckit::mpi::Comm & comm_all() const { return comm_all_; }

  /// \brief return the rank number for the all communicator group
  int rank_all() const { return rank_all_; }

  /// \brief return the number of processes for the all communicator group
  int size_all() const { return size_all_; }

  /// \brief return the pool mpi communicator
  const eckit::mpi::Comm * comm_pool() const { return comm_pool_; }

  /// \brief return the rank number for the pool communicator group
  int rank_pool() const { return rank_pool_; }

  /// \brief return the number of processes for the pool communicator group
  int size_pool() const { return size_pool_; }

  /// \brief return the rank assignment for this object.
  const std::vector<std::pair<int, int>> & rank_assignment() const { return rank_assignment_; }

  /// \brief finalize the io pool before destruction
  virtual void finalize() = 0;

  /// \brief fill in print routine for the util::Printable base class
  virtual void print(std::ostream & os) const = 0;

protected:
  typedef std::map<int, std::vector<int>> IoPoolGroupMap;

  /// \brief io pool parameters
  const oops::Parameter<IoPoolParameters> & params_;

  /// \brief DA timing window start
  util::DateTime win_start_;

  /// \brief DA timing window end
  util::DateTime win_end_;

  /// \brief parallel io flag, true -> read/write in parallel mode
  bool is_parallel_io_;

  /// \brief target pool size
  int target_pool_size_;

  /// \brief number of locations for this rank
  std::size_t nlocs_;

  /// \brief number of patch locations for this rank
  std::size_t patch_nlocs_;

  /// \brief total number of locations (sum of this rank nlocs + assigned ranks nlocs)
  std::size_t total_nlocs_;

  /// \brief global number of locations (sum of total_nlocs_ from all ranks in the io pool)
  std::size_t global_nlocs_;

  /// \brief starting point along the nlocs dimension (for single file output)
  std::size_t nlocs_start_;

  /// \brief MPI communicator group for all processes
  const eckit::mpi::Comm & comm_all_;

  /// \brief rank in MPI communicator group for all processes
  int rank_all_;

  /// \brief size of MPI communicator group for all processes
  int size_all_;

  /// \brief MPI time communicator group
  const eckit::mpi::Comm & comm_time_;

  /// \brief rank in MPI time communicator group
  int rank_time_;

  /// \brief size of MPI time communicator group
  int size_time_;

  /// \brief MPI communicator group for all processes in the i/o pool
  /// \details This communicator group will hold a subset of the world communicator
  /// group. If an MPI task is not a member of the i/o pool, then this pointer will
  /// be set to nullptr to indicate that.
  eckit::mpi::Comm *comm_pool_;

  /// \brief rank in MPI communicator group for this pool
  int rank_pool_;

  /// \brief size of MPI communicator group for this pool
  int size_pool_;

  /// \brief ranks in the all_comm_ group that this rank transfers data
  /// \detail Each pair in this vector contains as the first element the rank number
  /// it is assigned and as the second element the number of locations for the assigned
  /// rank. Note that the data types in the pair need to match for the eckit MPI send/recv
  /// commands.
  std::vector<std::pair<int, int>> rank_assignment_;

  /// \brief set the pool size (number of MPI processes) for this instance
  /// \detail This function sets the data member target_pool_size_ to the minumum of
  /// the specified maximum pool size or the size of the comm_all_ communicator group.
  void setTargetPoolSize();

  /// \brief group ranks into sets for the io pool assignments
  /// \detail This function will create a vector of vector of ints structure which
  /// shows how to form the io pool and how to assign the non io pool ranks to each
  /// of the ranks in the io pool.
  /// \param rankGrouping structure that maps ranks outside the pool to ranks in the pool
  virtual void groupRanks(IoPoolGroupMap & rankGrouping) = 0;

  /// \brief assign ranks in the comm_all_ comm group to each of the ranks in the io pool
  /// \detail This function will dole out the ranks within the comm_all_ group, that are
  /// not in the io pool, to the ranks that are in the io pool. This sets up the send/recv
  /// communication for collecting the variable data. When finished, all ranks in the
  /// comm_all_ group will have a list of all the ranks that the send to or receive from.
  /// \param nlocs number of locations on this MPI rank
  /// \param rankGrouping structure that maps ranks outside the pool to ranks in the pool
  virtual void assignRanksToIoPool(const std::size_t nlocs,
                                   const IoPoolGroupMap & rankGrouping) = 0;

  /// \brief create the io pool communicator group
  /// \detail This function will create the io pool communicator group using the eckit
  /// MPI split command. This function sets the comm_pool_, rank_pool_ and size_pool_
  /// data members. If this rank is not in the io pool communicator group comm_pool_ is
  /// set to nullptr, and both rank_pool_ and size_pool_ are set to -1. 
  /// \param rankGrouping structure that maps ranks outside the pool to ranks in the pool
  void createIoPool(IoPoolGroupMap & rankGrouping);

  /// \brief collect nlocs from assigned ranks and compute total for this rank
  /// \detail For each of the ranks in the io pool, this function collects nlocs from
  /// all of the assigned ranks and sums up them up to get the total nlocs for each
  /// output file.
  /// \param nlocs Number of locations for this rank.
  void setTotalNlocs(const std::size_t nlocs);

  /// \brief collect information related to a single file output from all ranks in the io pool
  /// \detail This function will collect two pieces of information. The first is the sum
  /// total nlocs for all ranks in the io pool. This value represents the total amount
  /// of nlocs from all obs spaces in the all communicator group. The global nlocs value
  /// is used to properly size the variables when writing to a single output file.
  /// The second piece of information is the proper start values for each rank in regard
  /// to the nlocs dimension when writing to a single output file.
  void collectSingleFileInfo();
};

}  // namespace ioda

/// @}
