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

#include <cmath>
#include <list>
#include <map>
#include <string>
#include <vector>

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

FFluxTrajectoryList::FFluxTrajectoryList(uint64_t simultaneousTrajectoryCount, double zerothInterface, map<string,string>& simulationParameters, const lm::io::ReactionModel& reactionModel, const lm::io::FFluxParameters& ffluxParams):
    TrajectoryList(),simulationParameters(simulationParameters),reactionModel(reactionModel),ffluxParams(ffluxParams),xorShift(0,0),simultaneousTrajectoryCount(simultaneousTrajectoryCount),direction(FORWARD),ffluxPhase(0),crossingsPerPhase(1000),zerothInterface(zerothInterface),finalInterface(25.0),interfaceCount(12),maxPhaseZeroTime(10000),maxFFluxPhase(),oParamStep() // TODO: change maxFFluxPhase from fixed to varying with input //the rng object xorShift uses the current time as a seed when given 0,0 as constructor arguments
{
	maxFFluxPhase = ffluxParams.interface(0).bin_border_size();
	finishedTrajectoriesCounts = vector<long long>(maxFFluxPhase, 0);
	//// TEMP
	oParamStep = (double)(finalInterface - zerothInterface)/interfaceCount;
	//// TEMP
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
	trajectories[id] = new lm::resource::Trajectory(id);

	// Initialize the trajectory's runWorkUnit message
	trajectories[id]->setMsg(trajectoryTemplateMsg);

	// Copy the TrajectoryState referenced in the function args to the TrajectoryState of the newly constructed trajectory
	trajectories[id]->setState(*state);

	// Set the trajectory id in the trajectory state.
	trajectories[id]->getState().set_trajectory_id(id);

	// Set the trajectory id in the CME state of the trajectory state (if applicable).
	if (trajectories[id]->getState().has_cme_state())
		trajectories[id]->getState().mutable_cme_state()->mutable_species_counts()->set_trajectory_id(id);

	// Set the trajectory id in the RDME state of the trajectory state (if applicable).
//	if (trajectories[id]->getState().has_rdme_state())
//		trajectories[id]->getState().mutable_rdme_state()->mutable_species_counts()->set_trajectory_id(id);
}

lm::io::TrajectoryState* FFluxTrajectoryList::initFirstTrajectoryState()
{
	lm::io::TrajectoryState* trajectoryState = new lm::io::TrajectoryState();
	trajectoryState->mutable_cme_state()->mutable_species_counts()->set_number_species(reactionModel.number_species());
	trajectoryState->mutable_cme_state()->mutable_species_counts()->set_number_entries(1);
	for (int j=0; j<(int)reactionModel.number_species(); j++)
		trajectoryState->mutable_cme_state()->mutable_species_counts()->add_species_count(reactionModel.initial_species_count(j));
	trajectoryState->mutable_cme_state()->mutable_species_counts()->add_time(0.0);
	return trajectoryState;
}

void FFluxTrajectoryList::reset()
{

}

void FFluxTrajectoryList::reverse()
{

}

void FFluxTrajectoryList::workUnitFinished(const lm::message::FinishedWorkUnit & finishedWorkUnitMsg)
{
	// Call the base class method.
	TrajectoryList::workUnitFinished(finishedWorkUnitMsg);
//	Print::printf(Print::DEBUG, "finishedTrajectoryCount is: %d",finishedTrajectoriesCounts[ffluxPhase]);
	// If the work unit stopped because it detected a crossing event...]
	if (finishedWorkUnitMsg.status()==lm::message::FinishedWorkUnit::LIMIT_REACHED)
	{
		// ...and if the crossing event was a forward flux...
		if (calcTestCaseOParam(finishedWorkUnitMsg.final_state())>=getLimitsMsg()->increasing_species_count(0))
		{
			// ...add the work unit's final state to the appropriate list of crossings
			Print::printf(Print::INFO,"Crossing %d added to phase %d list", crossings[ffluxPhase].size(), ffluxPhase);
			lm::io::TrajectoryState * newCrossing = new lm::io::TrajectoryState(finishedWorkUnitMsg.final_state());
			crossings[ffluxPhase].push_back(newCrossing);
		}
		// Regardless of whether this crossing was a forward or backwards flux, increment this phase's finished trajectories counter and delete the finished trajectory
		++finishedTrajectoriesCounts[ffluxPhase];
		deleteTrajectory(finishedWorkUnitMsg.final_state().trajectory_id());
		//Print::printf(Print::INFO, "ffluxPhase: %d, crossings[fflux].size(): %d, finishedTrajectoriesCount %d, time: %f, oparam: %f", ffluxPhase, crossings[ffluxPhase].size(), finishedTrajectoriesCounts[ffluxPhase], crossings[ffluxPhase].back()->cme_state().species_counts().time(crossings[ffluxPhase].back()->cme_state().species_counts().number_entries() - 1), calcTestCaseOParam(finishedWorkUnitMsg.final_state()));
		// If the forward flux sampling is still in its 0th (ie initial) phase...
		if 	(ffluxPhase==0)
		{
			// ...and if enough time has passed for phase zero to be complete...
			if (crossings[ffluxPhase].back()->cme_state().species_counts().time(crossings[ffluxPhase].back()->cme_state().species_counts().number_entries() - 1) > maxPhaseZeroTime)
			{
				if (crossings.find(ffluxPhase)==crossings.end()) Print::printf(Print::ERROR, "No crossings were recorded during forward flux phase zero. Try increasing maxPhaseZeroTime");
				Print::printf(Print::INFO,"By the end of forward flux phase zero, %d forward crossings were recorded", crossings[ffluxPhase].size());
				// ...delete the currently running set of trajectories.
				deleteAllTrajectories();
				// Next, increment the fflux phase counter. If there are still more phases to run...
				++ffluxPhase;
				if (maxFFluxPhase > ffluxPhase)
				{
					// ...increment the interface position (by altering the increasing/decreasing limits)...
				    incrLimits();
					//// TEMP : replace
					//incrTestCaseLimits();
					//// TEMP
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
			if (crossings[ffluxPhase].size()>=crossingsPerPhase)
			{
				// ...delete the currently running set of trajectories
				deleteAllTrajectories();
				// Next, increment the fflux phase counter. If there are still more phases to run...
				++ffluxPhase;
				if (maxFFluxPhase > ffluxPhase)
				{
					// ...increment the interface position (by altering the increasing/decreasing limits)...
					//// TEMP : replace
					//incrTestCaseLimits();
					//// TEMP
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
					    direction=BACKWARD;
					    ffluxParams.interface(0).set_arrangement(ffluxParams.interface(0).arrangement()==lm::io::FFluxParameters::INCREASING ? lm::io::FFluxParameters::DECREASING : lm::io::FFluxParameters::INCREASING);

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

void FFluxTrajectoryList::setFFluxLimits(bool hasLow, double lowLimit, bool hasHigh, double highLimit)
{
    lm::message::RunWorkUnit* runWorkUnitMsg = getRunWorkUnitMsg();
    if (hasLow) runWorkUnitMsg->mutable_limits()->set_decreasing_species_count(0, lowLimit);
    if (hasHigh) runWorkUnitMsg->mutable_limits()->set_increasing_species_count(0, highLimit);
}

double FFluxTrajectoryList::oparam(const lm::io::TrajectoryState& finalState)
{
    switch (ffluxParams.order_parameter(0).type())
    {
    case 0:
        return oparamLinear(finalState);
    }
}

double FFluxTrajectoryList::oparamLinear(const lm::io::TrajectoryState& finalState)
{
    double ret = 0;
    for (int i=0;i<ffluxParams.order_parameter(0).species_id_size();++i)
    {
        ret += (double)(finalState.cme_state().species_counts().species_count(ffluxParams.order_parameter(0).species_id(i))
                      * ffluxParams.order_parameter(0).species_coefficient(i));
    }
    return ret;
}

void FFluxTrajectoryList::incrLimits()
{
    // If this is running, ffluxPhase has just been incremented by one, so now also increment
    lm::message::RunWorkUnit* runWorkUnitMsg = getRunWorkUnitMsg();
    double prevBorder, nextBorder;
    prevBorder = ffluxParams.interface(0).bin_border(ffluxPhase-1);
    nextBorder = ffluxParams.interface(0).bin_border(ffluxPhase);
    switch (ffluxParams.interface(0).arrangement()) {
    case lm::io::FFluxParameters::INCREASING:
        runWorkUnitMsg->mutable_limits()->set_decreasing_species_count(0, prevBorder);
        runWorkUnitMsg->mutable_limits()->set_increasing_species_count(0, nextBorder);
        break;
    case lm::io::FFluxParameters::DECREASING:
        runWorkUnitMsg->mutable_limits()->set_increasing_species_count(0, prevBorder);
        runWorkUnitMsg->mutable_limits()->set_decreasing_species_count(0, nextBorder);
        break;
    }
}

//// TEMP: replace
double FFluxTrajectoryList::calcTestCaseOParam(const lm::io::TrajectoryState& finalState)
    {
		return (double)(finalState.cme_state().species_counts().species_count(3) + \
			   2*finalState.cme_state().species_counts().species_count(4) + \
			   2*finalState.cme_state().species_counts().species_count(5)) - \
			   (double)(finalState.cme_state().species_counts().species_count(0) + \
			   2*finalState.cme_state().species_counts().species_count(1) + \
			   2*finalState.cme_state().species_counts().species_count(2));
    }

void FFluxTrajectoryList::incrTestCaseLimits()
{
	lm::message::RunWorkUnit* runWorkUnitMsg = getRunWorkUnitMsg();
	Print::printf(Print::INFO, "decr_limit: %f incr_limit: %f", zerothInterface, runWorkUnitMsg->limits().increasing_species_count(0) + oParamStep);
	runWorkUnitMsg->mutable_limits()->set_decreasing_species_count(0, zerothInterface);
	runWorkUnitMsg->mutable_limits()->set_increasing_species_count(0, runWorkUnitMsg->limits().increasing_species_count(0) + oParamStep);
}
//// TEMP

}
}
