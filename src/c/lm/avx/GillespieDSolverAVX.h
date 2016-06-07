/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
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
 * Author(s): Elijah Roberts
 */

#ifdef OPT_AVX

#ifndef LM_AVX_GILLESPIEDSOLVERAVX_H_
#define LM_AVX_GILLESPIEDSOLVERAVX_H_

#include <deque>
#include <map>
#include <list>
#include <string>

#include <immintrin.h>

#include "lm/ClassFactory.h"
#include "lm/cme/GillespieDSolver.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/rng/RandomGenerator.h"

using std::deque;
using std::list;
using std::map;
using std::pair;
using std::string;
using lm::rng::RandomGenerator;

namespace lm {
namespace avx {

class GillespieDSolverAVX : public lm::cme::GillespieDSolver
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

public:
    GillespieDSolverAVX();
    virtual ~GillespieDSolverAVX();
    virtual uint getSimultaneousTrajectories();
    virtual void setReactionModel(const lm::io::ReactionModel& rm);
    virtual void setOrderParameters(const lm::io::OrderParameters& opsBuf);
    virtual void setLimits(const lm::io::TrajectoryLimits& limits);
    virtual void reset();
    virtual void getState(lm::io::TrajectoryState* state, uint trajectoryNumber=0);
    virtual void setState(const lm::io::TrajectoryState& state, uint trajectoryNumber=0);
    virtual long long generateTrajectory(long long maxSteps);
    virtual lm::message::WorkUnitOutput* getOutput(uint trajectoryNumber=0);
    virtual lm::message::WorkUnitStatus::Status getStatus(uint trajectoryNumber=0);

protected:
    void updateAllPropensities();
    void updatePropensities(avxd time, uint* sourceReaction);
    void performReactionEventAVX(uint* reactionsToPerform);
    void callUpdateSpeciesCountsListenersAVX();
    bool isTrajectoryOutsideLimitsAVX();
    void copyTrajectoryStateToBaseSolver(uint trajectoryNumber);
    void copyTrajectoryStateFromBaseSolver(uint trajectoryNumber);

protected:
    // If the trajectory has been initialized.
    bool initialized[DOUBLES_PER_AVX];

    // Trajectory output.
    lm::message::WorkUnitOutput* output[DOUBLES_PER_AVX];

    // Trajectory status.
    lm::message::WorkUnitStatus::Status status[DOUBLES_PER_AVX];

    // Limits for the trajectory.
    avxd timeLimit;
    int32_t limitIDReached[DOUBLES_PER_AVX];
    lm::io::TrajectoryLimits::LimitType limitTypeReached[DOUBLES_PER_AVX];
    double* limitValues;

    //First passage time variables.
    uint numberFptValues;
    double* fptMinValuesAchieved;
    double* fptMaxValuesAchieved;
    deque<pair<int,double> >* fptValues;

    // The current state.
    uint64_t trajectoryId[DOUBLES_PER_AVX];
    bool trajectoryStarted[DOUBLES_PER_AVX];
    double* speciesCounts;
    double* propensities;
    avxd time;
    avxd timeStep;
    double* orderParameterValues;
    double* orderParameterPreviousValues;
};

}
}

#endif
#endif
