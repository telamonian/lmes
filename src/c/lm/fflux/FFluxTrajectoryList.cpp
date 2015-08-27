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
#include <functional>
#include <list>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include "lm/Exceptions.h"
#include "lm/fflux/FFluxTrajectoryList.h"
#include "lm/io/CMEState.pb.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/FFluxOutput.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/main/Main.h"
#include "lm/Print.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/tiling/Tilings.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using lm::io::DiffusionModel;
using lm::io::ReactionModel;
using std::map;
using std::string;
using std::vector;

namespace lm {
namespace fflux {

// setup directionString for printing the name of the current simulation direction
vector<string> MakeDirectionStrings()
{
    vector<string> directionStrings;
    directionStrings.push_back("FORWARD");
    directionStrings.push_back("BACKWARD");
    return directionStrings;
}
const vector<string> FFluxTrajectoryList::directionStrings = MakeDirectionStrings();

// these typedefs have to be here in .cpp because Direction is part of the FFluxTrajectoryList definition
typedef map<lm::fflux::FFluxTrajectoryList::Direction, CrossingsMap> CrossingsMapMap;
typedef map<lm::fflux::FFluxTrajectoryList::Direction, DwellTimeMap> DwellTimeMapMap;
typedef map<lm::fflux::FFluxTrajectoryList::Direction, FinishedTrajectoriesCountMap> FinishedTrajectoriesCountMapMap;

FFluxTrajectoryList::FFluxTrajectoryList(lm::message::Communicator& communicator, uint64_t simultaneousTrajectoryCount,lm::input::Input& input)
:TrajectoryList(input),
 crossingsPerPhase(atof(input.simulationParametersMap["crossingsPerPhase"].c_str())),
 direction(FORWARD),
 dwellTimes(),
 ffluxPhase(0),
 ffluxOutputQueueSize(1e4),
 finishedTrajectoriesCounts(),
 maxFFluxPhase(input.tilings.getCurrentTiling()->getEdgesCount()),
 maxPhaseZeroTime(atof(input.simulationParametersMap["maxPhaseZeroTime"].c_str())),
 simultaneousTrajectoryCount(simultaneousTrajectoryCount),
 xorShift(0,0)  //the rng object xorShift uses the current time as a seed when given 0,0 as constructor arguments
{
    setCommunicator(communicator);
    init();
}

FFluxTrajectoryList::~FFluxTrajectoryList()
{
    // TODO: for the sake of this damn destructor, if for nothing else, I'm going to tear down the CrossingsMapMap stuff and replace it with something less obstinate
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
	initFFluxOutput();
    initTrajectories(simultaneousTrajectoryCount);
    averageTilingHist.set_tiling_id(input.tilings.getCurrentTiling()->getID());
    for (lm::tiling::EdgeIterator e_it=input.tilings.getCurrentTiling()->begin();e_it!=input.tilings.getCurrentTiling()->end();e_it++)
    {
        averageTilingHist.add_tile_vals(0);
    }
    dwellTimes[-1] = 0;
    dwellTimes[0] = 0;
    finishedTrajectoriesCounts[0] = 0;
}

// initialize variables related to fflux output
void FFluxTrajectoryList::initFFluxOutput()
{
    lm::io::FFluxOutput::TrajectoryOutput* trajectoryOutput;
    lm::io::FFluxOutput::BasinOutput* basinOutput;
    lm::io::FFluxOutput::FinalOutput* finalOutput;

    // initialize the FFluxOutput part of the non-streaming/streaming member Messages
    lm::message::ProcessWorkUnitOutput* msgPWUO = msg.add_process_work_unit_output();
    msgPWUO->set_work_unit_id(999999999999998);

    lm::message::ProcessWorkUnitOutput* msgStreamingPWUO = msgStreaming.add_process_work_unit_output();
    msgStreamingPWUO->set_work_unit_id(999999999999999);

	getFFluxOutput()->set_tiling_id(input.tilings.getCurrentTilingID());
	getFFluxOutput()->set_number_tiles(maxFFluxPhase + 1);
	getFFluxOutput()->set_number_species(input.reactionModelBuf.number_species());

    getFFluxOutputStreaming()->set_tiling_id(input.tilings.getCurrentTilingID());
    getFFluxOutputStreaming()->set_number_tiles(maxFFluxPhase + 1);
    getFFluxOutputStreaming()->set_number_species(input.reactionModelBuf.number_species());

	// setup 1 final_output entry
	finalOutput = getFFluxOutput()->mutable_final_output();
	for (int direc=0; direc!=2; direc++)
	{
	    // create 2 basin_outputs entries, one for each direction
	    basinOutput = getFFluxOutput()->add_basin_outputs();
        basinOutput->set_direction(static_cast<lm::io::FFluxOutput::Direction>(direc));
	    for (int lcycle=0; lcycle!=3; lcycle++)
	    {
	        /////// FIXME //////////

	        // create 6 trajectory_outputs entries, one for each combination of direction and lifecycle
            trajectoryOutput = getFFluxOutputStreaming()->add_trajectory_outputs();
            trajectoryOutput->set_direction(static_cast<lm::io::FFluxOutput::Direction>(direc));
            trajectoryOutput->set_lifecycle(static_cast<lm::io::FFluxOutput::Lifecycle>(lcycle));
	    }
	}
}

void FFluxTrajectoryList::initReversed() // TODO: need to verify that reactionModel has a reversed_initial_species_count field before running this method
{
    initTrajectories(simultaneousTrajectoryCount, true);
}

void FFluxTrajectoryList::initTrajectories(uint64_t trajectoriesToStart,bool reversed)
{
    for (long long i=0; i<trajectoriesToStart; i++)
    {
    	lm::fflux::FFluxTrajectory* newTraj = new lm::fflux::FFluxTrajectory(trajectoryCount,ffluxPhase,input,reversed);
    	if (intermediateOutputFlag) {ffluxOutputAddTrajectory(newTraj, lm::io::FFluxOutput::INITIAL);}
    	trajectories[trajectoryCount] = newTraj;
        trajectoryCount++;
    }
}

void FFluxTrajectoryList::initTrajectories(uint64_t trajectoriesToStart, lm::io::TrajectoryState* oldTraj)
{
    for (long long i=0; i<trajectoriesToStart; i++)
    {
    	lm::fflux::FFluxTrajectory* newTraj = new lm::fflux::FFluxTrajectory(trajectoryCount,ffluxPhase,input,oldTraj);
    	if (intermediateOutputFlag) {ffluxOutputAddTrajectory(newTraj, lm::io::FFluxOutput::INITIAL);}
    	trajectories[trajectoryCount] = newTraj;
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
    PROF_BEGIN(PROF_FFLUX_WORK_UNIT_FINISHED);
    lm::fflux::FFluxTrajectory* traj = static_cast<lm::fflux::FFluxTrajectory*>(getTrajectory(finishedWorkUnitMsg.final_state().trajectory_id()));
    double prevTime = traj->getSimTime();
    uint prevFinalLimitID = traj->getFinalLimitID();
    // Call the base class method.
    traj = static_cast<lm::fflux::FFluxTrajectory*>(TrajectoryList::workUnitFinished(finishedWorkUnitMsg));
    // If the work unit was from a previous phase of the fflux simulation, delete the associated trajectory and move on
    if (traj->getFFluxPhase() < ffluxPhase)
    {
    	deleteTrajectory(traj->getID());
    }
    // If the work unit stopped because it detected a crossing event...]
    else if (finishedWorkUnitMsg.status()==lm::message::FinishedWorkUnit::LIMIT_REACHED)
    {
        // If the forward flux sampling is still in its 0th (ie initial) phase...
        if (isZerothPhase())
        {
            workUnitFinishedPhaseZero(finishedWorkUnitMsg, prevFinalLimitID, prevTime, traj);
        }
        // ...otherwise if ffluxPhase > 0...
        else
        {
            workUnitFinishedPhaseN(finishedWorkUnitMsg, prevFinalLimitID, prevTime, traj);
        }
    }
    PROF_END(PROF_FFLUX_WORK_UNIT_FINISHED);
    return traj;
}

lm::fflux::FFluxTrajectory* FFluxTrajectoryList::workUnitFinishedPhaseZero(const lm::message::FinishedWorkUnit& finishedWorkUnitMsg, uint prevFinalLimitID, double prevTime, lm::fflux::FFluxTrajectory* traj)
{
    PROF_BEGIN(PROF_FFLUX_WORK_UNIT_FINISHED_PHASE_ZERO);
    uint runnerIndex = finishedWorkUnitMsg.thread() + finishedWorkUnitMsg.process()*1000;
    // ...and if the crossing event was a forward flux...
    if (traj->fluxedForward() && traj->getFinalLimitID()==0)
    {
        // ...add the work unit's final state to the appropriate list of crossings
        Print::printf(Print::DEBUG,"Crossing %d added to phase %d list", crossings[ffluxPhase].size(), ffluxPhase);
        addCrossing(finishedWorkUnitMsg);
        dwellTimes[-1]+=traj->getSimTime() - traj->getLastLimitTime();
//        if (phaseZeroCrossings.find(runnerIndex)==phaseZeroCrossings.end())
//        {
//            phaseZeroCrossings[runnerIndex] = 0;
//            phaseZeroTimes[runnerIndex] = 0;
//        }
//        phaseZeroCrossings[runnerIndex] = phaseZeroCrossings[runnerIndex] + 1;
    }
    if (prevFinalLimitID==0)
    {
        dwellTimes[ffluxPhase]+=traj->getSimTime() - traj->getLastLimitTime();
//        phaseZeroTimes[runnerIndex]+=traj->getSimTime() - traj->getLastLimitTime();
    }
    // Regardless of whether this crossing was a forward or backwards flux, increment this phase's finished trajectories counter and dwell time, and delete the finished trajectory
    if (intermediateOutputFlag) {ffluxOutputAddTrajectory(traj, lm::io::FFluxOutput::FINAL);}
//        Print::printf(Print::INFO, "ffluxPhase: %d, crossings[fflux].size(): %d, finishedTrajectoriesCount %d, time: %f, oparam: %f", ffluxPhase, crossings[ffluxPhase].size(), finishedTrajectoriesCounts[ffluxPhase], crossings[ffluxPhase].back()->cme_state().species_counts().time(crossings[ffluxPhase].back()->cme_state().species_counts().number_entries() - 1), calcTestCaseOParam(finishedWorkUnitMsg.final_state()));
    // ...and if enough time has passed for phase zero to be complete...
//    if (isPhaseDone())
    if (isZerothPhaseDone(traj->getSimTime()))
//    if (isZerothPhaseDone(dwellTimes[ffluxPhase]))
    {
        // increment the finished trajectory count by the total number of phase zero trajectories (i.e. workUnitRunnerCount)
        finishedTrajectoriesCounts[0]+=simultaneousTrajectoryCount;
        deleteTrajectory(traj->getID());
        if (crossings.find(0)==crossings.end()) Print::printf(Print::ERROR, "No crossings were recorded during forward flux phase zero. Try increasing maxPhaseZeroTime");
        Print::printf(Print::INFO,"By the end of forward flux phase zero, %d forward crossings were recorded", crossings[ffluxPhase].size());
        Print::printf(Print::INFO,"The regional phase 0 dwell time is: %.10f", dwellTimes[0]);
        Print::printf(Print::INFO,"The basinal phase 0 dwell time is: %.10f", dwellTimes[-1]);
        Print::printf(Print::INFO,"The regional forward flux is: %.10f", crossings[ffluxPhase].size()/dwellTimes[0]);
        Print::printf(Print::INFO,"The basinal forward flux is: %.10f", crossings[ffluxPhase].size()/dwellTimes[-1]);
//        double carefulFlux = 0;
//        uint runnerCount = 0;
//        for (map<uint, uint>::iterator it = phaseZeroCrossings.begin();it!=phaseZeroCrossings.end();it++)
//        {
//            carefulFlux+=(it->second)/(phaseZeroTimes[it->first]);
//            runnerCount++;
//        }
//        carefulFlux/=runnerCount;
//        phaseZeroCrossings.clear(); phaseZeroTimes.clear();
//        Print::printf(Print::INFO,"The careful flux is: %.10f", carefulFlux);
        // ...delete any trajectories that have yet to start and mark the currently running set of trajectories as finished
        deleteAllNotStarted(); setAllFinished();
        // Next, increment the fflux phase counter. If there are still more phases to run...
        ++ffluxPhase;
        Print::printf(Print::INFO,"Forward flux phase %d:%s starting now", ffluxPhase, directionStrings[direction].c_str());
        if (!isFFluxDone())
        {
            // ...start up a new set of trajectories and make room to store their data
            dwellTimes[ffluxPhase] = 0;
            finishedTrajectoriesCounts[ffluxPhase] = 0;
            initPhaseNTrajectories(simultaneousTrajectoryCount);
        }
    }
    // ...otherwise we still have more time to go in phase zero...
    else
    {
        traj->setLastLimitTime(traj->getSimTime());
        setTrajectoryStatus(finishedWorkUnitMsg.final_state().trajectory_id(), lm::trajectory::Trajectory::WAITING);
//        deleteTrajectory(traj->getID());
//        // ...so start one phase zero trajectory.
//        initTrajectories(1, const_cast<lm::message::FinishedWorkUnit&>(finishedWorkUnitMsg).mutable_final_state()); // crossings[ffluxPhase].back());
//        initTrajectories(1, direction==FORWARD ? false : true);
    }
    PROF_END(PROF_FFLUX_WORK_UNIT_FINISHED_PHASE_ZERO);
    return traj;
}

lm::fflux::FFluxTrajectory* FFluxTrajectoryList::workUnitFinishedPhaseN(const lm::message::FinishedWorkUnit& finishedWorkUnitMsg, uint prevFinalLimitID, double prevTime, lm::fflux::FFluxTrajectory* traj)
{
    PROF_BEGIN(PROF_FFLUX_WORK_UNIT_FINISHED_PHASE_N);
    // ...and if the crossing event was a forward flux...
    if (traj->fluxedForward())
    {
        // ...add the work unit's final state to the appropriate list of crossings
        Print::printf(Print::DEBUG,"Crossing %d added to phase %d list", crossings[ffluxPhase].size(), ffluxPhase);
        addCrossing(finishedWorkUnitMsg);
    }
    // Regardless of whether this crossing was a forward or backwards flux, increment this phase's finished trajectories counter and dwell time, and delete the finished trajectory
    dwellTimes[ffluxPhase] += traj->getSimTime() - traj->getLastLimitTime();
    ++finishedTrajectoriesCounts[ffluxPhase];
    if (intermediateOutputFlag) {ffluxOutputAddTrajectory(traj, lm::io::FFluxOutput::FINAL);}
    deleteTrajectory(traj->getID());
//        Print::printf(Print::INFO, "ffluxPhase: %d, crossings[fflux].size(): %d, finishedTrajectoriesCount %d, time: %f, oparam: %f", ffluxPhase, crossings[ffluxPhase].size(), finishedTrajectoriesCounts[ffluxPhase], crossings[ffluxPhase].back()->cme_state().species_counts().time(crossings[ffluxPhase].back()->cme_state().species_counts().number_entries() - 1), calcTestCaseOParam(finishedWorkUnitMsg.final_state()));
    // ...and if enough crossing events have been detected for this phase of forward flux sampling...
    if (isPhaseDone())
    {
        // ...delete any trajectories that have yet to start and mark the currently running set of trajectories as finished
        deleteAllNotStarted(); setAllFinished();
        // Next, increment the fflux phase counter. If there are still more phases to run...
        ++ffluxPhase;
        if (!isFFluxDone())
        {
            // ...and start up a new set of trajectories
            dwellTimes[ffluxPhase] = 0;
            finishedTrajectoriesCounts[ffluxPhase] = 0;
            Print::printf(Print::INFO,"Forward flux phase %d:%s starting now", ffluxPhase, directionStrings[direction].c_str());
            initPhaseNTrajectories(simultaneousTrajectoryCount);
        }
        // ...otherwise if the whole simulation is complete, output some data.
        else
        {
            saveCrossings();
            saveDwellTimes();
            saveFinishedTrajectoriesCounts();

            if (intermediateOutputFlag) {ffluxOutputFinishTrajectory();}
            ffluxOutputAddBasin(crossings, dwellTimes, finishedTrajectoriesCounts);

            // If we have to run fflux sampling in both directions, check if we're on the forward phase...
            if (direction==FORWARD) // if (direction==FORWARD && bothDirections==TRUE)
            {
                // ...and if we are, reverse the arrangement of the edges and restart the simulation
                reverse();
                restart();
            }
            // If we're completely done with sampling in both directions, do the probability calculations
            else
            {
                ffluxOutputSetFinal_DinnerMethod(savedCrossings, savedDwellTimes, savedFinishedTrajectoriesCounts, savedHists);
                // if intermediateOutputFlag is not set, remove the basinOutputs from the final output message
                if (!(intermediateOutputFlag)) {getFFluxOutput()->clear_basin_outputs();}
//                // TODO this call to deleteAllTrajectories should not be necessary here, but there do seem to be a significant quantity of hangovers that stick around. Look into this
//                deleteAllTrajectories();
            }
        }
    }
    // ...otherwise we still need to collect more crossing events for this phase of forward flux sampling...
    else
    {
        // ...so start one phase N trajectory.
        initPhaseNTrajectories(1);
    }
    PROF_END(PROF_FFLUX_WORK_UNIT_FINISHED_PHASE_N);
    return traj;
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

lm::io::FFluxOutput* FFluxTrajectoryList::getFFluxOutput()
{
    return msg.mutable_process_work_unit_output(0)->mutable_fflux_output();
}

lm::io::FFluxOutput* FFluxTrajectoryList::getFFluxOutputStreaming()
{
    return msgStreaming.mutable_process_work_unit_output(0)->mutable_fflux_output();
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

bool FFluxTrajectoryList::isZerothPhaseDone(double simTime)
{
    return simTime>=maxPhaseZeroTime;
}

void FFluxTrajectoryList::reduceTilingHist(const lm::io::TilingHist& tHist)
{
}

void FFluxTrajectoryList::restart()
{
    deleteAllTrajectories();
    crossings.clear();
    dwellTimes.clear(); dwellTimes[0] = 0;
    finishedTrajectoriesCounts.clear(); finishedTrajectoriesCounts[0] = 0;
    //std::fill(finishedTrajectoriesCounts.begin(), finishedTrajectoriesCounts.end(), 0);
    std::fill(averageTilingHist.mutable_tile_vals()->begin(), averageTilingHist.mutable_tile_vals()->end(), 0);
    ffluxPhase = 0;
    this->initReversed();
}

void FFluxTrajectoryList::reverse()
{
    direction = direction==FORWARD ? BACKWARD : FORWARD;
    input.tilings.reverse();
}

void FFluxTrajectoryList::saveCrossings()
{

    savedCrossings.insert(CrossingsMapMap::value_type(direction, crossings));
}

void FFluxTrajectoryList::saveDwellTimes()
{
    savedDwellTimes.insert(DwellTimeMapMap::value_type(direction, dwellTimes));
}

void FFluxTrajectoryList::saveFinishedTrajectoriesCounts()
{
    savedFinishedTrajectoriesCounts.insert(FinishedTrajectoriesCountMapMap::value_type(direction, finishedTrajectoriesCounts));
}

void FFluxTrajectoryList::ffluxOutputAddBasin(CrossingsMap& crossings, DwellTimeMap& dwellTimes, FinishedTrajectoriesCountMap& finishedTrajectoriesCounts)
{
    // get a pointer to the germane BasinOutput buf
    lm::io::FFluxOutput::BasinOutput* basOut = getFFluxOutput()->mutable_basin_outputs(direction);

    // load the data into the BasinOutput buf pointer
    basOut->set_flux_out_of_tile_zero((double)crossings[0].size()/dwellTimes[0]);
//    basOut->set_flux_out_of_tile_zero((double)crossings[0].size()/(maxPhaseZeroTime*simultaneousTrajectoryCount));

    basOut->clear_probability_i_to_i_plus_one();
    lm::io::TilingHist* probabilityIToIPlusOne = basOut->mutable_probability_i_to_i_plus_one();
    probabilityIToIPlusOne->set_number_tiles(getFFluxOutput()->number_tiles());
    probabilityIToIPlusOne->set_tiling_id(getFFluxOutput()->tiling_id());

    probabilityIToIPlusOne->add_tile_indices(0);
    probabilityIToIPlusOne->add_tile_vals(0.0);
    for (int i=1;i<maxFFluxPhase;i++)
    {
        probabilityIToIPlusOne->add_tile_indices(i);
        probabilityIToIPlusOne->add_tile_vals((double)crossings[i].size()/finishedTrajectoriesCounts[i]);
    }
    probabilityIToIPlusOne->add_tile_indices(maxFFluxPhase);
    probabilityIToIPlusOne->add_tile_vals(0.0);

    basOut->clear_probability_one_to_i_plus_one();
    lm::io::TilingHist* probabilityOneToIPlusOne = basOut->mutable_probability_one_to_i_plus_one();
    probabilityOneToIPlusOne->set_number_tiles(getFFluxOutput()->number_tiles());
    probabilityOneToIPlusOne->set_tiling_id(getFFluxOutput()->tiling_id());

    probabilityOneToIPlusOne->add_tile_indices(0);
    probabilityOneToIPlusOne->add_tile_vals(0.0);
    probabilityOneToIPlusOne->add_tile_indices(1);
    probabilityOneToIPlusOne->add_tile_vals(probabilityIToIPlusOne->tile_vals(1));
    for (int i=2;i<maxFFluxPhase;i++)
    {
        probabilityOneToIPlusOne->add_tile_indices(i);
        probabilityOneToIPlusOne->add_tile_vals(probabilityOneToIPlusOne->tile_vals(i-1)*probabilityIToIPlusOne->tile_vals(i));
    }
    probabilityOneToIPlusOne->add_tile_indices(maxFFluxPhase);
    probabilityOneToIPlusOne->add_tile_vals(0.0);

    double probabilityOneToLast = probabilityOneToIPlusOne->tile_vals(maxFFluxPhase-1);
    double switchingRateConstant = basOut->flux_out_of_tile_zero()*probabilityOneToLast;
    basOut->set_switching_rate_constant(switchingRateConstant);
}

void FFluxTrajectoryList::ffluxOutputAddTrajectory(FFluxTrajectory* traj, lm::io::FFluxOutput::Lifecycle lifecycle)
{
	// get a number from 0-5 based on the current direction of the fflux simulation and the lifecycle of the trajectory being added
    // exciting bitshift version! (only works for 0-3)
	//uint outIndex = (direction!=FORWARD)<<1 | lifecycle!=lm::io::FFluxOutput::INITIAL;
    // more practical version
    uint outIndex = direction*3 + lifecycle;

//    getFFluxOutputStreaming()->set_tiling_id(input.tilings.getCurrentTilingID());
//    getFFluxOutputStreaming()->set_number_tiles(maxFFluxPhase - 1);
//    getFFluxOutputStreaming()->set_number_species(input.reactionModelBuf.number_species());

    // get a pointer to the germane TrajectoryOutput buf
    lm::io::FFluxOutput::TrajectoryOutput* trajOut = getFFluxOutputStreaming()->mutable_trajectory_outputs(outIndex);
//    trajOut->set_direction(static_cast<lm::io::FFluxOutput::Direction>(direction));
//    trajOut->set_lifecycle(static_cast<lm::io::FFluxOutput::Lifecycle>(lifecycle));

    // load the data into the TrajectoryOutput buf pointer
    trajOut->add_count(traj->getOPVal());
    if (lifecycle==lm::io::FFluxOutput::FINAL)
    {
        trajOut->add_edge_id(traj->getFinalLimitID());
    }
    else
    {
        trajOut->add_edge_id(traj->getFFluxPhase());
    }
    traj->getLastSpeciesCounts(trajOut);
    trajOut->add_time(traj->getSimTime());
    trajOut->add_trajectory_id(traj->getID());

    // adjust (statically) the fflux output queue tuning parameter. matches to 1/10 the size of the communicator's buffer and the size requirements of a single "row" in FFlux TrajectoryOutput
    ffluxOutputQueueSize = (10.0*1024.0*1024.0)/(8 + 8 + getFFluxOutputStreaming()->number_species()*4 + 8 + 8);

    // send a message to the output writer if a certain number of "rows" of trajectory data has accumulated                 // and if this is the final part of the current trajectory
    if (trajOut->time_size() > ffluxOutputQueueSize)    //*simultaneousTrajectoryCount)                                            // && lifecycle==lm::io::FFluxOutput::FINAL)
    {
        ffluxOutputFinishTrajectory();
        // adjust (dynamically) the fflux output queue tuning parameter
        //ffluxOutputQueueSize*=(10.0*1024.0*1024.0)/communicator->getLastMessageSize();
//        printf("ffluxOutputQueueSize: %d\n", ffluxOutputQueueSize);
    }
}

void FFluxTrajectoryList::ffluxOutputAddTrajectory(const lm::io::SpeciesCounts& specCountsMsg, lm::io::FFluxOutput::Lifecycle lifecycle)
{
    // get a number from 0-5 based on the current direction of the fflux simulation and the lifecycle of the trajectory being added
    uint outIndex = direction*3 + lifecycle;

    // lookup the trajectory associated with the data in specCountsMsg
    lm::fflux::FFluxTrajectory* traj = static_cast<FFluxTrajectory*>(getTrajectory(specCountsMsg.trajectory_id()));
    if (traj!=NULL)
    {
        // get a pointer to the germane TrajectoryOutput buf
        lm::io::FFluxOutput::TrajectoryOutput* trajOut = getFFluxOutputStreaming()->mutable_trajectory_outputs(outIndex);

        for (int i=0;i<specCountsMsg.number_entries();i++)
        {
            // load the data into the TrajectoryOutput buf pointer
            if (lifecycle==lm::io::FFluxOutput::FINAL)
            {
                trajOut->add_edge_id(traj->getFinalLimitID());
            }
            else
            {
                trajOut->add_edge_id(traj->getFFluxPhase());
            }
            trajOut->add_time(specCountsMsg.time(i));
            trajOut->add_trajectory_id(traj->getID());

            uint offset = i*(specCountsMsg.number_species());
            // FIXME: deal with the whole int vs uint thing in the line bellow
            trajOut->add_count(input.oparams[input.tilings.getCurrentTiling()->getOrderParameterID()]->calc((uint*)(specCountsMsg.species_count().data()) + offset, specCountsMsg.time(i)));
            for (int j=0; j<specCountsMsg.number_species(); j++)
            {
                trajOut->add_species_count(specCountsMsg.species_count(offset + j));
            }
        }

        // adjust (statically) the fflux output queue tuning parameter. matches to 1/10 the size of the communicator's buffer and the size requirements of a single "row" in FFlux TrajectoryOutput
        ffluxOutputQueueSize = (10.0*1024.0*1024.0)/(8 + 8 + getFFluxOutputStreaming()->number_species()*4 + 8 + 8);

        // send a message to the output writer if a certain number of "rows" of trajectory data has accumulated                 // and if this is the final part of the current trajectory
        if (trajOut->time_size() > ffluxOutputQueueSize)    //*simultaneousTrajectoryCount)                                            // && lifecycle==lm::io::FFluxOutput::FINAL)
        {
            ffluxOutputFinishTrajectory();
        }
    }
}

void FFluxTrajectoryList::ffluxOutputAddTrajectory(const lm::io::SpeciesTimeSeries& speciesTimeSeriesMsg, lm::io::FFluxOutput::Lifecycle lifecycle)
{
    // get a number from 0-5 based on the current direction of the fflux simulation and the lifecycle of the trajectory being added
    uint outIndex = direction*3 + lifecycle;

    int numberEntries = speciesTimeSeriesMsg.counts().shape(0);
    int numberSpecies = speciesTimeSeriesMsg.counts().shape(1);

    int32_t* counts = lm::io::hdf5::Hdf5File::dumpSpeciesCounts(speciesTimeSeriesMsg);
    double* times = lm::io::hdf5::Hdf5File::dumpSpeciesTimes(speciesTimeSeriesMsg);
//    lm::io::hdf5::Hdf5File::dumpSpeciesCountsAndTimes(speciesTimeSeriesMsg, counts, times);

    // lookup the trajectory associated with the data in specCountsMsg
    lm::fflux::FFluxTrajectory* traj = static_cast<FFluxTrajectory*>(getTrajectory(speciesTimeSeriesMsg.trajectory_id()));
    if (traj!=NULL)
    {
        // get a pointer to the germane TrajectoryOutput buf
        lm::io::FFluxOutput::TrajectoryOutput* trajOut = getFFluxOutputStreaming()->mutable_trajectory_outputs(outIndex);

        for (int i=0;i<numberEntries;i++)
        {
            // load the data into the TrajectoryOutput buf pointer
            if (lifecycle==lm::io::FFluxOutput::FINAL)
            {
                trajOut->add_edge_id(traj->getFinalLimitID());
            }
            else
            {
                trajOut->add_edge_id(traj->getFFluxPhase());
            }
            trajOut->add_time(times[i]);
            trajOut->add_trajectory_id(traj->getID());

            uint offset = i*(numberSpecies);
            // FIXME: deal with the whole int vs uint thing in the line bellow
            trajOut->add_count(input.oparams[input.tilings.getCurrentTiling()->getOrderParameterID()]->calc((uint*)counts + offset, times[i]));
            for (int j=0; j<numberSpecies; j++)
            {
                trajOut->add_species_count(counts[offset + j]);
            }
        }

        // adjust (statically) the fflux output queue tuning parameter. matches to 1/10 the size of the communicator's buffer and the size requirements of a single "row" in FFlux TrajectoryOutput
        ffluxOutputQueueSize = (10.0*1024.0*1024.0)/(8 + 8 + getFFluxOutputStreaming()->number_species()*4 + 8 + 8);

        // send a message to the output writer if a certain number of "rows" of trajectory data has accumulated
        if (trajOut->time_size() > ffluxOutputQueueSize)
        {
            ffluxOutputFinishTrajectory();
        }
    }

    // Free any allocated memory.
    if (speciesTimeSeriesMsg.counts().compressed_deflate())
        delete[] counts;
    if (speciesTimeSeriesMsg.times().compressed_deflate())
        delete[] times;
}

void FFluxTrajectoryList::ffluxOutputFinishTrajectory()
{
    communicator->sendMessageToMasterOutput(&msgStreaming);      //0,3, &msgStreaming);

    for (int i=0;i<3;i++)
    {
        // get a number from 0-5 based on the current direction of the fflux simulation and a lifecycle
        uint outIndex = direction*3 + i;
        lm::io::FFluxOutput::TrajectoryOutput* trajOut = getFFluxOutputStreaming()->mutable_trajectory_outputs(outIndex);
        // Clear the fflux output data
        trajOut->clear_count();
        trajOut->clear_edge_id();
        trajOut->clear_species_count();
        trajOut->clear_time();
        trajOut->clear_trajectory_id();
    }
}

void FFluxTrajectoryList::ffluxOutputPrintBasin(CrossingsMap& crossings, FinishedTrajectoriesCountMap& finishedTrajectoriesCounts)
{
    Print::printf(Print::INFO, "Phase 0 probability flux: %.10f", (double)crossings[0].size()/(maxPhaseZeroTime*simultaneousTrajectoryCount));
    for (int i=1;i<maxFFluxPhase;i++)
    {
        Print::printf(Print::INFO, "Probability of crossing from tile %d:%f to tile %d:%f : %.10f", i, input.tilings.getCurrentTiling()->getEdge(i), i+1, input.tilings.getCurrentTiling()->getEdge(i+1), (double)crossings[i].size()/finishedTrajectoriesCounts[i]);
    }
    double Kab = (double)crossings[0].size()/(maxPhaseZeroTime*simultaneousTrajectoryCount);
    for (int i=1;i<maxFFluxPhase;i++)
    {
        Kab *= (double)crossings[i].size()/finishedTrajectoriesCounts[i];
    }
    Print::printf(Print::INFO, "Switching rate constant: %.10f", Kab);
}

void FFluxTrajectoryList::ffluxOutputPrintFinal_DinnerMethod(SavedCrossings& savedCrossings, SavedDwellTimes& savedDwellTimes, SavedFinishedTrajectoriesCounts& savedFinishedTrajectoriesCounts, SavedHists& savedHists)
{
    // This version of the probability calculation is taken from Dinner, 2010
    double phaseZeroFluxA = (double)savedCrossings[FORWARD][0].size()/(maxPhaseZeroTime*simultaneousTrajectoryCount);
    double phaseZeroFluxB = (double)savedCrossings[BACKWARD][0].size()/(maxPhaseZeroTime*simultaneousTrajectoryCount);
    vector<double> paiaiplusone, pbibiplusone, pa0ai, pb0bi, pa0aiNormed, pb0biNormed;
    for (int i=0;i<maxFFluxPhase;i++)
    {
        paiaiplusone.push_back((double)savedCrossings[FORWARD][i].size()/savedFinishedTrajectoriesCounts[FORWARD][i]);
        pbibiplusone.push_back((double)savedCrossings[BACKWARD][i].size()/savedFinishedTrajectoriesCounts[BACKWARD][i]);
        pa0ai.push_back(paiaiplusone[1]);
        pb0bi.push_back(pbibiplusone[1]);
        for (int j=1;j<i;j++)
        {
            pa0ai[i]*=paiaiplusone[j];
            pb0bi[i]*=pbibiplusone[j];
        }
    }
//  std::partial_sum(paiaiplusone.begin(), paiaiplusone.end(), &pa0ai, std::multiplies<double>());
    double Kab = pa0ai.back();
    double Kba = pb0bi.back();
    double Pa = Kba/(Kab + Kba);
    double Pb = Kab/(Kab + Kba);
    double totalWeight = 0;
//  double totalProb = 0;
    for (int i=1;i<maxFFluxPhase;i++)
    {
        Print::printf(Print::INFO, "Forward tile %d probability: %.10f", i, Pa*phaseZeroFluxA*pa0ai[i]*savedDwellTimes[FORWARD][i]);
        totalWeight+=Pa*phaseZeroFluxA*pa0ai[i]*savedDwellTimes[FORWARD][i];
    }
    for (int i=1;i<maxFFluxPhase;i++)
    {
        Print::printf(Print::INFO, "Backward tile %d probability: %.10f", i, Pb*phaseZeroFluxB*pb0bi[i]*savedDwellTimes[BACKWARD][i]);
        totalWeight+=Pb*phaseZeroFluxB*pb0bi[i]*savedDwellTimes[BACKWARD][i];
    }
    for (int i=0;i<maxFFluxPhase;i++)
    {
        pa0aiNormed.push_back((Pa*phaseZeroFluxA*pa0ai[i]*savedDwellTimes[FORWARD][i])/totalWeight);
        pb0biNormed.push_back((Pb*phaseZeroFluxB*pb0bi[i]*savedDwellTimes[BACKWARD][i])/totalWeight);
    }
    Print::printf(Print::INFO, "Pa: %.10f", Pa);
    for (int i=1;i<maxFFluxPhase;i++)
    {
        Print::printf(Print::INFO, "Normalized tile %d probability: %.10f", i, pa0aiNormed[i]+pb0biNormed[maxFFluxPhase-i]);
    }
    Print::printf(Print::INFO, "Pb: %.10f", Pb);
}

void FFluxTrajectoryList::ffluxOutputSetFinal_DinnerMethod(SavedCrossings& savedCrossings, SavedDwellTimes& savedDwellTimes, SavedFinishedTrajectoriesCounts& savedFinishedTrajectoriesCounts, SavedHists& savedHists)
{
    // This version of the probability calculation is taken from Dinner, 2010
    // And then modified by me to be in terms of the tiles rather than the edges in between them

    // get a pointer to the FinalOutput buf
    lm::io::FFluxOutput::FinalOutput* finalOut = getFFluxOutput()->mutable_final_output();

    // some of the basin-associated data can't be calculated until both directions of the simulation, FORWARD and BACKWARD, are complete
    // so we calculate that data here rather than in ffluxOutputAddBasin and then load it into the BasinOutput bufs
    for (int direction=0; direction!=2; direction++)
    {
        // negate the current direction to get the opposite direction
        int oppositeDirection = !direction;
        // get pointers to the BasinOutputs corresponding to direction and oppositeDirection
        lm::io::FFluxOutput::BasinOutput* basinOut = getFFluxOutput()->mutable_basin_outputs(direction);
        lm::io::FFluxOutput::BasinOutput* oppositeBasinOut = getFFluxOutput()->mutable_basin_outputs(oppositeDirection);

        double thisBasinLastVisitedProbability = oppositeBasinOut->switching_rate_constant()/(basinOut->switching_rate_constant() + oppositeBasinOut->switching_rate_constant());
        basinOut->set_this_basin_last_visited_probability(thisBasinLastVisitedProbability);

        // initialize the weight variable, used below to calculate a normalized version of probability_i
        double weight = 0;
        basinOut->clear_probability_i();
        lm::io::TilingHist* probabilityI = basinOut->mutable_probability_i();
        probabilityI->set_number_tiles(getFFluxOutput()->number_tiles());
        probabilityI->set_tiling_id(getFFluxOutput()->tiling_id());

        probabilityI->add_tile_indices(0);
        probabilityI->add_tile_vals(0.0);
        for (int i=0;i<=maxFFluxPhase;i++) printf("basin: %s phase: %d dwell time: %.3f\n", directionStrings[direction].c_str(), i, (savedDwellTimes[static_cast<Direction>(direction)][i]/savedFinishedTrajectoriesCounts[static_cast<Direction>(direction)][i]));
        for (int i=1;i<maxFFluxPhase;i++)
        {
            probabilityI->add_tile_indices(i);
            probabilityI->add_tile_vals(basinOut->this_basin_last_visited_probability() *
                                        basinOut->flux_out_of_tile_zero() *
                                        basinOut->probability_one_to_i_plus_one().tile_vals(i) *
                                        (savedDwellTimes[static_cast<Direction>(direction)][i]/savedFinishedTrajectoriesCounts[static_cast<Direction>(direction)][i]));
            // keep a running sum of the values of probability_i in weight
            weight+=probabilityI->tile_vals(i);
        }
        probabilityI->add_tile_indices(maxFFluxPhase);
        probabilityI->add_tile_vals(0.0);

        basinOut->set_probability_i_weight(weight);

        basinOut->clear_normalized_probability_i();
        lm::io::TilingHist* normalizedProbabilityI = basinOut->mutable_normalized_probability_i();
        for (int i=0;i<probabilityI->tile_vals_size();i++)
        {
            normalizedProbabilityI->set_number_tiles(getFFluxOutput()->number_tiles());
            normalizedProbabilityI->set_tiling_id(getFFluxOutput()->tiling_id());
            normalizedProbabilityI->add_tile_indices(probabilityI->tile_indices(i));
            normalizedProbabilityI->add_tile_vals(probabilityI->tile_vals(i) /
                                                  basinOut->probability_i_weight());
        }
        // add the switching rate constant for this basin to the finalOut
        finalOut->add_switching_rate_constants(basinOut->switching_rate_constant());
    }

    // initialze some BasinOuput buf pointers to correspond to the FORWARD and BACKWARD BasinOutputs
    lm::io::FFluxOutput::BasinOutput* basinOut = getFFluxOutput()->mutable_basin_outputs(FORWARD);
    lm::io::FFluxOutput::BasinOutput* oppositeBasinOut = getFFluxOutput()->mutable_basin_outputs(BACKWARD);

    finalOut->clear_probability_i();
    lm::io::TilingHist* probabilityI = finalOut->mutable_probability_i();
    for (int i=0;i<basinOut->probability_i().tile_vals_size();i++)
    {
        probabilityI->set_number_tiles(getFFluxOutput()->number_tiles());
        probabilityI->set_tiling_id(getFFluxOutput()->tiling_id());
        probabilityI->add_tile_indices(i);
        probabilityI->add_tile_vals(basinOut->probability_i().tile_vals(i) +
                                    oppositeBasinOut->probability_i().tile_vals(oppositeBasinOut->probability_i().tile_vals_size() - 1 - i));
    }

    // weight for basin-independent probability_i is just the sum of the weights for basin-dependent probability_i
    double probabilityIWeight = basinOut->probability_i_weight() + oppositeBasinOut->probability_i_weight();

    finalOut->clear_normalized_probability_i();
    lm::io::TilingHist* normalizedProbabilityI = finalOut->mutable_normalized_probability_i();
    for (int i=0;i<probabilityI->tile_vals_size();i++)
    {
        normalizedProbabilityI->set_number_tiles(getFFluxOutput()->number_tiles());
        normalizedProbabilityI->set_tiling_id(getFFluxOutput()->tiling_id());
        normalizedProbabilityI->add_tile_indices(probabilityI->tile_indices(i));
        normalizedProbabilityI->add_tile_vals(probabilityI->tile_vals(i) /
                                              probabilityIWeight);
    }

    // add or remove some things from finalOutput depending on the status of intermediateOutputFlag
    if (intermediateOutputFlag) {finalOut->set_probability_i_weight(probabilityIWeight);}
    if (!(intermediateOutputFlag)) {normalizedProbabilityI->clear_tile_indices();}
}


}
}
