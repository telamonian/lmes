/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
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
 * Author(s): Elijah Roberts, Max Klein
 */
#include <csignal>
#include <cstdio>
#include <list>
#include <map>
#include <string>

#include "lm/EnumHelper.h"
#include "lm/fflux/FFluxTrajectory.h"
#include "lm/input/DiffusionModel.pb.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/input/Tilings.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

using lm::input::DiffusionModel;
using lm::input::ReactionModel;
using lm::io::TrajectoryState;
using std::map;
using std::string;

namespace lm {
namespace fflux {

FFluxTrajectory::FFluxTrajectory(uint64_t id, uint64_t phaseIndex, const lm::io::TrajectoryState& initialState)
: Trajectory(initialState, phaseIndex, id),ffluxPhase(ffluxPhase),input(input),lastLimitTime(0.0)
{
}

FFluxTrajectory::FFluxTrajectory(uint64_t id, uint64_t phase, const lm::input::Input& input, bool reversed, uint64_t ffluxPhase):
    Trajectory(input, phase, id, reversed),ffluxPhase(ffluxPhase),input(input),lastLimitTime(0.0)
{
}

FFluxTrajectory::FFluxTrajectory(uint64_t id, uint64_t phase, const TrajectoryState& initialState, uint64_t ffluxPhase, const lm::input::Input& input):
    Trajectory(initialState, phase, id),ffluxPhase(ffluxPhase),input(input),lastLimitTime(getSimTime())
{
}

FFluxTrajectory::~FFluxTrajectory()
{
}

bool FFluxTrajectory::fluxedBackward()
{
//    if (input.getCurrentTiling().getSortOrder()==TilingEnums::ASCENDING)
//    {
//        return (getLastLimitStoppingCondition()==TrajLimEnums::DECREASING);
//    }
//    else
//    {
//        return (getLastLimitStoppingCondition()==TrajLimEnums::INCREASING);
//    }
}

bool FFluxTrajectory::fluxedForward()
{
//    if (input.getCurrentTiling().getSortOrder()==TilingEnums::ASCENDING)
//    {
//        return (getLastLimitStoppingCondition()==TrajLimEnums::INCREASING);
//    }
//    else
//    {
//        return (getLastLimitStoppingCondition()==TrajLimEnums::DECREASING);
//    }
}

// accessor definitions
uint FFluxTrajectory::getFFluxPhase()
{
    return ffluxPhase;
}

TrajLimEnums::StoppingCondition FFluxTrajectory::getLastLimitStoppingCondition()
{
    return getState().limit_reached().stopping_condition();
}

double FFluxTrajectory::getLastLimitTime()
{
    return lastLimitTime;
}

// TODO: reenable?
//void FFluxTrajectory::getLastSpeciesCounts(lm::io::FFluxOutput::TrajectoryOutput* trajectoryOutputBuf)
//{
//    uint offset = (getSpeciesCounts()->number_entries() - 1)*(getSpeciesCounts()->number_species());
//    for (int i=0; i<getSpeciesCounts()->number_species(); i++)
//    {
//        trajectoryOutputBuf->add_species_count(getSpeciesCounts()->species_count(i + offset));
//    }
//}

bool FFluxTrajectory::hasElapsed(double time)
{
    return (getSimTime()>=time);
}

// mutator definitions
void FFluxTrajectory::setLastLimitTime(double llt)
{
    lastLimitTime = llt;
}

}
}
