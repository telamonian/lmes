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
#include <cmath>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "lm/Exceptions.h"
#include "lm/fflux/FFluxTrajectoryList.h"
#include "lm/io/CMEState.pb.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/FFluxParameters.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/Print.h"
#include "lm/resource/Trajectory.h"

using std::map;
using std::string;
using std::vector;

namespace lm {
namespace fflux {

FFluxTrajectoryList::FFluxTrajectoryList(uint64_t simultaneousTrajectoryCount, map<string,string>& simulationParameters, const lm::io::ReactionModel& reactionModel, lm::tiling::Tilings& tilings):
    TrajectoryList(),simulationParameters(simulationParameters),reactionModel(reactionModel),tilings(tilings),xorShift(0,0),simultaneousTrajectoryCount(simultaneousTrajectoryCount),direction(FORWARD),ffluxPhase(0),crossingsPerPhase(1000),maxPhaseZeroTime(10000),maxFFluxPhase() // TODO: change maxFFluxPhase from fixed to varying with input //the rng object xorShift uses the current time as a seed when given 0,0 as constructor arguments
{
    maxFFluxPhase = tilings[0].getBorderCount();
    finishedTrajectoriesCounts = vector<long long>(maxFFluxPhase, 0);
}

FFluxTrajectoryList::~FFluxTrajectoryList()
{
    for (CrossingsMap::iterator mit=crossings.begin(); mit!=crossings.end(); mit++)
    {
        for (CrossingVector::iterator vit=mit->second.begin(); vit!=mit->second.end(); vit++)
        {
            delete *vit;
        }
    }
}

void FFluxTrajectoryList::init()
{
    lm::io::TrajectoryState* trajectoryState = initFirstTrajectoryState();
    for (long long i=0; i<=simultaneousTrajectoryCount; i++)
    {
        initTrajectory(trajectoryCount++, trajectoryState);
        // TODO: limit setting code
    }
    delete trajectoryState;
}

void FFluxTrajectoryList::initPhaseZeroTrajectory(lm::io::TrajectoryState* oldCrossing)
{
    // While still in the 0th phase of forward flux sampling, if a crossing event is detected and a trajectory stops, start a new trajectory from that same crossing event
    initTrajectory(trajectoryCount++, oldCrossing);
    // TODO: limit setting code
}

void FFluxTrajectoryList::initPhaseNTrajectories(uint64_t trajectoriesToStart, long long FFluxPhase)
{
    for (long long i=0; i<trajectoriesToStart; i++)
    {
        // Randomly choose a crossing state collected in the last round of fflux sampling, and use as the starting state for a new trajectory
        lm::io::TrajectoryState* randomCrossing = getRandomCrossing(FFluxPhase - 1);
        initTrajectory(trajectoryCount++, randomCrossing);
        // TODO: limit setting code
    }
}

void FFluxTrajectoryList::initTrajectory(uint64_t id, lm::io::TrajectoryState* state)
{
    // Construct new trajectory
    trajectories[id] = new lm::fflux::FFluxTrajectory(id, trajectoryTemplateMsg, state, tilings);

//    // Initialize the trajectory's runWorkUnit message
//    trajectories[id]->setMsg(trajectoryTemplateMsg);
//
//    // Copy the TrajectoryState referenced in the function args to the TrajectoryState of the newly constructed trajectory
//    trajectories[id]->setState(*state);
//
//    // Set the trajectory id in the trajectory state.
//    trajectories[id]->getState().set_trajectory_id(id);
//
//    // Set the trajectory id in the CME state of the trajectory state (if applicable).
//    if (trajectories[id]->getState().has_cme_state())
//        trajectories[id]->getState().mutable_cme_state()->mutable_species_counts()->set_trajectory_id(id);

    // Set the trajectory id in the RDME state of the trajectory state (if applicable).
//    if (trajectories[id]->getState().has_rdme_state())
//        trajectories[id]->getState().mutable_rdme_state()->mutable_species_counts()->set_trajectory_id(id);
}

lm::io::TrajectoryState* FFluxTrajectoryList::initFirstTrajectoryState()
{
    lm::io::TrajectoryState* trajectoryState = new lm::io::TrajectoryState();
    trajectoryState->mutable_cme_state()->mutable_species_counts()->set_number_species(reactionModel.number_species());
    trajectoryState->mutable_cme_state()->mutable_species_counts()->set_number_entries(1);
    for (int j=0; j<(int)reactionModel.number_species(); j++)
        trajectoryState->mutable_cme_state()->mutable_species_counts()->add_species_count(reactionModel.initial_species_count(j));
    trajectoryState->mutable_cme_state()->mutable_species_counts()->add_time(0.0);
    trajectoryState->mutable_fflux_state()->set_bin_id(0);
    trajectoryState->mutable_fflux_state()->set_interface_id(0);
    return trajectoryState;
}

void FFluxTrajectoryList::restart()
{
    deleteAllTrajectories();
    // TODO: need to check on reseting crossings

}

void FFluxTrajectoryList::reverse()
{
    direction = direction==FORWARD ? BACKWARD : FORWARD;
    tilings.reverse();
}

lm::fflux::FFluxTrajectory* FFluxTrajectoryList::workUnitFinished(const lm::message::FinishedWorkUnit & finishedWorkUnitMsg)
{
    // Call the base class method.
    lm::fflux::FFluxTrajectory* traj = TrajectoryList::workUnitFinished(finishedWorkUnitMsg);
//    Print::printf(Print::DEBUG, "finishedTrajectoryCount is: %d",finishedTrajectoriesCounts[ffluxPhase]);
    // If the work unit stopped because it detected a crossing event...]
    if (finishedWorkUnitMsg.status()==lm::message::FinishedWorkUnit::LIMIT_REACHED)
    {
        // ...and if the crossing event was a forward flux...
        if (traj->fluxedForward())
        {
            // ...add the work unit's final state to the appropriate list of crossings
            Print::printf(Print::INFO,"Crossing %d added to phase %d list", crossings[ffluxPhase].size(), ffluxPhase);
            addCrossing(finishedWorkUnitMsg);
        }
        // Regardless of whether this crossing was a forward or backwards flux, increment this phase's finished trajectories counter and delete the finished trajectory
        ++finishedTrajectoriesCounts[ffluxPhase];
        deleteTrajectory(finishedWorkUnitMsg.final_state().trajectory_id());
        //Print::printf(Print::INFO, "ffluxPhase: %d, crossings[fflux].size(): %d, finishedTrajectoriesCount %d, time: %f, oparam: %f", ffluxPhase, crossings[ffluxPhase].size(), finishedTrajectoriesCounts[ffluxPhase], crossings[ffluxPhase].back()->cme_state().species_counts().time(crossings[ffluxPhase].back()->cme_state().species_counts().number_entries() - 1), calcTestCaseOParam(finishedWorkUnitMsg.final_state()));
        // If the forward flux sampling is still in its 0th (ie initial) phase...
        if (isZerothPhase())
        {
            // ...and if enough time has passed for phase zero to be complete...
            if (traj->isZerothPhaseDone())
            {
                if (crossings.find(0)==crossings.end()) Print::printf(Print::ERROR, "No crossings were recorded during forward flux phase zero. Try increasing maxPhaseZeroTime");
                Print::printf(Print::INFO,"By the end of forward flux phase zero, %d forward crossings were recorded", crossings[ffluxPhase].size());
                // ...delete the currently running set of trajectories.
                deleteAllTrajectories();
                // Next, increment the fflux phase counter. If there are still more phases to run...
                ++ffluxPhase;
                if (!isFFluxDone())
                {
                    // ...increment the interface position (by altering the increasing/decreasing limits)...
//                    ratchetInterfaces();
                    // ...and start up a new set of trajectories
                    initPhaseNTrajectories(simultaneousTrajectoryCount,ffluxPhase);
                }
            }
            // ...otherwise we still have more time to go in phase zero...
            else
            {
                // ...so start one phase zero trajectory.
                initPhaseZeroTrajectory(crossings[ffluxPhase].back());
            }
        }
        // ...otherwise if ffluxPhase > 0...
        else
        {
            // ...and if enough crossing events have been detected for this phase of forward flux sampling...
            if (isPhaseDone())
            {
                // ...delete the currently running set of trajectories
                deleteAllTrajectories();
                // Next, increment the fflux phase counter. If there are still more phases to run...
                ++ffluxPhase;
                if (!isFFluxDone())
                {
                    // ...increment the interface position (by altering the increasing/decreasing limits)...
//                    ratchetInterfaces();
                    // ...and start up a new set of trajectories
                    initPhaseNTrajectories(simultaneousTrajectoryCount,ffluxPhase);
                }
                // ...otherwise if the whole simulation is complete, output some data.
                else
                {
                    Print::printf(Print::INFO, "Phase 0 probability flux: %.10f", (double)crossings[0].size()/(maxPhaseZeroTime*simultaneousTrajectoryCount));
                    for (int i=1;i<maxFFluxPhase;i++)
                    {
                        Print::printf(Print::INFO, "Crossing probability for interface at %f: %.10f", zerothInterface+i*oParamStep,(double)crossings[i].size()/finishedTrajectoriesCounts[i]);
                    }
                    double Kab = (double)crossings[0].size()/(maxPhaseZeroTime*simultaneousTrajectoryCount);
                    for (int i=1;i<maxFFluxPhase;i++)
                    {
                        Kab *= (double)crossings[i].size()/finishedTrajectoriesCounts[i];
                    }
                    Print::printf(Print::INFO, "Pseudo first order rate constant: %.10f", Kab);

                    // If we have to run fflux sampling in both directions, check if we're on the forward phase...
                    if (direction==FORWARD) // if (direction==FORWARD && bothDirections==TRUE)
                    {
                        // ...and if we are, reverse the arrangement of the binBorders and restart the simulation
                        reverse();
                        restart();
                    }
                }
            }
            // ...otherwise we still need to collect more crossing events for this phase of forward flux sampling...
            else
            {
                // ...so start one phase N trajectory.
                initPhaseNTrajectories(1, ffluxPhase);
            }
        }

    }
    return traj;
        //        *run.mutable_initial_state() = trajectories->getTrajectoryState(nextTrajectory);
        //        Print::printf(Print::INFO, "Sending message to start work unit %d with trajectory %d on slot %d:%d.", run.work_unit_id(), nextTrajectory, workSlot->getSlotKey()[0], workSlot->getSlotKey()[1]);
        //        communicator.sendMessage(workSlot->getSlotKey()[0], workSlot->getSlotKey()[1], &msg);
        //        trajectories->updateTrajectoryStatus(nextTrajectory, FFluxTrajectoryList::RUNNING);
}

lm::io::TrajectoryState * FFluxTrajectoryList::getRandomCrossing(long long ffluxPhase)
{
    unsigned i = floor(xorShift.getRandomDouble()*crossings[ffluxPhase].size());
    return crossings[ffluxPhase][i];
}

void FFluxTrajectoryList::addCrossing(const lm::message::FinishedWorkUnit& finishedWorkUnitMsg)
{
    lm::io::TrajectoryState * newCrossing = new lm::io::TrajectoryState(finishedWorkUnitMsg.final_state());
    crossings[ffluxPhase].push_back(newCrossing);
}

bool FFluxTrajectoryList::isZerothPhase()
{
    return (ffluxPhase==0);
}

bool FFluxTrajectoryList::isPhaseDone()
{
    return (crossings[ffluxPhase].size()>=crossingsPerPhase);
}

bool FFluxTrajectoryList::isFFluxDone()
{
    return (ffluxPhase < maxFFluxPhase);
}

//void FFluxTrajectoryList::initInterfaces()
//{
//    clearInterfaces();
//    for (tilingIterator iface_it=ffluxParams.interface().begin(); iface_it!=ffluxParams.interface().end(); ++iface_it)
//    {
//        for (uint i=0;i<iface_it->order_parameter_id_size();++i)
//        {
//            switch (iface_it->arrangement()) {
//            case lm::io::FFluxParameters::DECREASING:
//                setDecrInterface(iface_it->order_parameter_id(i), iface_it->bin_border(0));
//                break;
//            case lm::io::FFluxParameters::INCREASING:
//                setIncrInterface(iface_it->order_parameter_id(i), iface_it->bin_border(0));
//                break;
//            }
//        }
//    }
//}
//
//void FFluxTrajectoryList::ratchetInterfaces()
//{
//    // If this is running, ffluxPhase has just been incremented by one, so now also increment
//    clearInterfaces();
//    for (tilingIterator iface_it=ffluxParams.interface().begin(); iface_it!=ffluxParams.interface().end(); ++iface_it)
//    {
//        for (uint i=0;i<iface_it->order_parameter_id_size();++i)
//        {
//            switch (iface_it->arrangement()) {
//            case lm::io::FFluxParameters::DECREASING:
//                setInterface(iface_it->order_parameter_id(i), iface_it->bin_border(ffluxPhase-1), iface_it->bin_border(ffluxPhase));
//                break;
//            case lm::io::FFluxParameters::INCREASING:
//                setInterface(iface_it->order_parameter_id(i), iface_it->bin_border(ffluxPhase), iface_it->bin_border(ffluxPhase-1));
//                break;
//            }
//        }
//    }
//}
//
//void FFluxTrajectoryList::clearInterfaces()
//{
//    lm::message::RunWorkUnit* runWorkUnitMsg = getRunWorkUnitMsg();
//    runWorkUnitMsg->mutable_limits()->clear_decreasing_order_parameter_limit();
//    runWorkUnitMsg->mutable_limits()->clear_increasing_order_parameter_limit();
//    for (opIterator it=ffluxParams.order_parameter().begin(); it!=ffluxParams.order_parameter().end(); ++it)
//    {
//        lm::io::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = runWorkUnitMsg->mutable_limits()->add_decreasing_order_parameter_limit();
//        dopl->set_order_parameter_id(it->id());
//        lm::io::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = runWorkUnitMsg->mutable_limits()->add_increasing_order_parameter_limit();
//        iopl->set_order_parameter_id(it->id());
//    }
//}
//
//void FFluxTrajectoryList::setDecrInterface(uint opID, double decrLimit)
//{
//    lm::message::RunWorkUnit* runWorkUnitMsg = getRunWorkUnitMsg();
//    for (decrLimitIterator it=runWorkUnitMsg->mutable_limits()->decreasing_order_parameter_limit().begin(); it!=runWorkUnitMsg->mutable_limits()->decreasing_order_parameter_limit().end(); ++it)
//    {
//        if (it->order_parameter_id()==opID)
//        {
//            it->add_value(decrLimit);
//            goto end;
//        }
//    }
//    throw InvalidArgException("opID", "does not correspond to order parameter IDs in initialized limits");
//    end: ;
//}
//
//void FFluxTrajectoryList::setIncrInterface(uint opID, double incrLimit)
//{
//    lm::message::RunWorkUnit* runWorkUnitMsg = getRunWorkUnitMsg();
//    for (incrLimitIterator it=runWorkUnitMsg->mutable_limits()->increasing_order_parameter_limit().begin(); it!=runWorkUnitMsg->mutable_limits()->increasing_order_parameter_limit().end(); ++it)
//    {
//        if (it->order_parameter_id()==opID)
//        {
//            it->add_value(incrLimit);
//            goto end;
//        }
//    }
//    throw InvalidArgException("opID", "does not correspond to order parameter IDs in initialized limits");
//    end: ;
//}
//
//void FFluxTrajectoryList::setInterface(uint opID, double decrLimit, double incrLimit)
//{
//    setDecrInterface(opID, decrLimit);
//    setIncrInterface(opID, incrLimit);
//}

//// TEMP: replace
//double FFluxTrajectoryList::calcTestCaseOParam(const lm::io::TrajectoryState& finalState)
//    {
//        return (double)(finalState.cme_state().species_counts().species_count(3) + \
//               2*finalState.cme_state().species_counts().species_count(4) + \
//               2*finalState.cme_state().species_counts().species_count(5)) - \
//               (double)(finalState.cme_state().species_counts().species_count(0) + \
//               2*finalState.cme_state().species_counts().species_count(1) + \
//               2*finalState.cme_state().species_counts().species_count(2));
//    }
//
//void FFluxTrajectoryList::incrTestCaseLimits()
//{
//    lm::message::RunWorkUnit* runWorkUnitMsg = getRunWorkUnitMsg();
//    Print::printf(Print::INFO, "decr_limit: %f incr_limit: %f", zerothInterface, runWorkUnitMsg->limits().increasing_species_count(0) + oParamStep);
//    runWorkUnitMsg->mutable_limits()->set_decreasing_species_count(0, zerothInterface);
//    runWorkUnitMsg->mutable_limits()->set_increasing_species_count(0, runWorkUnitMsg->limits().increasing_species_count(0) + oParamStep);
//}
//// TEMP

}
}
