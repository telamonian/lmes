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
#include <csignal>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "lm/Exceptions.h"
#include "lm/fflux/FFluxTrajectoryList.h"
#include "lm/io/CMEState.pb.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/Print.h"
#include "lm/resource/Trajectory.h"
#include "lm/tiling/Tilings.h"

using lm::io::DiffusionModel;
using lm::io::ReactionModel;
using std::map;
using std::string;
using std::vector;

namespace lm {
namespace fflux {

// this has to be here because Direction is part of the FFluxTrajectoryList definition
typedef map<lm::fflux::FFluxTrajectoryList::Direction, CrossingsMap> CrossingsMapMap;

FFluxTrajectoryList::FFluxTrajectoryList(uint64_t simultaneousTrajectoryCount,const ReactionModel& reactionModel,const DiffusionModel& diffusionModel, map<string,string>& simulationParameters, lm::tiling::Tilings& tilings)
:TrajectoryList(reactionModel, diffusionModel, simulationParameters),
 tilings(tilings),
 crossingsPerPhase(atof(simulationParameters["crossingsPerPhase"].c_str())),
 direction(FORWARD),
 ffluxPhase(0),
 finishedTrajectoriesCounts(tilings[0]->getEdgesCount(), 0),
 maxFFluxPhase(tilings[0]->getEdgesCount()),
 maxPhaseZeroTime(atof(simulationParameters["maxPhaseZeroTime"].c_str())),
 simultaneousTrajectoryCount(simultaneousTrajectoryCount),
 xorShift(0,0)  //the rng object xorShift uses the current time as a seed when given 0,0 as constructor arguments
{
    init();
}

FFluxTrajectoryList::~FFluxTrajectoryList()
{
    // TODO: for the sake of this damn destructor, if for nothing else, I'm going to tear down the CrossingsMapMap stuff and replace it with something less obstinate
    // free all of the memory used by the crossing member
//    for (CrossingsMap::iterator mit=crossings.begin();mit!=crossings.end();++mit)
//    {
//        for (CrossingVector::iterator vit=mit->second.begin();vit!=mit->second.end();++vit)
//        {
//            if (*vit!=NULL)
//            {
//                //raise(SIGINT);
//                delete *vit;
//                *vit=NULL;
//            }
//        }
//    }
    // free all of the memory used by the savedCrossings member
    for (CrossingsMapMap::iterator mvit=savedCrossings.begin();mvit!=savedCrossings.end();++mvit)
    {
        for (CrossingsMap::iterator mit=mvit->second.begin();mit!=mvit->second.end();++mit)
        {
            for (CrossingVector::iterator vit=mit->second.begin();vit!=mit->second.end();++vit)
            {
                if (*vit!=NULL)
                {
                    delete *vit;
                    *vit=NULL;
                }
            }
        }
    }
}

void FFluxTrajectoryList::init()
{
    initTrajectories(simultaneousTrajectoryCount);
}

void FFluxTrajectoryList::initTrajectories(uint64_t trajectoriesToStart)
{
    for (long long i=0; i<trajectoriesToStart; i++)
    {
        trajectories[trajectoryCount] = new lm::fflux::FFluxTrajectory(trajectoryCount,ffluxPhase,reactionModel,diffusionModel,simulationParameters,tilings);
        trajectoryCount++;
    }
}

void FFluxTrajectoryList::initTrajectories(uint64_t trajectoriesToStart, lm::io::TrajectoryState* zerothTraj)
{
    for (long long i=0; i<trajectoriesToStart; i++)
    {
        trajectories[trajectoryCount] = new lm::fflux::FFluxTrajectory(trajectoryCount,ffluxPhase,reactionModel,diffusionModel,simulationParameters,tilings,zerothTraj);
        trajectoryCount++;
    }
}

void FFluxTrajectoryList::initPhaseNTrajectories(uint64_t trajectoriesToStart)
{
    for (long long i=0; i<trajectoriesToStart; i++)
    {
        // Randomly choose a crossing state collected in the last round of fflux sampling, and use as the starting state for a new trajectory
        lm::io::TrajectoryState* randomCrossing = getRandomCrossing(ffluxPhase - 1);
        initTrajectories(1, randomCrossing);
    }
}

lm::fflux::FFluxTrajectory* FFluxTrajectoryList::workUnitFinished(const lm::message::FinishedWorkUnit & finishedWorkUnitMsg)
{
    // Call the base class method.
    lm::fflux::FFluxTrajectory* traj = static_cast<lm::fflux::FFluxTrajectory*>(TrajectoryList::workUnitFinished(finishedWorkUnitMsg));
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
            if (isZerothPhaseDone(traj))
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
                    initPhaseNTrajectories(simultaneousTrajectoryCount);
                }
            }
            // ...otherwise we still have more time to go in phase zero...
            else
            {
                // ...so start one phase zero trajectory.
                initTrajectories(1, crossings[ffluxPhase].back());
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
                    initPhaseNTrajectories(simultaneousTrajectoryCount);
                }
                // ...otherwise if the whole simulation is complete, output some data.
                else
                {
                    Print::printf(Print::INFO, "Phase 0 probability flux: %.10f", (double)crossings[0].size()/(maxPhaseZeroTime*simultaneousTrajectoryCount));
                    for (int i=1;i<maxFFluxPhase;i++)
                    {
                        Print::printf(Print::INFO, "Crossing probability for interface at %f: %.10f", tilings[0]->getEdge(i), (double)crossings[i].size()/finishedTrajectoriesCounts[i]);
                    }
                    double Kab = (double)crossings[0].size()/(maxPhaseZeroTime*simultaneousTrajectoryCount);
                    for (int i=1;i<maxFFluxPhase;i++)
                    {
                        Kab *= (double)crossings[i].size()/finishedTrajectoriesCounts[i];
                    }
                    Print::printf(Print::INFO, "Pseudo first order rate constant: %.10f", Kab);
                    saveCrossings();
                    // If we have to run fflux sampling in both directions, check if we're on the forward phase...
                    if (direction==FORWARD) // if (direction==FORWARD && bothDirections==TRUE)
                    {
                        // ...and if we are, reverse the arrangement of the edges and restart the simulation
                        reverse();
                        restart();
                    }
                }
            }
            // ...otherwise we still need to collect more crossing events for this phase of forward flux sampling...
            else
            {
                // ...so start one phase N trajectory.
                initPhaseNTrajectories(1);
            }
        }
    }
    return traj;
        //        *run.mutable_initial_state() = trajectories->getTrajectoryState(nextTrajectory);
        //        Print::printf(Print::INFO, "Sending message to start work unit %d with trajectory %d on slot %d:%d.", run.work_unit_id(), nextTrajectory, workSlot->getSlotKey()[0], workSlot->getSlotKey()[1]);
        //        communicator.sendMessage(workSlot->getSlotKey()[0], workSlot->getSlotKey()[1], &msg);
        //        trajectories->updateTrajectoryStatus(nextTrajectory, FFluxTrajectoryList::RUNNING);
}

// getters
CrossingVector FFluxTrajectoryList::getCrossings(long long ffluxPhase)
{
    return crossings[ffluxPhase];
}

uint FFluxTrajectoryList::getCrossingsPerPhase()
{
    return crossingsPerPhase;
}

long long FFluxTrajectoryList::getFFluxPhase()
{
    return ffluxPhase;
}

double FFluxTrajectoryList::getMaxPhaseZeroTime()
{
    return maxPhaseZeroTime;
}

lm::io::TrajectoryState* FFluxTrajectoryList::getRandomCrossing(long long ffluxPhase)
{
    unsigned i = floor(xorShift.getRandomDouble()*crossings[ffluxPhase].size());
    return crossings[ffluxPhase][i];
}


CrossingsMap FFluxTrajectoryList::getSavedCrossings(lm::fflux::FFluxTrajectoryList::Direction dir)
{
    return savedCrossings[dir];
}

// encapsulated inner loop functions
void FFluxTrajectoryList::addCrossing(const lm::message::FinishedWorkUnit& finishedWorkUnitMsg)
{
    lm::io::TrajectoryState* newCrossing = new lm::io::TrajectoryState(finishedWorkUnitMsg.final_state());
    crossings[ffluxPhase].push_back(newCrossing);
}

uint FFluxTrajectoryList::incrFFluxPhase()
{
    return ++ffluxPhase;
}

bool FFluxTrajectoryList::isFFluxDone()
{
    return (ffluxPhase>=maxFFluxPhase);
}

bool FFluxTrajectoryList::isPhaseDone()
{
    return (crossings[ffluxPhase].size()>=crossingsPerPhase);
}

bool FFluxTrajectoryList::isZerothPhase()
{
    return (ffluxPhase==0);
}

bool FFluxTrajectoryList::isZerothPhaseDone(lm::fflux::FFluxTrajectory* traj)
{
    return traj->hasElapsed(maxPhaseZeroTime);
}

void FFluxTrajectoryList::restart()
{
    deleteAllTrajectories();
    crossings.clear();
    ffluxPhase = 0;
    this->init();
}

void FFluxTrajectoryList::reverse()
{
    direction = direction==FORWARD ? BACKWARD : FORWARD;
    tilings.reverse();
}

void FFluxTrajectoryList::saveCrossings()
{

    savedCrossings.insert(CrossingsMapMap::value_type(direction, crossings));
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
//                setDecrInterface(iface_it->order_parameter_id(i), iface_it->edge(0));
//                break;
//            case lm::io::FFluxParameters::INCREASING:
//                setIncrInterface(iface_it->order_parameter_id(i), iface_it->edge(0));
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
//                setInterface(iface_it->order_parameter_id(i), iface_it->edge(ffluxPhase-1), iface_it->edge(ffluxPhase));
//                break;
//            case lm::io::FFluxParameters::INCREASING:
//                setInterface(iface_it->order_parameter_id(i), iface_it->edge(ffluxPhase), iface_it->edge(ffluxPhase-1));
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
