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
#include "lm/gtest.h"

#include <csignal>
#include <map>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "lm/fflux/FFluxTrajectory.h"
#include "lm/fflux/FFluxTrajectoryList.h"
#include "lm/input/input_fixture.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.pb.h"
#include "lm/io/Tilings.pb.h"
#include "lm/oparam/OParams.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

class FFluxTrajectoryListFixture : public InputFixture
{
public:
    FFluxTrajectoryListFixture(): simultaneousTrajectoryCount(8), ffTL(NULL)
    {
//        file.getDiffusionModel(&diffBuf);
//        file.getOrderParameters(&opBuf);
//        ops.init(opBuf);
//        file.getReactionModel(&reactBuf);
//        file.getParameters(&simulationParametersBuf);
//        for (int i=0; i<simulationParametersBuf.key_size() && i<simulationParametersBuf.value_size(); i++)
//        {
//            simulationParameterMap[simulationParametersBuf.key(i)] = simulationParametersBuf.value(i);
//        }
//        file.getTilings(&tilingBuf);
//        tilings.init(tilingBuf);
        ffTL = new lm::fflux::FFluxTrajectoryList(8,*input);
        ffTL->init();
    }
    ~FFluxTrajectoryListFixture()
    {
        if (ffTL!=NULL) delete ffTL; ffTL = NULL;
    }

    uint64_t simultaneousTrajectoryCount;
    lm::fflux::FFluxTrajectoryList* ffTL;
};

TEST_F(FFluxTrajectoryListFixture, AddCrossing)
{
    lm::message::FinishedWorkUnit fWUB;
    // get a random-ish TrajectoryState in order to initialize the FinishedWorkUnit
    *fWUB.mutable_final_state() = *(ffTL->getTrajectoryState(2));

    ffTL->addCrossing(fWUB);
    EXPECT_EQ(ffTL->getCrossings(0)[0]->trajectory_id(), 2);
    EXPECT_EQ(ffTL->getCrossings(0)[0]->trajectory_started(), false);
}

TEST_F(FFluxTrajectoryListFixture, IncrFFluxPhase)
{
    EXPECT_EQ(ffTL->getFFluxPhase(), 0);
    for (int i=0;i<100;++i)
    {
        ffTL->incrFFluxPhase();
    }
    EXPECT_EQ(ffTL->getFFluxPhase(), 100);
}

TEST_F(FFluxTrajectoryListFixture, IsFFluxDone)
{
    EXPECT_EQ(ffTL->isFFluxDone(), false);
    for (int i=0;i<12;++i)
    {
        ffTL->incrFFluxPhase();
    }
    EXPECT_EQ(ffTL->isFFluxDone(), false);
    ffTL->incrFFluxPhase();
    EXPECT_EQ(ffTL->isFFluxDone(), true);
}

TEST_F(FFluxTrajectoryListFixture, IsPhaseDone)
{
    lm::message::FinishedWorkUnit fWUB;
    // get a random-ish TrajectoryState in order to initialize the FinishedWorkUnit
    *fWUB.mutable_final_state() = *(ffTL->getTrajectoryState(2));

    ffTL->incrFFluxPhase();
    EXPECT_EQ(ffTL->isPhaseDoneN(), false);
    ffTL->addCrossing(fWUB);
    EXPECT_EQ(ffTL->isPhaseDoneN(), false);
    for (int i=0;i<ffTL->getCrossingsPerPhase()-2;++i)
    {
        ffTL->addCrossing(fWUB);
    }
    EXPECT_EQ(ffTL->isPhaseDoneN(), false);
    ffTL->addCrossing(fWUB);
    EXPECT_EQ(ffTL->isPhaseDoneN(), true);
}

TEST_F(FFluxTrajectoryListFixture, IsZerothPhase)
{
    EXPECT_EQ(ffTL->isPhaseZero(), true);
    ffTL->incrFFluxPhase();
    EXPECT_EQ(ffTL->isPhaseZero(), false);
}

TEST_F(FFluxTrajectoryListFixture, IsZerothPhaseDone)
{
    // a live pointer to the SpeciesCounts that lives in the 3rd trajectory of FFluxTrajectoryList
    lm::io::SpeciesCounts* sC = ffTL->getTrajectory(2)->getState()->mutable_cme_state()->mutable_species_counts();

    EXPECT_EQ(ffTL->isZerothPhaseDone(static_cast<lm::fflux::FFluxTrajectory*>(ffTL->getTrajectory(2))->getSimTime()), false);
    sC->add_time(ffTL->getMaxPhaseZeroTime()-.02);
    EXPECT_EQ(ffTL->isZerothPhaseDone(static_cast<lm::fflux::FFluxTrajectory*>(ffTL->getTrajectory(2))->getSimTime()), false);
    sC->add_time(ffTL->getMaxPhaseZeroTime()-.01);
    EXPECT_EQ(ffTL->isZerothPhaseDone(static_cast<lm::fflux::FFluxTrajectory*>(ffTL->getTrajectory(2))->getSimTime()), false);
    sC->add_time(ffTL->getMaxPhaseZeroTime());
    EXPECT_EQ(ffTL->isZerothPhaseDone(static_cast<lm::fflux::FFluxTrajectory*>(ffTL->getTrajectory(2))->getSimTime()), true);
}

TEST_F(FFluxTrajectoryListFixture, Reset)
{
    EXPECT_EQ(ffTL->isPhaseZero(), true);
    ffTL->incrFFluxPhase();
    EXPECT_EQ(ffTL->isPhaseZero(), false);
}

TEST_F(FFluxTrajectoryListFixture, Restart)
{
    EXPECT_EQ(ffTL->isPhaseZero(), true);
    ffTL->incrFFluxPhase();
    EXPECT_EQ(ffTL->isPhaseZero(), false);
}

TEST_F(FFluxTrajectoryListFixture, SaveCrossings)
{
    lm::message::FinishedWorkUnit fWUB;
    // get a random-ish TrajectoryState in order to initialize the FinishedWorkUnit
    *fWUB.mutable_final_state() = *(ffTL->getTrajectoryState(2));

    ffTL->addCrossing(fWUB);
    ffTL->saveCrossings();

    EXPECT_EQ(ffTL->getSavedCrossings(lm::fflux::FFluxTrajectoryList::FORWARD)[0][0]->trajectory_id(), 2);
}
