/*
 * (C) Copyright 2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IO_OBSIOCONSTRUCTOR_H_
#define TEST_IO_OBSIOCONSTRUCTOR_H_

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/DateTime.h"
#include "oops/util/FloatCompare.h"
#include "oops/util/Logger.h"

#include "ioda/core/IodaUtils.h"
#include "ioda/distribution/DistributionFactory.h"
#include "ioda/io/ObsIo.h"
#include "ioda/io/ObsIoFactory.h"
#include "ioda/ObsGroup.h"
#include "ioda/ObsSpaceParameters.h"
#include "ioda/Variables/Variable.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Test Functions
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
void testConstructor() {
    const eckit::LocalConfiguration conf(::test::TestEnvironment::config());
    std::vector<eckit::LocalConfiguration> confOspaces = conf.getSubConfigurations("observations");
    util::DateTime bgn(::test::TestEnvironment::config().getString("window begin"));
    util::DateTime end(::test::TestEnvironment::config().getString("window end"));

    for (std::size_t i = 0; i < confOspaces.size(); ++i) {
        eckit::LocalConfiguration obsConfig;
        eckit::LocalConfiguration testConfig;
        confOspaces[i].get("obs space", obsConfig);
        confOspaces[i].get("test data", testConfig);
        oops::Log::trace() << "ObsIo obs space config: " << i << ": " << obsConfig << std::endl;
        oops::Log::trace() << "ObsIo test data config: " << i << ": " << testConfig << std::endl;

        ioda::ObsTopLevelParameters topParams;
        topParams.validateAndDeserialize(obsConfig);
        ioda::ObsSpaceParameters obsParams(topParams, bgn, end,
                                           oops::mpi::world(), oops::mpi::myself());

        // Try the input constructor first - should have one to try if we got here
        std::shared_ptr<ObsIo> obsIo;
        obsIo = ObsIoFactory::create(ObsIoModes::READ, obsParams);

        // Test the counts that should be set on construction
        ioda::Dimensions_t expectedMaxVarSize = testConfig.getInt("max var size", 0);
        ioda::Dimensions_t maxVarSize = obsIo->maxVarSize();
        EXPECT_EQUAL(maxVarSize, expectedMaxVarSize);

        ioda::Dimensions_t expectedNumLocs = testConfig.getInt("nlocs", 0);
        ioda::Dimensions_t numLocs = obsIo->numLocs();
        EXPECT_EQUAL(numLocs, expectedNumLocs);

        ioda::Dimensions_t expectedNumVars = testConfig.getInt("nvars", 0);
        ioda::Dimensions_t numVars = obsIo->numVars();
        EXPECT_EQUAL(numVars, expectedNumVars);

        ioda::Dimensions_t expectedNumDimVars = testConfig.getInt("ndvars", 0);
        ioda::Dimensions_t numDimVars = obsIo->numDimVars();
        EXPECT_EQUAL(numDimVars, expectedNumDimVars);

        // Try the output constructor, if one was specified
        if (obsParams.top_level_.obsOutFile.value() != boost::none) {
            setOfileParamsFromTestConfig(testConfig, obsParams);
            obsIo = ObsIoFactory::create(ObsIoModes::WRITE, obsParams);

            // See if we get expected number of locations
            std::vector<eckit::LocalConfiguration> writeDimConfigs =
                testConfig.getSubConfigurations("write dimensions");
            ioda::Dimensions_t expectedNumLocs = 0;
            for (std::size_t j = 0; j < writeDimConfigs.size(); ++j) {
                std::string dimName = writeDimConfigs[i].getString("name");
                Dimensions_t dimSize = writeDimConfigs[i].getInt("size");
                if (dimName == "Location") {
                    expectedNumLocs = dimSize;
                }
            }

            ioda::Dimensions_t numLocs = obsIo->numLocs();
            EXPECT_EQUAL(numLocs, expectedNumLocs);
        }
    }
}

// -----------------------------------------------------------------------------

class ObsIoConstructor : public oops::Test {
 public:
    ObsIoConstructor() {}
    virtual ~ObsIoConstructor() {}
 private:
    std::string testid() const override {return "test::ObsIoConstructor";}

    void register_tests() const override {
        std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

        ts.emplace_back(CASE("ioda/ObsIoConstructor/testConstructor")
            { testConstructor(); });
    }

    void clear() const override {}
};

// -----------------------------------------------------------------------------

}  // namespace test
}  // namespace ioda

#endif  // TEST_IO_OBSIOCONSTRUCTOR_H_
