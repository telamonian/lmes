/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the Software), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimers.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimers in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the names of the Roberts Group, Johns Hopkins University,
 * nor the names of its contributors may be used to endorse or
 * promote products derived from this Software without specific prior written
 * permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS WITH THE SOFTWARE.
 *
 * Author(s): Elijah Roberts, Max Klein
 */
#include <map>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "lm/fflux/FFluxTrajectory.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.pb.h"
#include "lm/io/Tilings.pb.h"
#include "lm/oparam/OParams.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

class FFluxTrajectoryFixture : public ::testing::Test
{
public:
    FFluxTrajectoryFixture(): simultaneousTrajectoryCount(8), ffT(NULL), file("/Users/tel/git/lm/gtest/data/lm/fflux/biphasic_switch.lm")
    {
        file.getDiffusionModel(&diffBuf);
        file.getOrderParameters(&opBuf);
        ops.init(opBuf);
        file.getReactionModel(&reactBuf);
        file.getParameters(&simulationParametersBuf);
        for (int i=0; i<simulationParametersBuf.key_size() && i<simulationParametersBuf.value_size(); i++)
        {
            simulationParameterMap[simulationParametersBuf.key(i)] = simulationParametersBuf.value(i);
        }
        file.getTilings(&tilingBuf);
        tilings.init(tilingBuf);
        ffT = new lm::fflux::FFluxTrajectory(0,0,reactBuf,diffBuf,simulationParameterMap,tilings);
    }
    static uint speciesCounts[7];
    uint64_t simultaneousTrajectoryCount;
    std::map<std::string,std::string> simulationParameterMap;
    lm::fflux::FFluxTrajectory* ffT;
    lm::io::hdf5::Hdf5File file;
    lm::io::DiffusionModel diffBuf;
    lm::io::OrderParameters opBuf;
    lm::io::ReactionModel reactBuf;
    lm::io::SimulationParameters simulationParametersBuf;
    lm::io::Tilings tilingBuf;
    lm::oparam::OParams ops;
    lm::tiling::Tilings tilings;
};

TEST_F(FFluxTrajectoryFixture, FluxedBackward)
{
    ffT->getState()->set_final_limit_type(lm::io::TrajectoryLimits::DECREASINGORDERPARAMETER);
    EXPECT_EQ(ffT->fluxedBackward(), true);
}

TEST_F(FFluxTrajectoryFixture, FluxedForward)
{
    ffT->getState()->set_final_limit_type(lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER);
    EXPECT_EQ(ffT->fluxedForward(), true);
}

TEST_F(FFluxTrajectoryFixture, GetFinalLimitType)
{
    ffT->getState()->set_final_limit_type(lm::io::TrajectoryLimits::DECREASINGORDERPARAMETER);
    EXPECT_EQ(ffT->getFinalLimitType(), lm::io::TrajectoryLimits::DECREASINGORDERPARAMETER);
    ffT->getState()->set_final_limit_type(lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER);
    EXPECT_EQ(ffT->getFinalLimitType(), lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER);
}

TEST_F(FFluxTrajectoryFixture, GetSimSteps)
{
    EXPECT_EQ(ffT->getSimSteps(), 1);
}

TEST_F(FFluxTrajectoryFixture, GetSimTime)
{
    EXPECT_EQ(ffT->getSimTime(), 0);
}

TEST_F(FFluxTrajectoryFixture, HasElapsed)
{
    EXPECT_EQ(ffT->hasElapsed(0), true);
    EXPECT_EQ(ffT->hasElapsed(1.3), false);
}
