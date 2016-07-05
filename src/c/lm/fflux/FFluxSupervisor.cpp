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
#include <limits>
#include <map>
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
using std::string;
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
:currentTilingPtr(NULL),previousFFluxPhaseOutputPtr(&_ffluxPhaseOutput_0),currentFFluxPhaseOutputPtr(&_ffluxPhaseOutput_1),
 input(NULL),trajectoryList(NULL)
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
    for (Repeated<lm::input::Tiling>::const_iterator tilingIt=input->getTilingsMsg().tilings().begin();tilingIt!=input->getTilingsMsg().tilings().end();++tilingIt)
    {
        for (int basinIndex=0;basinIndex<tilingIt->basins_size();basinIndex++)
        {
            // initialize a stage (and possibly also its pilot stage)
            lm::fflux::input::FFluxStage* productionStage = buildProductionStage(ffluxStageList.add_fflux_stages(), *tilingIt, basinIndex);

            // place a ptr to the stage in the execution order (the pilot stage ptr, if any, will be placed before the production stage pointer)
            ffluxStageExecutionOrder.push_back(productionStage);
        }
    }

    currentFFluxStageIter = ffluxStageExecutionOrder.begin();
}

lm::fflux::input::FFluxStage* FFluxSupervisor::buildProductionStage(lm::fflux::input::FFluxStage* productionStage, const lm::input::Tiling& tiling, int basinIndex)
{
    productionStage->mutable_tiling()->CopyFrom(tiling);
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

    addFFluxPhaseLimits(pilotStage, FFPhaseLimEnums::FORWARD_FLUXES, 1000);

    return pilotStage;
}

void FFluxSupervisor::addFFluxPhases(lm::fflux::input::FFluxStage* stage, FFPhaseEnums::TrajectoryGeneration trajGeneration, FFPhaseEnums::TrajectoryDuplication trajDuplication)
{
    input->reinitOutputOptions("");

    for (int i=0;i<stage->tiling().edges_size();i++)
    {
        lm::fflux::input::FFluxPhase* ffluxPhase = stage->add_fflux_phases();

        ffluxPhase->set_fflux_phase_index(i);
        ffluxPhase->set_basin_index(stage->basin_index());
        ffluxPhase->set_tiling_id(stage->tiling().id());

        ffluxPhase->set_trajectory_duplication(trajDuplication);
        ffluxPhase->set_trajectory_generation(trajGeneration);

        ffluxPhase->mutable_output_options()->CopyFrom(input->getOutputOptionsMsg());
    }
}

void FFluxSupervisor::startSimulationStage()
{
    // set the ffluxPhaseLimits for this stage, if it hasn't already been taken care of somehow
    if (mutableCurrentStage()->has_pilot_stage() and mutableCurrentStage()->fflux_phase_limits_size()==0)
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
void FFluxSupervisor::buildFFluxPhaseLimit(lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit, FFPhaseLimEnums::StopCondition stopCondition, T value)
{
    ffluxPhaseLimit->set_stop_condition(stopCondition);
    switch (stopCondition)
    {
    case FFPhaseLimEnums::FORWARD_FLUXES: ffluxPhaseLimit->set_uvalue(value); break;
    case FFPhaseLimEnums::TRAJECTORY_COUNT: ffluxPhaseLimit->set_uvalue(value); break;
    case FFPhaseLimEnums::TIME: ffluxPhaseLimit->set_dvalue(value); break;
    }
}

template <typename T>
void FFluxSupervisor::addFFluxPhaseLimits(lm::fflux::input::FFluxStage* stage, FFPhaseLimEnums::StopCondition stopCondition, T value)
{
    for (int i=0;i<stage->fflux_phases_size();i++)
    {
        buildFFluxPhaseLimit(stage->add_fflux_phase_limits(), stopCondition, value);
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

void FFluxSupervisor::addFFluxPhaseLimitsFromInput(lm::fflux::input::FFluxStage* productionStage)
{
    // TODO: implement manually specified ffluxPhaseLimits
    //productionStage->mutable_fflux_phase_limits()->CopyFrom(input->getFFluxPhaseLimits(productionStage->tiling().id(), productionStage->basin_index()));

    // temporary placeholder
    addFFluxPhaseLimits(productionStage, FFPhaseLimEnums::FORWARD_FLUXES, 1000);
}

void FFluxSupervisor::addFFluxPhaseLimitsFromStageOutput(lm::fflux::input::FFluxStage* productionStage, const lm::protowrap::FFluxStageOutput& stageOutput, bool minimizeCost)
{
    vector<uint64_t> trajectoryCounts(optimizeTrajectoryCounts(input->precisionGoal(), input->precisionGoalConfidence(), stageOutput, minimizeCost));

    for (vector<uint64_t>::const_iterator it=trajectoryCounts.begin();it!=trajectoryCounts.end();it++)
    {
        buildFFluxPhaseLimit(productionStage->add_fflux_phase_limits(), FFPhaseLimEnums::TRAJECTORY_COUNT, *it);
    }
}

vector<uint64_t> FFluxSupervisor::optimizeTrajectoryCounts(double precisionGoal, double precisionGoalConfidence, const lm::protowrap::FFluxStageOutput& stageOutput, bool minimizeCost)
{
    vector<double> probabilities(stageOutput.probabilities().begin(), stageOutput.probabilities().end());

    if (minimizeCost)
    {
        vector<double> costVector(stageOutput.fluxes().begin(), stageOutput.fluxes().end());
        return minimizeCostTrajectoryCounts(precisionGoal, precisionGoalConfidence, probabilities, costVector);
    }
    else
    {
        return minimizeCountTrajectoryCounts(precisionGoal, precisionGoalConfidence, probabilities);
    }
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
            currentFFluxPhaseOutputPtr->addEndPoint(wusMsg.final_state());
        }
    }
}

void FFluxSupervisor::receivedFinishedWorkUnitPartPhaseZero(const lm::message::WorkUnitStatus& wusMsg)
{
    currentFFluxPhaseOutputPtr->addEndPointPhaseZero(wusMsg.final_state(), input->ffluxOptions().phase_zero_burn_in_count());
}

void FFluxSupervisor::startSimulationPhase()
{
    // add a new phase output
    addFFluxPhaseOutput();

    // set the trajectory limits/tracking for this phase
    input->reinitTrajectoryLimits(currentFFluxPhaseIndex(), currentTiling(), requiredFluxesPerTrajectory());

    // call the base class method
    lm::main::SimulationSupervisor::startSimulationPhase();
}

void FFluxSupervisor::buildTrajectoryList()
{
    uint64_t currentTrajectoryCount = 0;

    // if there is an old trajectoryList, get the trajectory count from that and then delete it
    if (trajectoryList != NULL)
    {
        currentTrajectoryCount = trajectoryList->count();

        delete trajectoryList;
        trajectoryList = NULL;
    }

    if (currentFFluxPhaseIndex()==0)
    {
        setTrajectoryList(new FFluxTrajectoryList(currentTrajectoryCount, currentFFluxPhaseIndex(), currentPhase(), currentPhaseLimit(), slots.getSimultaneousWorkUnits(), *input));
    }
    else
    {
        setTrajectoryList(new FFluxTrajectoryList(currentTrajectoryCount, currentFFluxPhaseIndex(), currentPhase(), currentPhaseLimit(), slots.getSimultaneousWorkUnits(), *input, previousPhaseOutput()));
    }
}

bool FFluxSupervisor::terminateSimulationPhase()
{
    switch(mutableCurrentPhaseLimit()->stop_condition())
    {
    case FFPhaseLimEnums::FORWARD_FLUXES:
        simulationPhaseTerminated = (currentFFluxPhaseOutputPtr->getMsg()->sucessful_trajectories_launched_count() >=
            mutableCurrentPhaseLimit()->uvalue());
        break;
    case FFPhaseLimEnums::TRAJECTORY_COUNT:
        simulationPhaseTerminated = (currentFFluxPhaseOutputPtr->getMsg()->sucessful_trajectories_launched_count() + currentFFluxPhaseOutputPtr->getMsg()->failed_trajectories_launched_count() >=
            mutableCurrentPhaseLimit()->uvalue());
        break;
    case FFPhaseLimEnums::TIME:
        simulationPhaseTerminated = (currentFFluxPhaseOutputPtr->getMsg()->sucessful_trajectories_launched_total_time() + currentFFluxPhaseOutputPtr->getMsg()->failed_trajectories_launched_total_time() >=
            mutableCurrentPhaseLimit()->dvalue());
        break;
    }
    return simulationPhaseTerminated;
}

void FFluxSupervisor::finishSimulationPhase()
{
    // send the phase output to the output writer
    // TODO: implement sending FFluxPhaseOutput to output writer

    // do any necessary cleanup of the now finished simulation phase
    cleanUpSimulationPhase();

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
    // swap the subjects of the current and previous phase output pointers
    lm::protowrap::FFluxPhaseOutput* tmpFFluxPhaseOutputPtr = previousFFluxPhaseOutputPtr;
    previousFFluxPhaseOutputPtr = currentFFluxPhaseOutputPtr;
    currentFFluxPhaseOutputPtr = tmpFFluxPhaseOutputPtr;

    // add a new phase output
    lm::fflux::io::FFluxPhaseOutput* newPhaseOutputMsg = ffluxPhaseOutputs.Add();

    // set the new phase output to be the current phase output
    currentFFluxPhaseOutputPtr->setMsg(newPhaseOutputMsg);
}

void FFluxSupervisor::finishSimulationStage()
{
    // send the stage output to the output writer
    // TODO: implement sending FFluxStageOutput to output writer

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

    // increment the stage output
    addFFluxStageOutput();
}

void FFluxSupervisor::addFFluxStageOutput()
{
    // add a new phase output
    lm::fflux::io::FFluxStageOutput* newStageOutputMsg = ffluxStageOutputs.Add();

    // set the new phase output to be the current phase output
    currentFFluxStageOutput.setWrappedMsg(newStageOutputMsg);
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

//void FFluxSupervisor::incrementFFluxPhase()
//{
//    currentFFluxPhaseIndex()++;
//
//
//    static_cast<lm::fflux::FFluxTrajectoryList*>(trajectoryList)->incrementFFluxPhase();
//}

//void FFluxSupervisor::finishSimulation()
//{
//    // Create the output message.
//    lm::message::Message msg;
//    lm::message::ProcessWorkUnitOutput* pwoMsg = msg.mutable_process_work_unit_output();
//    pwoMsg->set_work_unit_id(std::numeric_limits<int64_t>::max());
//    lm::message::WorkUnitOutput* wuoMsg = pwoMsg->add_part_output();
//
//    // Initialize/assign the fflux output data
//    lm::io::FFluxOutput* ffluxOutputBuf = wuoMsg->mutable_fflux_output();
//    ffluxOutputBuf->CopyFrom(*(static_cast<lm::fflux::FFluxTrajectoryList*>(trajectoryList)->getFFluxOutput()));
//
//    // Send the message
//    communicator.sendMessageToMasterOutput(&msg);
//
//    SimulationSupervisor::finishSimulation();
//}

//void FFluxSupervisor::receivedProcessWorkUnitOutput(lm::message::Message& msg)
//{
//    // Loop over every output in the message.
//    lm::message::ProcessWorkUnitOutput pwuMsg = msg.process_work_unit_output();
//    for (int i=0; i<pwuMsg.part_output_size(); i++)
//    {
//        lm::message::WorkUnitOutput wuoMsg = pwuMsg.part_output(i);
//        if (wuoMsg.has_species_counts())
//        {
//            (static_cast<FFluxTrajectoryList*>(trajectoryList))->ffluxOutputAddTrajectory(wuoMsg.species_counts(), lm::io::FFluxOutput::RUNNING);
//        }
//        else if (wuoMsg.has_species_time_series())
//        {
//            (static_cast<FFluxTrajectoryList*>(trajectoryList))->ffluxOutputAddTrajectory(wuoMsg.species_time_series(), lm::io::FFluxOutput::RUNNING);
//        }
//    }
//}

//void FFluxSupervisor::receivedStartedOutputWriter(const lm::message::StartedOutputWriter& msg)
//{
//    Print::printf(Print::INFO, "Output writer started: %d:%d.",msg.process(),msg.thread());
//    hasOutputWriterStarted = true;
//
//    // set output process/thread to that of this supervisor, while keeping track of the real values
//    outputWriterProcess = communicator.getSourceProcess();
//    outputWriterThread = communicator.getSourceThread();
////    outputWriterProcess = msg.process();
////    outputWriterThread = msg.thread();
//    communicator.setMasterOutputEndpoint(msg.process(), msg.thread());
//    startSimulationIfAllWorkersStarted();
//}

//void FFluxSupervisor::resetFFluxPhase()
//{
//    currentFFluxPhaseIndex() = 0;
//}

//void FFluxSupervisor::startSimulation()
//{
//    // Check for some error conditions.
//    if (outputWriterProcess == -1 || outputWriterThread == -1)
//        throw new Exception("Forward flux supervisor could not start the simulation, no output writer available.");
//
//    Print::printf(Print::INFO, "Forward flux supervisor starting simulation.");
//
//    // Call the base class method.
//    SimulationSupervisor::startSimulation();
//}


//void FFluxSupervisor::buildRunWorkUnitLimits(lm::message::RunWorkUnit* msg)
//{
//    // Set the limits in the RunWorkUnit header.
//    msg->mutable_trajectory_limits()->CopyFrom(trajectoryLimits.buf());
//}

//void FFluxSupervisor::setLimits()
//{
//    trajectoryLimits.clear();
//    const lm::tiling::Tiling& tiling = input->getTilings().getCurrentTiling();
//
//    if (currentFFluxPhaseIndex()==0)
//    {
//        tiling.addLimitMsg(trajectoryLimits, 0, EH::INCREASING);
//        tiling.addLimitMsg(trajectoryLimits, 0, EH::DECREASING);
//
//        tiling.addLimitMsg(trajectoryLimits, tiling.getLastEdgeIndex(), EH::INCREASING);
//    }
//    else
//    {
//        tiling.addLimitMsg(trajectoryLimits, 0, EH::DECREASING);
//
//        tiling.addLimitMsg(trajectoryLimits, currentFFluxPhaseIndex(), EH::INCREASING);
//    }
//
////    switch ((ffluxPhase!=0)<<1|input.tilings.getCurrentTiling()->getSortOrder()!=lm::input::Tilings::ASCENDING)
////    {
////    case 0: // ffluxphase==0 and tilings.getCurrentTiling().getSortOrder()==lm::input::Tilings::ASCENDING
////    {
////        // increasing edge 0 limit
////        lm::input::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_increasing_order_parameter_limit();
////        iopl->set_arrangement(lm::input::TrajectoryLimits::ASCENDING);
////        iopl->set_limit_id(0);
////        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
////
////        // decreasing edge 0 limit
////        lm::input::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_decreasing_order_parameter_limit();
////        dopl->set_arrangement(lm::input::TrajectoryLimits::ASCENDING);
////        dopl->set_limit_id(0);
////        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
////
////        // increasing final edge limit
////        iopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_increasing_order_parameter_limit();
////        iopl->set_arrangement(lm::input::TrajectoryLimits::ASCENDING);
////        iopl->set_limit_id(input.tilings.getCurrentTiling()->getEdgesCount() - 1);
////        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        iopl->add_value(input.tilings.getCurrentTiling()->getFinalEdge());
////        break;
////    }
////    case 1: // ffluxphase==0 and tilings.getCurrentTiling().getSortOrder()==lm::input::Tilings::DESCENDING
////    {
////        // decreasing edge 0 limit
////        lm::input::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_decreasing_order_parameter_limit();
////        dopl->set_arrangement(lm::input::TrajectoryLimits::DESCENDING);
////        dopl->set_limit_id(0);
////        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
////
////        // increasing edge 0 limit
////        lm::input::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_increasing_order_parameter_limit();
////        iopl->set_arrangement(lm::input::TrajectoryLimits::DESCENDING);
////        iopl->set_limit_id(0);
////        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
////
////        // decreasing final edge limit
////        dopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_decreasing_order_parameter_limit();
////        dopl->set_arrangement(lm::input::TrajectoryLimits::DESCENDING);
////        dopl->set_limit_id(input.tilings.getCurrentTiling()->getEdgesCount() - 1);
////        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        dopl->add_value(input.tilings.getCurrentTiling()->getFinalEdge());
////        break;
////    }
////    case 2: // ffluxphase!=0 and tilings.getCurrentTiling().getSortOrder()==lm::input::Tilings::ASCENDING
////    {
////        // decreasing edge 0 limit
////        lm::input::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_decreasing_order_parameter_limit();
////        dopl->set_arrangement(lm::input::TrajectoryLimits::ASCENDING);
////        dopl->set_limit_id(0);
////        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
////
////        // increasing current phase edge limit
////        lm::input::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_increasing_order_parameter_limit();
////        iopl->set_arrangement(lm::input::TrajectoryLimits::ASCENDING);
////        iopl->set_limit_id(ffluxPhase);
////        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(ffluxPhase));
////        break;
////    }
////    case 3: // ffluxphase!=0 and tilings.getCurrentTiling().getSortOrder()==lm::input::Tilings::DESCENDING
////    {
////        // increasing edge 0 limit
////        lm::input::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_increasing_order_parameter_limit();
////        iopl->set_arrangement(lm::input::TrajectoryLimits::DESCENDING);
////        iopl->set_limit_id(0);
////        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
////
////        // decreasing current phase edge limit
////        lm::input::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_decreasing_order_parameter_limit();
////        dopl->set_arrangement(lm::input::TrajectoryLimits::DESCENDING);
////        dopl->set_limit_id(ffluxPhase);
////        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(ffluxPhase));
////        break;
////    }
////    }
//}

//void FFluxSupervisor::finishSimulation()
//{
//	// Create the output message.
//	lm::message::Message msgp;
//	lm::message::ProcessWorkUnitOutput* msg = msgp.add_process_work_unit_output();
//	msg->set_work_unit_id(999999999999999);
//
//	// Initialize the fflux output data
//	lm::io::FFluxOutput* ffluxOutput = NULL;
//	ffluxOutput = msg->mutable_fflux_output();
//
//	// Assign the fflux output data
//	*ffluxOutput = *(static_cast<lm::fflux::FFluxTrajectoryList*>(trajectoryList)->getFFluxOutput());
//
//	// Send the message
//	communicator.sendMessageToMasterOutput(&msgp);
//
//	SimulationSupervisor::finishSimulation();
//}

}
}
