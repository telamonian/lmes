/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *                  Johns Hopkins University
 *                  http://biophysics.jhu.edu/roberts/
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
#include <algorithm>
#include <cmath>
#include <csignal>
#include <functional>
#include <list>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include "lm/EnumHelper.h"
#include "lm/Exceptions.h"
#include "lm/fflux/FFluxTrajectoryList.h"
#include "lm/fflux/input/FFluxPhase.pb.h"
#include "lm/io/CMEState.pb.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/FFluxOutput.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/main/Globals.h"
#include "lm/main/Main.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/Print.h"
#include "lm/fflux/io/FFluxPhaseOutputWrap.h"
#include "lm/protowrap/Repeated.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/tiling/Tilings.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using lm::fflux::input::FFluxInput;
using lm::fflux::input::FFluxPhase;
using lm::fflux::input::FFluxPhaseLimit;
using lm::protowrap::FFluxPhaseOutputWrap;
using lm::protowrap::Repeated;
using std::map;
using std::string;
using std::vector;

namespace lm {
namespace fflux {

// ffluxPhase n==0 constructor
FFluxTrajectoryList::FFluxTrajectoryList(uint64_t count, uint64_t newSimulationPhaseIndex, const FFluxPhase& ffluxPhase, const FFluxPhaseLimit& ffluxPhaseLimit, uint simultaneousTrajectoryCount, const FFluxInput& input, const lm::input::Basin& basin)
:TrajectoryList(count, newSimulationPhaseIndex),input(input),ffluxPhase(ffluxPhase),ffluxPhaseLimit(ffluxPhaseLimit),
 previousPhaseOutputPtr(NULL),cyclicCounter(0)
{
    // consistency check
    if (ffluxPhase.fflux_phase_id()!=0)
    {
        throw ConsistencyException("Forward Flux phase 0 version of FFluxTrajectoryList constructor called durring phase %d", ffluxPhase.fflux_phase_id());
    }

    // figure out how many trajectories we need to start right now
    uint64_t trajectoriesToStart = getTrajectoriesToStart(ffluxPhase, ffluxPhaseLimit, simultaneousTrajectoryCount);

    // initialize trajectories based on simulation input files
    for (uint64_t i=0;i<trajectoriesToStart;i++)
    {
        initFFluxPhaseZeroTrajectory(input, basin.species_count().begin(), basin.species_count().end(), 0.0, simulationPhaseIndex(), DEFAULT_TRAJECTORY_ID);
    }
}

// ffluxPhase n>0 constructor
FFluxTrajectoryList::FFluxTrajectoryList(uint64_t count, uint64_t newSimulationPhaseIndex, const FFluxPhase& ffluxPhase, const FFluxPhaseLimit& ffluxPhaseLimit, uint simultaneousTrajectoryCount, const FFluxInput& input, const FFluxPhaseOutputWrap& previousPhaseOutput)
:TrajectoryList(count, newSimulationPhaseIndex),input(input),ffluxPhase(ffluxPhase),ffluxPhaseLimit(ffluxPhaseLimit),
 previousPhaseOutputPtr(&previousPhaseOutput),cyclicCounter(0)
{
    // consistency check
    if (ffluxPhase.fflux_phase_id()==0) throw ConsistencyException("Forward Flux phase n>0 version of FFluxTrajectoryList constructor called durring phase 0. fflux_phase_id: %d", ffluxPhase.fflux_phase_id());

    // figure out how many trajectories we need to start right now
    uint64_t trajectoriesToStart = getTrajectoriesToStart(ffluxPhase, ffluxPhaseLimit, simultaneousTrajectoryCount);

    initTrajectories(trajectoriesToStart);
}

// ffluxPhase custom constructor
FFluxTrajectoryList::FFluxTrajectoryList(uint64_t count, uint64_t newSimulationPhaseIndex, const FFluxPhase& ffluxPhase, const FFluxPhaseLimit& ffluxPhaseLimit, uint simultaneousTrajectoryCount, const FFluxInput& input)
:TrajectoryList(count, newSimulationPhaseIndex),input(input),ffluxPhase(ffluxPhase),ffluxPhaseLimit(ffluxPhaseLimit),
previousPhaseOutputPtr(&previousPhaseOutputCustomWrap),cyclicCounter(0)
{
    previousPhaseOutputCustom.mutable_successful_trajectory_end_points()->CopyFrom(ffluxPhase.start_points());
    previousPhaseOutputCustomWrap.setWrappedMsg(&previousPhaseOutputCustom);

    // consistency check
    if (ffluxPhase.fflux_phase_id()==0)
    {
        throw ConsistencyException("Forward Flux phase custom version of FFluxTrajectoryList constructor called durring phase 0. fflux_phase_id: %d", ffluxPhase.fflux_phase_id());
    }

    // figure out how many trajectories we need to start right now
    uint64_t trajectoriesToStart = getTrajectoriesToStart(ffluxPhase, ffluxPhaseLimit, simultaneousTrajectoryCount);

    initTrajectories(trajectoriesToStart);
}

uint64_t FFluxTrajectoryList::getTrajectoriesToStart(const FFluxPhase& ffluxPhase, const FFluxPhaseLimit& ffluxPhaseLimit, uint simultaneousWorkUnits)
{
    uint64_t toStart;
    uint64_t simulataneousActiveTrajectories = simultaneousWorkUnits*ffluxPhase.batch_size();

    if (ffluxPhase.trajectory_generation()==FFPhaseEnums::EAGER)
    {
        // EAGER is only implemented for certain ffluxPhaseLimit.stop_condition() values
        if (ffluxPhaseLimit.stop_condition()==FFPhaseLimEnums::TRAJECTORY_COUNT or (ffluxPhaseLimit.stop_condition()==FFPhaseLimEnums::FORWARD_FLUXES and ffluxPhase.fflux_phase_id()==0))
        {
            if (ffluxPhaseLimit.has_events_per_trajectory())
            {
                // given that our trajectory limits are set up to observe x events per trajectory, run ceil(y/x) trajectories to ensure that we observe at least y events total
                toStart = (uint64_t)ceil(ffluxPhaseLimit.uvalue()/(double)ffluxPhaseLimit.events_per_trajectory());
            }
            else
            {
                // in this case assume that we want to observe the maximum number of events per trajectory, so just run enough trajectories for one "round" (ie one trajectory per work unit runner)
                toStart = simulataneousActiveTrajectories;
            }
        }
        else throw UnimplementedException("In Forward Flux phase %d, ffluxPhase.trajectory_generation()==EAGER is only implemented for certain ffluxPhaseLimit.stop_condition() values (ie those that let us calculate the necessary trajectory count up front). Attempting to use unimplemented ffluxPhaseLimit.stop_condition(): %s", ffluxPhase.fflux_phase_id(), FFPhaseLimEnums::StopCondition_Name(ffluxPhaseLimit.stop_condition()).c_str());
    }
    else if (ffluxPhase.trajectory_generation()==FFPhaseEnums::LAZY)
    {
        toStart = simulataneousActiveTrajectories;
    }
    else throw UnimplementedException("unimplemented");

    return toStart;
}

void FFluxTrajectoryList::workUnitPartFinished(const message::WorkUnitStatus& wusMsg, lm::trajectory::Trajectory* traj)
{
    // Call the base class method.
    lm::trajectory::TrajectoryList::workUnitPartFinished(wusMsg, traj);

    // If the work unit stopped because it hit a terminating limit...
    if (wusMsg.status()==lm::message::WorkUnitStatus::LIMIT_REACHED)
    {
        // FFluxSupervisor will have already extracted the necessary information, so we need to dispose of the trajectory here
        if (ffluxPhase.trajectory_generation()==FFPhaseEnums::LAZY)
        {
            // If the phase "plan" calls for it, generate a replacement trajectory by recycling the old one
            recycleFFluxTrajectory(traj);
        }
        else
        {

//            initTrajectories(1);
            // otherwise just delete the trajectory
            deleteTrajectory(traj->getID());
        }
    }
    else if (ffluxPhase.fflux_phase_id()==0)
    {
        // clear out any limit tracking time series data. prevents a major slowdown on long runs
        limitTrackingListWrap.setWrappedMsg(traj->getStateMutable()->mutable_limit_tracking_list());
        limitTrackingListWrap.clearTimeSeriesData();
    }
}

void FFluxTrajectoryList::initTrajectories(uint64_t trajectoriesToStart)
{
    switch(ffluxPhase.trajectory_duplication())
    {
    case FFPhaseEnums::NONE:
        // do nothing
        break;
    case FFPhaseEnums::CYCLIC:
        // initialize trajectories by cycling through EndPoints from a previous phase
        initTrajectoriesCyclic(trajectoriesToStart);
        break;
    case FFPhaseEnums::UNIFORM_RANDOM:
        // initialize trajectories based on randomly selected EndPoints from a previous phase
        initTrajectoriesUniformRandom(trajectoriesToStart);
        break;
    default: throw UnimplementedException("Unimplemented");
    }
}

void FFluxTrajectoryList::recycleFFluxTrajectory(lm::trajectory::Trajectory* traj)
{
    limitTrackingListWrap.setWrappedMsg(traj->getStateMutable()->mutable_limit_tracking_list());
    limitTrackingListWrap.clearStateData();

    traj->clearLimitReached();

    switch(ffluxPhase.trajectory_duplication())
    {
    case FFPhaseEnums::NONE:
        // do nothing
        break;
    case FFPhaseEnums::CYCLIC:
        // initialize trajectories by cycling through EndPoints from a previous phase
        recycleTrajectoryCyclic(traj->getID());
        break;
    case FFPhaseEnums::UNIFORM_RANDOM:
        // initialize trajectories based on randomly selected EndPoints from a previous phase
        recycleTrajectoryUniformRandom(traj->getID());
        break;
    default: throw UnimplementedException("Unimplemented");
    }
}

void FFluxTrajectoryList::initTrajectoriesCyclic(uint64_t trajectoriesToStart)
{
    for (uint64_t i=0;i<trajectoriesToStart;i++)
    {
        const lm::protowrap::EndPointVector::Pair& endPointPair(previousPhaseOutputPtr->getEndPointCyclic(cyclicCounter++));
        initFFluxTrajectory(input, endPointPair.first->species_coordinates().begin(), endPointPair.first->species_coordinates().end(), endPointPair.first->times(endPointPair.second), simulationPhaseIndex(), DEFAULT_TRAJECTORY_ID);
    }
}

void FFluxTrajectoryList::initTrajectoriesUniformRandom(uint64_t trajectoriesToStart)
{
    for (uint64_t i=0;i<trajectoriesToStart;i++)
    {
        PROF_BEGIN(PROF_TRAJECTORY_LIST_WORK_UNIT_FINISHED_ONE);
        const lm::protowrap::EndPointVector::Pair& endPointPair(previousPhaseOutputPtr->getEndPointUniformRandom());
        PROF_END(PROF_TRAJECTORY_LIST_WORK_UNIT_FINISHED_ONE);

        PROF_BEGIN(PROF_TRAJECTORY_LIST_WORK_UNIT_FINISHED_TWO);
        initFFluxTrajectory(input, endPointPair.first->species_coordinates().begin(), endPointPair.first->species_coordinates().end(), endPointPair.first->times(endPointPair.second), simulationPhaseIndex(), DEFAULT_TRAJECTORY_ID);
        PROF_END(PROF_TRAJECTORY_LIST_WORK_UNIT_FINISHED_TWO);
    }
}

lm::trajectory::Trajectory* FFluxTrajectoryList::recycleTrajectoryCyclic(uint64_t oldID)
{
    // choose an endpoint (by cycling through the list of endpoints) from the previous phase to use as a starting state
    const lm::protowrap::EndPointVector::Pair& endPointPair(previousPhaseOutputPtr->getEndPointCyclic(cyclicCounter++));

    // reuse as much of the existing trajectory as possible
    lm::trajectory::Trajectory* recycTraj = recycleTrajectory(endPointPair.first->species_coordinates().begin(), endPointPair.first->species_coordinates().end(), endPointPair.first->times(endPointPair.second), oldID, DEFAULT_TRAJECTORY_ID);

    // record the correct "initial" state in the recycled trajectory
    static_cast<lm::fflux::FFluxTrajectory*>(recycTraj)->setInitialStateToCurrentState();

    // return the refurbished trajectory
    return recycTraj;
}
lm::trajectory::Trajectory* FFluxTrajectoryList::recycleTrajectoryUniformRandom(uint64_t oldID)
{
    // choose an endpoint (at random from the list of endpoints) from the previous phase to use as a starting state
    const lm::protowrap::EndPointVector::Pair& endPointPair(previousPhaseOutputPtr->getEndPointUniformRandom());

    // reuse as much of the existing trajectory as possible
    lm::trajectory::Trajectory* recycTraj = recycleTrajectory(endPointPair.first->species_coordinates().begin(), endPointPair.first->species_coordinates().end(), endPointPair.first->times(endPointPair.second), oldID, DEFAULT_TRAJECTORY_ID);

    // record the correct "initial" state in the recycled trajectory
    static_cast<lm::fflux::FFluxTrajectory*>(recycTraj)->setInitialStateToCurrentState();

    // return the refurbished trajectory
    return recycTraj;
}

}
}