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
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <valarray>
#include <vector>

#include "lm/ClassFactory.h"
#include "lm/EnumHelper.h"
#include "lm/fflux/FFluxSupervisor.h"
#include "lm/fflux/FFluxTrajectoryList.h"
#include "lm/fflux/input/FFluxInput.h"
#include "lm/fflux/input/FFluxStage.pb.h"
#include "lm/fflux/input/FFluxPhaseLimit.pb.h"
#include "lm/fflux/FFluxMath.h"
#include "lm/input/Tilings.pb.h"
#include "lm/io/OutputWriter.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/main/Main.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Message.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/ProcessWorkUnitOutput.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/StartedOutputWriter.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/protowrap/Repeated.h"
#include "lm/Print.h"
#include "lm/resource/ResourceMap.h"
#include "lm/tiling/Tiling.h"

using lm::protowrap::Repeated;
using lm::resource::ResourceMap;
using std::map;
using std::setfill;
using std::setw;
using std::string;
using std::stringstream;
using std::valarray;
using std::vector;

namespace lm {
namespace fflux {

bool FFluxSupervisor::registered=FFluxSupervisor::registerClass();

bool FFluxSupervisor::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::main::SimulationSupervisor","lm::fflux::FFluxSupervisor",&FFluxSupervisor::allocateObject);
    return true;
}

void* FFluxSupervisor::allocateObject()
{
    return new FFluxSupervisor();
}

// if >0, we use a hand-rolled mpi receive polling scheme in order to reduce the supervisor cpu%
int FFluxSupervisor::getRecvSleepMilliseconds()
{
    return -1;
}

FFluxSupervisor::FFluxSupervisor()
:ffluxPhaseOutputsWrap(&ffluxPhaseOutputMsgs),previousFFluxPhaseOutputWrapPtr(&_ffluxPhaseOutputWrap_0),currentFFluxPhaseOutputWrapPtr(&_ffluxPhaseOutputWrap_1),
 ffluxStageOutputsWrap(&ffluxStageOutputMsgs),input(NULL),trajectoryList(NULL)
{
}

FFluxSupervisor::~FFluxSupervisor()
{
}

void FFluxSupervisor::init()
{
    // Initialize the FFluxInput pointer with the input file.
    setInput(new lm::fflux::input::FFluxInput(lm::io::hdf5::Hdf5File(simulationInputFilename)));
}

// overrides parent method completely
void FFluxSupervisor::startSimulation()
{
    initSimulationStageList();
    Print::printf(Print::INFO, "Simulation started.");

    // call the function which starts the simulation stage (which will then call startSimulationPhase())
    startSimulationStage();
}

void FFluxSupervisor::initSimulationStageList()
{
    // build the stage list
    for (lm::tiling::Tilings::const_iterator tilingIt=input->getTilings().begin();tilingIt!=input->getTilings().end();++tilingIt)
    {
        for (int basinIndex=0;basinIndex<tilingIt->second->basins().size();basinIndex++)
        {
            // initialize a stage (and possibly also its pilot stage)
            lm::fflux::input::FFluxStage* productionStage = buildProductionStage(ffluxStageListMsg.add_fflux_stages(), *tilingIt->second, basinIndex);

            // place a ptr to the stage in the execution order (the pilot stage ptr, if any, will be placed before the production stage pointer)
            ffluxStageExecutionOrder.push_back(productionStage);
        }
    }
    currentFFluxStageIter = ffluxStageExecutionOrder.begin();
}

lm::fflux::input::FFluxStage* FFluxSupervisor::buildProductionStage(lm::fflux::input::FFluxStage* productionStage, const lm::tiling::Tiling& tiling, int basinIndex)
{
    // TODO: encapsulate this mess in a FFluxStage wrapper
    // shallow copy the tiling wrapper. We're done with the passed in tiling wrapper
    lm::tiling::Tiling tilingWrapCopy = tiling;

    // copy the actual tiling message over to the production stage message
    productionStage->mutable_tiling()->CopyFrom(tilingWrapCopy.getTilingMsg());

    // reseat the tiling wrapper copy around the tiling message copy
    tilingWrapCopy.setTilingMsg(productionStage->mutable_tiling());

    // use the tiling wrapper copy to set the appropriate basin_index in the tiling. This will also reverse the tiling, if needed
    tilingWrapCopy.setBasin(basinIndex);

    productionStage->set_basin_index(basinIndex);

    addFFluxPhases(productionStage, FFPhaseEnums::LAZY, FFPhaseEnums::UNIFORM_RANDOM);

    if (input->hasPrecisionGoal())
    {
        // initialize the pilot stage
        lm::fflux::input::FFluxStage* pilotStage = addPilotStage(productionStage);

        // add the pilot stage to the execution order
        ffluxStageExecutionOrder.push_back(pilotStage);
    }
    else
    {
        addFFluxPhaseLimitsFromInput(productionStage);
    }

    return productionStage;
}

lm::fflux::input::FFluxStage* FFluxSupervisor::addPilotStage(lm::fflux::input::FFluxStage* productionStage)
{
    lm::fflux::input::FFluxStage* pilotStage = productionStage->mutable_pilot_stage();

    pilotStage->mutable_tiling()->CopyFrom(productionStage->tiling());
    pilotStage->set_basin_index(productionStage->basin_index());

    addFFluxPhases(pilotStage, FFPhaseEnums::LAZY, FFPhaseEnums::SIMPLE);

    addFFluxPhaseLimits(pilotStage, FFPhaseLimEnums::FORWARD_FLUXES, 10);

    return pilotStage;
}

void FFluxSupervisor::addFFluxPhases(lm::fflux::input::FFluxStage* stage, FFPhaseEnums::TrajectoryGeneration trajGeneration, FFPhaseEnums::TrajectoryDuplication trajDuplication)
{
    stringstream ss;
    ss << "/FFluxOutput/Tilings/" << setfill('0') << setw(7) << stage->tiling().id() << "/Basins/" << setfill('0') << setw(7) << stage->tiling().current_basin_index();
    input->reinitOutputOptions(ss.str());

    for (int i=0;i<stage->tiling().edges_size();i++)
    {
        lm::fflux::input::FFluxPhase* ffluxPhase = stage->add_fflux_phases();

        ffluxPhase->set_fflux_phase_index(i);
        ffluxPhase->set_basin_index(stage->basin_index());
        ffluxPhase->set_tiling_id(stage->tiling().id());

        ffluxPhase->set_trajectory_duplication(trajDuplication);

        ffluxPhase->mutable_output_options()->CopyFrom(input->getOutputOptionsMsg());

        ffluxPhase->set_batch_size(1);

        // set ffluxPhase values that depend on whether phaseIndex==0 or phaseIndex > 0
        if (i==0)
        {
            ffluxPhase->set_trajectory_generation(FFPhaseEnums::EAGER);
        }
        else
        {
            ffluxPhase->set_trajectory_generation(trajGeneration);
        }
    }
}

void FFluxSupervisor::startSimulationStage()
{
    Print::printf(Print::INFO, "Starting Forward Flux stage %d (tiling_id: %d, basin_index: %d).", currentStageIndex(), currentStage().tiling().id(), currentStage().tiling().current_basin_index());

    // set the current tiling to wrap the current stage's tiling msg
    setCurrentTiling(mutableCurrentStage()->mutable_tiling());

    // set the ffluxPhaseLimits for this stage, if it hasn't already been taken care of somehow
    if (currentStage().has_pilot_stage() and currentStage().fflux_phase_limits_size()==0)
    {
        addFFluxPhaseLimitsFromStageOutput(mutableCurrentStage(), currentStageOutput());
    }

    // add a new stage output
    addFFluxStageOutput();

    // set the first phase of the new stage as the currentFFluxPhase
    currentFFluxPhaseIter = mutableCurrentStage()->mutable_fflux_phases()->begin();

    // start the new phase
    startSimulationPhase();
}

template <typename T>
lm::fflux::input::FFluxPhaseLimit* FFluxSupervisor::buildFFluxPhaseLimit(lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit, FFPhaseLimEnums::StopCondition stopCondition, T value)
{
    ffluxPhaseLimit->set_stop_condition(stopCondition);

    switch (stopCondition)
    {
    case FFPhaseLimEnums::FORWARD_FLUXES: ffluxPhaseLimit->set_uvalue(value); break;
    case FFPhaseLimEnums::TRAJECTORY_COUNT: ffluxPhaseLimit->set_uvalue(value); break;
    case FFPhaseLimEnums::TIME: ffluxPhaseLimit->set_dvalue(value); break;
    }

    return ffluxPhaseLimit;
}

void FFluxSupervisor::buildFFluxPhaseLimitTrajectoriesToRun(lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit, const lm::fflux::input::FFluxPhase& ffluxPhase, uint simultaneousWorkUnits)
{
    uint64_t simulataneousActiveTrajectories = simultaneousWorkUnits*ffluxPhase.batch_size();

    // events_per_trajectory can be set before running this function
    if (not ffluxPhaseLimit->has_events_per_trajectory())
    {
        if (ffluxPhase.fflux_phase_index()==0)
        {
            if (ffluxPhaseLimit->stop_condition()==FFPhaseLimEnums::FORWARD_FLUXES)
            {
                ffluxPhaseLimit->set_events_per_trajectory(ceilDiv(ffluxPhaseLimit->uvalue(), simulataneousActiveTrajectories));
            }
            else throw UnimplementedException("ffluxPhaseLimit->stop_condition()==TIME, ==TRAJECTORY_COUNT currently unimplemented for fflux phase 0");
        }
        else
        {
            ffluxPhaseLimit->set_events_per_trajectory(1);
        }
    }

    if (ffluxPhase.trajectory_generation()==FFPhaseEnums::EAGER)
    {
        // EAGER is only implemented for certain ffluxPhaseLimit.stop_condition() values
        if (ffluxPhaseLimit->stop_condition()==FFPhaseLimEnums::TRAJECTORY_COUNT or (ffluxPhaseLimit->stop_condition()==FFPhaseLimEnums::FORWARD_FLUXES and ffluxPhase.fflux_phase_index()==0))
        {
            // given that our trajectory limits are set up to observe x events per trajectory, run ceil(y/x) trajectories to ensure that we observe at least y events total
            ffluxPhaseLimit->set_trajectories_per_phase(ceilDiv(ffluxPhaseLimit->uvalue(), ffluxPhaseLimit->events_per_trajectory()));
        }
        else throw UnimplementedException("In Forward Flux phase %d, ffluxPhase.trajectory_generation()==EAGER is only implemented for certain ffluxPhaseLimit.stop_condition() values (ie those that let us calculate the necessary trajectory count up front). Attempting to use unimplemented ffluxPhaseLimit.stop_condition(): %s", ffluxPhase.fflux_phase_index(), FFPhaseLimEnums::StopCondition_Name(ffluxPhaseLimit->stop_condition()).c_str());
    }
    else if (ffluxPhase.trajectory_generation()==FFPhaseEnums::LAZY)
    {
        return ffluxPhaseLimit->set_trajectories_per_phase(simulataneousActiveTrajectories);
    }
    else throw UnimplementedException("unimplemented");
}

template <typename T>
void FFluxSupervisor::addFFluxPhaseLimits(lm::fflux::input::FFluxStage* stage, FFPhaseLimEnums::StopCondition stopCondition, T value)
{
    for (FFluxPhases::const_iterator it=stage->fflux_phases().begin();it!=stage->fflux_phases().end();it++)
    {
        lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit = buildFFluxPhaseLimit(stage->add_fflux_phase_limits(), stopCondition, value);
        buildFFluxPhaseLimitTrajectoriesToRun(ffluxPhaseLimit, *it, slots.getSimultaneousWorkUnits());
    }
}

void FFluxSupervisor::addFFluxPhaseLimitsFromInput(lm::fflux::input::FFluxStage* productionStage)
{
    // TODO: implement manually specified ffluxPhaseLimits
    //productionStage->mutable_fflux_phase_limits()->CopyFrom(input->getFFluxPhaseLimits(productionStage->tiling().id(), productionStage->basin_index()));

    // temporary placeholder
    addFFluxPhaseLimits(productionStage, FFPhaseLimEnums::FORWARD_FLUXES, 1000);
}

void FFluxSupervisor::addFFluxPhaseLimitsFromStageOutput(lm::fflux::input::FFluxStage* productionStage, const lm::protowrap::FFluxStageOutputWrap& stageOutput, bool minimizeCost)
{
    vector<uint64_t> trajectoryCounts(optimizeTrajectoryCounts(input->precisionGoal(), input->precisionGoalConfidence(), stageOutput, minimizeCost));

    vector<uint64_t>::const_iterator tc_it=trajectoryCounts.begin();
    FFluxPhases::const_iterator ph_it=productionStage->fflux_phases().begin();

    // special treatment for phase zero
    lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit = buildFFluxPhaseLimit(productionStage->add_fflux_phase_limits(), FFPhaseLimEnums::FORWARD_FLUXES, *tc_it);
    buildFFluxPhaseLimitTrajectoriesToRun(ffluxPhaseLimit, *ph_it, slots.getSimultaneousWorkUnits());
    tc_it++, ph_it++;

    // all phases n>0
    for (;tc_it!=trajectoryCounts.end() and ph_it!=productionStage->fflux_phases().end();tc_it++, ph_it++)
    {
        lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit = buildFFluxPhaseLimit(productionStage->add_fflux_phase_limits(), FFPhaseLimEnums::TRAJECTORY_COUNT, *tc_it);
        buildFFluxPhaseLimitTrajectoriesToRun(ffluxPhaseLimit, *ph_it, slots.getSimultaneousWorkUnits());
    }
}

void FFluxSupervisor::repeatFFluxPhaseLimits(lm::fflux::input::FFluxStage* stage, const lm::fflux::input::FFluxPhaseLimit& limitToRepeat)
{
    // add copies of limitToRepeat for every ffluxPhase that's missing a corresponding ffluxPhaseLimit
    for (int i=stage->fflux_phase_limits_size();i<stage->fflux_phases_size();i++)
    {
        stage->add_fflux_phase_limits()->CopyFrom(limitToRepeat);
    }
}

vector<uint64_t> FFluxSupervisor::optimizeTrajectoryCounts(double precisionGoal, double precisionGoalConfidence, const lm::protowrap::FFluxStageOutputWrap& stageOutput, bool minimizeCost)
{
    vector<double> probabilities(stageOutput.probabilities().begin(), stageOutput.probabilities().end());

    vector<uint64_t> trajectoryCounts;
    if (minimizeCost)
    {
        vector<double> costVector(stageOutput.fluxes().begin(), stageOutput.fluxes().end());
        trajectoryCounts = minimizeCostTrajectoryCounts(precisionGoal, precisionGoalConfidence, probabilities, costVector);
    }
    else
    {
        trajectoryCounts = minimizeCountTrajectoryCounts(precisionGoal, precisionGoalConfidence, probabilities);
    }

    for (vector<uint64_t>::iterator it=trajectoryCounts.begin();it!=trajectoryCounts.end();it++) if (*it < 1000) *it=1000;
    return trajectoryCounts;
}

vector<uint64_t> FFluxSupervisor::minimizeCostTrajectoryCounts(double precisionGoal, double precisionGoalConfidence, const vector<double>& probabilities, const vector<double>& costVector)
{
    valarray<double> constantFactors(getConstantFactors(probabilities));
    valarray<double> costs(costVector.data(), costVector.size());
    costs = sqrt(costs);

    double coeff = pow(normalZ(precisionGoalConfidence)/precisionGoal, 2)*((costs*constantFactors).sum());
    constantFactors /= costs;
    constantFactors *= coeff;

    vector<uint64_t> trajectoryCounts;
    for (int i=0;i<constantFactors.size();i++)
    {
        trajectoryCounts.push_back(static_cast<uint64_t>(ceil(constantFactors[i])));
    }
    return trajectoryCounts;
}

vector<uint64_t> FFluxSupervisor::minimizeCountTrajectoryCounts(double precisionGoal, double precisionGoalConfidence, const vector<double>& probabilities)
{
    valarray<double> constantFactors(getConstantFactors(probabilities));

    constantFactors *= pow(normalZ(precisionGoalConfidence)/precisionGoal, 2)*(constantFactors.sum());

    vector<uint64_t> trajectoryCounts;
    for (int i=0;i<constantFactors.size();i++)
    {
        trajectoryCounts.push_back(static_cast<uint64_t>(ceil(constantFactors[i])));
    }
    return trajectoryCounts;
}

valarray<double> FFluxSupervisor::getConstantFactors(const vector<double>& probabilities)
{
    valarray<double> constantFactors(probabilities.data(), probabilities.size());
    constantFactors = (1.0 - constantFactors)/constantFactors;

    // ignore the probability from phase zero, store 1.0
    constantFactors[0] = 1.0;

    // take the square root
    constantFactors = sqrt(constantFactors);

    return constantFactors;
}

void FFluxSupervisor::receivedFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg)
{
    // deal with the individual parts of the work unit at the fflux supervisor level
    for (int i=0;i<msg.part_status_size();i++)
    {
        receivedFinishedWorkUnitPart(msg.part_status(i));
    }

    // call the base class function
    lm::main::SimulationSupervisor::receivedFinishedWorkUnit(msg);
}

void FFluxSupervisor::receivedFinishedWorkUnitPart(const lm::message::WorkUnitStatus& wusMsg)
{
    if (wusMsg.status()==lm::message::WorkUnitStatus::LIMIT_REACHED)
    {
        if (currentFFluxPhaseIndex()==0)
        {
            receivedFinishedWorkUnitPartPhaseZero(wusMsg);
        }
        else
        {
            currentFFluxPhaseOutputWrapPtr->addEndPoint(wusMsg.final_state());
        }
    }
}

void FFluxSupervisor::receivedFinishedWorkUnitPartPhaseZero(const lm::message::WorkUnitStatus& wusMsg)
{
    if (not trajectoryList->isTrajectoryAborted(wusMsg.final_state().trajectory_id()))
    {
        currentFFluxPhaseOutputWrapPtr->addEndPointPhaseZero(wusMsg.final_state(), input->ffluxOptions().phase_zero_burn_in_count());
    }
}

void FFluxSupervisor::startSimulationPhase()
{
    if (currentFFluxPhaseIndex()==0)
    {
        Print::printf(Print::INFO, "Starting Forward Flux phase %d (first_edge_value: %.2f).", currentFFluxPhaseIndex(), currentStage().tiling().edges(0));
    }
    else
    {
        Print::printf(Print::INFO, "Starting Forward Flux phase %d (starting_edge_value: %.2f, final_edge_value: %.2f).", currentFFluxPhaseIndex(), currentStage().tiling().edges(currentFFluxPhaseIndex() - 1), currentStage().tiling().edges(currentFFluxPhaseIndex()));
    }

    // add a new phase output
    addFFluxPhaseOutput();

    // call the base class method
    lm::main::SimulationSupervisor::startSimulationPhase();
}

void FFluxSupervisor::buildTrajectoryList()
{
    // if there is an old trajectoryList, get the trajectory count from that. Otherwise, we're at the very start of the simulation so count is 0
    uint64_t currentTrajectoryCount = trajectoryList != NULL ? trajectoryList->count() : 0;

//    // if there is an old trajectoryList, get the trajectory count from that and then delete it
//    if (trajectoryList != NULL)
//    {
//        currentTrajectoryCount = trajectoryList->count();
//
//        delete trajectoryList;
//        trajectoryList = NULL;
//        if (trajectoryList==NULL) printf("trajectory list is NULL\n");
//        else printf("trajectory list is not NULL\n");
//    }

    // set the trajectory limits/tracking for this phase
    input->reinitTrajectoryLimits(currentPhase(), currentPhaseLimit(), currentTiling());

    if (currentFFluxPhaseIndex()==0)
    {
        setTrajectoryList(new FFluxTrajectoryList(currentTrajectoryCount, currentFFluxPhaseIndex(), currentPhase(), currentPhaseLimit(), slots.getSimultaneousWorkUnits(), *input, currentTiling().currentBasin()));
    }
    else
    {
        setTrajectoryList(new FFluxTrajectoryList(currentTrajectoryCount, currentFFluxPhaseIndex(), currentPhase(), currentPhaseLimit(), slots.getSimultaneousWorkUnits(), *input, previousPhaseOutput()));
    }
}

bool FFluxSupervisor::terminateSimulationPhase()
{
    bool simulationPhaseTerminated;
    switch(mutableCurrentPhaseLimit()->stop_condition())
    {
    case FFPhaseLimEnums::FORWARD_FLUXES:
        simulationPhaseTerminated = (currentFFluxPhaseOutputWrapPtr->getMsg()->sucessful_trajectories_launched_count()>=mutableCurrentPhaseLimit()->uvalue());
        break;
    case FFPhaseLimEnums::TRAJECTORY_COUNT:
        simulationPhaseTerminated = (currentFFluxPhaseOutputWrapPtr->getMsg()->sucessful_trajectories_launched_count() + currentFFluxPhaseOutputWrapPtr->getMsg()->failed_trajectories_launched_count()>=mutableCurrentPhaseLimit()->uvalue());
        break;
    case FFPhaseLimEnums::TIME:
        simulationPhaseTerminated = (currentFFluxPhaseOutputWrapPtr->getMsg()->sucessful_trajectories_launched_total_time() + currentFFluxPhaseOutputWrapPtr->getMsg()->failed_trajectories_launched_total_time()>=mutableCurrentPhaseLimit()->dvalue());
        break;
    default: throw UnimplementedException("unimplemented");
    }

    if (simulationPhaseTerminated) simulationPhaseEverTerminated = true;
    return simulationPhaseTerminated;
}

void FFluxSupervisor::finishSimulationPhase()
{
    // send the phase output to the output writer
    // TODO: implement sending FFluxPhaseOutput to output writer

    // if we need to perform another phase, do so
    if (performAnotherSimulationPhase())
    {
        // initialize the next phase and switch over to it
        incrementSimulationPhase();
        startSimulationPhase();
    }
    // otherwise, finish up this stage of the simulation
    else
    {
        finishSimulationStage();
    }
}

void FFluxSupervisor::incrementSimulationPhase()
{
    // increment the currentFFluxPhase iterator
    currentFFluxPhaseIter++;

    // call the base class method
    lm::main::SimulationSupervisor::incrementSimulationPhase();
}

void FFluxSupervisor::addFFluxPhaseOutput()
{
    // hand the previous FFluxPhaseOutput message off to the storage list (if this isn't the first or second phase of the entire simulation)
    if (previousFFluxPhaseOutputWrapPtr->getMsg()!=NULL) ffluxPhaseOutputsWrap.AddAllocated(previousFFluxPhaseOutputWrapPtr->getMsg());

    // swap the subjects of the current and previous phase output wrapper pointers
    lm::protowrap::FFluxPhaseOutput* tmpFFluxPhaseOutputPtr = previousFFluxPhaseOutputWrapPtr;
    previousFFluxPhaseOutputWrapPtr = currentFFluxPhaseOutputWrapPtr;
    currentFFluxPhaseOutputWrapPtr = tmpFFluxPhaseOutputPtr;

    // add a new phase output and set it to be the current phase output
    currentFFluxPhaseOutputWrapPtr->setMsg(new lm::fflux::io::FFluxPhaseOutput);
//    * newPhaseOutputMsg = ffluxPhaseOutputsWrap.Add();
//
//    // set the new phase output to be the current phase output
//    currentFFluxPhaseOutputWrapPtr->setMsg(newPhaseOutputMsg);
}

void FFluxSupervisor::finishSimulationStage()
{
    // hand off the final ffluxPhaseOutputs to the repeated field wrapped by ffluxPhaseOutputsWrap
    if (previousFFluxPhaseOutputWrapPtr->getMsg()!=NULL) ffluxPhaseOutputsWrap.AddAllocated(previousFFluxPhaseOutputWrapPtr->getMsg());
    if (currentFFluxPhaseOutputWrapPtr->getMsg()!=NULL) ffluxPhaseOutputsWrap.AddAllocated(currentFFluxPhaseOutputWrapPtr->getMsg());

    // build the stage output from the phase outputs
    currentFFluxStageOutputWrap.buildFromFFluxPhaseOutputs(ffluxPhaseOutputsWrap);

    // send the stage output to the output writer
    // TODO: implement sending FFluxPhaseOutput to output writer

    // if we need to perform another stage, do so
    if (performAnotherSimulationStage())
    {
        // increment the stage-related iterators
        incrementSimulationStage();

        // start the new stage
        startSimulationStage();
    }
    // otherwise, stop the simulation
    else
    {
        finishSimulation();
    }
}

void FFluxSupervisor::incrementSimulationStage()
{
    // increment the currentFFluxPhase iterator
    currentFFluxStageIter++;
}

void FFluxSupervisor::addFFluxStageOutput()
{
    // add a new phase output
    lm::fflux::io::FFluxStageOutput* newStageOutputMsg = ffluxStageOutputsWrap.Add();

    // set the new phase output to be the current phase output
    currentFFluxStageOutputWrap.setWrappedMsg(newStageOutputMsg);
}

// setters
void FFluxSupervisor::setInput(lm::input::Input* newInput)
{
    lm::main::SimulationSupervisor::setInput(newInput);
    input = static_cast<lm::fflux::input::FFluxInput*>(lm::main::SimulationSupervisor::input);
}

void FFluxSupervisor::setTrajectoryList(lm::trajectory::TrajectoryList* newTrajectoryList)
{
    lm::main::SimulationSupervisor::setTrajectoryList(newTrajectoryList);
    trajectoryList = static_cast<lm::fflux::FFluxTrajectoryList*>(lm::main::SimulationSupervisor::trajectoryList);
}

}
}
