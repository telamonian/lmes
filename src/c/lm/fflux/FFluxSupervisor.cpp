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
#include <algorithm>
#include <iomanip>
#include <iterator>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <valarray>
#include <vector>

#include "hrtime.h"
#include "lm/ClassFactory.h"
#include "lm/EnumHelper.h"
#include "lm/fflux/FFluxPhaseZeroTrajectory.h"
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
#include "lm/VectorMath.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

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
:ffluxPhaseOutputListsWrap(&ffluxPhaseOutputListsMsg),ffluxProgress_lastPrintTime(getHrTime()),previousFFluxPhaseOutputWrapPtr(&_ffluxPhaseOutputWrap_0),currentFFluxPhaseOutputWrapPtr(&_ffluxPhaseOutputWrap_1),
 ffluxStageOutputsWrap(&ffluxStageOutputsMsg),simulationPhaseTerminated(false),simulationPhaseOutputSent(false),simulationStageOutputSent(false),input(NULL),trajectoryList(NULL)
{
    ffluxPhaseOutputContainingMsg.mutable_process_work_unit_output()->set_work_unit_id(0);
    ffluxPhaseOutputContainingMsg.mutable_process_work_unit_output()->add_part_output();

    ffluxStageOutputRawContainingMsg.mutable_process_work_unit_output()->set_work_unit_id(0);
    ffluxStageOutputRawContainingMsg.mutable_process_work_unit_output()->add_part_output();

    ffluxStageOutputSummaryContainingMsg.mutable_process_work_unit_output()->set_work_unit_id(0);
    ffluxStageOutputSummaryContainingMsg.mutable_process_work_unit_output()->add_part_output();
}

FFluxSupervisor::~FFluxSupervisor()
{
}

void FFluxSupervisor::init()
{
    // Initialize the FFluxInput pointer with the input file.
    setInput(new lm::fflux::input::FFluxInput(simulationInputFilenames));
}

// overrides parent method completely
void FFluxSupervisor::startSimulation()
{
    if (input->ffluxSimulationInput().has_fflux_stage_list())
    {
        initSimulationStageListCustom();
    }
    else
    {
        initSimulationStageList();
    }

    Print::printf(Print::INFO, "Simulation started.");

    // call the function which starts the simulation stage (which will then call startSimulationPhase())
    startSimulationStage();
}

void FFluxSupervisor::initSimulationStageList()
{
    // sanity check the input (make we sure we have at least one tiling, etc)
    sanityCheckInput();

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

void FFluxSupervisor::initSimulationStageListCustom()
{
    // copy the stage list over from input
    ffluxStageListMsg.CopyFrom(input->ffluxSimulationInput().fflux_stage_list());

    for (FFluxStagesWrap::iterator it=ffluxStageListMsg.mutable_fflux_stages()->begin(); it!=ffluxStageListMsg.mutable_fflux_stages()->end(); it++)
    {
        if (not it->has_tiling())
        {
            addTiling(&*it, input->getTilings().at(it->tiling_id()), it->basin_index());

            // place a ptr to the stage in the execution order (the pilot stage ptr, if any, will be placed before the production stage pointer)
            ffluxStageExecutionOrder.push_back(&*it);
        }
    }
}

void FFluxSupervisor::sanityCheckInput()
{
    if (input->getTilings().size() <= 0)
    {
        THROW_EXCEPTION(InputException, "FFPilot simulation requested (via cmd line arguments), but no tilings were provided in the input.");
    }

    int totalBasinCount = 0;
    for (lm::tiling::Tilings::const_iterator tilingIt=input->getTilings().begin();tilingIt!=input->getTilings().end();++tilingIt)
    {
        totalBasinCount += tilingIt->second->basins().size();
    }
    if (totalBasinCount <= 0)
    {
        THROW_EXCEPTION(InputException, "FFPilot simulation requested (via cmd line arguments), but there were no basins in any of the tilings provided in the input.");
    }
}

lm::fflux::input::FFluxStage* FFluxSupervisor::buildProductionStage(lm::fflux::input::FFluxStage* productionStage, const lm::tiling::Tiling& tiling, int64_t basinIndex)
{
    addTiling(productionStage, tiling, basinIndex);

    if (input->hasErrorGoal())
    {
        // initialize the pilot stage
        lm::fflux::input::FFluxStage* pilotStage = addPilotStage(productionStage);

        // add the pilot stage to the execution order
        ffluxStageExecutionOrder.push_back(pilotStage);
    }

    addFFluxPhases(productionStage, FFPhaseEnums::LAZY, FFPhaseEnums::UNIFORM_RANDOM);

    if (not productionStage->has_pilot_stage())
    {
        addFFluxPhaseLimitsFromInput(productionStage);
    }

    return productionStage;
}

void FFluxSupervisor::addTiling(lm::fflux::input::FFluxStage* stage, const lm::tiling::Tiling& tiling, int64_t basinIndex)
{
    // TODO: encapsulate this mess in a FFluxStage wrapper
    // shallow copy the tiling wrapper. We're done with the passed in tiling wrapper
    lm::tiling::Tiling tilingWrapCopy = tiling;

    // copy the actual tiling message over to the production stage message
    stage->mutable_tiling()->CopyFrom(tilingWrapCopy.getTilingMsg());

    // reseat the tiling wrapper copy around the tiling message copy
    tilingWrapCopy.setTilingMsg(stage->mutable_tiling());

    // use the tiling wrapper copy to set the appropriate basin_index in the tiling. This will also reverse the tiling, if needed
    tilingWrapCopy.setBasin(basinIndex);

    stage->set_basin_index(basinIndex);
    stage->set_tiling_id(tilingWrapCopy.id());
}

lm::fflux::input::FFluxStage* FFluxSupervisor::addPilotStage(lm::fflux::input::FFluxStage* productionStage)
{
    lm::fflux::input::FFluxStage* pilotStage = productionStage->mutable_pilot_stage();
    pilotStage->set_is_pilot_stage(true);

    pilotStage->mutable_tiling()->CopyFrom(productionStage->tiling());
    pilotStage->set_basin_index(productionStage->basin_index());

    addFFluxPhases(pilotStage, FFPhaseEnums::LAZY, FFPhaseEnums::UNIFORM_RANDOM);

    addFFluxPhaseLimitsForPilotStage(pilotStage, FFPhaseLimEnums::FORWARD_FLUXES, input->ffluxOptions().pilot_stage_count()*input->phaseZeroSamplingMultiplier(), input->ffluxOptions().pilot_stage_count());    //input->ffluxOptions().pilot_stage_count()*input->ffluxOptions().phase_zero_sampling_multiplier(), input->ffluxOptions().pilot_stage_count());

    return pilotStage;
}

void FFluxSupervisor::addFFluxPhases(lm::fflux::input::FFluxStage* stage, FFPhaseEnums::TrajectoryGeneration trajGeneration, FFPhaseEnums::TrajectoryDuplication trajDuplication)
{
    for (int i=0;i<stage->tiling().edges_size();i++)
    {
        lm::fflux::input::FFluxPhase* ffluxPhase = stage->add_fflux_phases();

        ffluxPhase->set_fflux_phase_index(i);
        ffluxPhase->set_basin_index(stage->basin_index());
        ffluxPhase->set_tiling_id(stage->tiling().id());
        ffluxPhase->set_tile_index(i);

        // set ffluxPhase values that depend on whether phaseIndex==0 or phaseIndex > 0
        if (i==0)
        {
            ffluxPhase->set_batch_size(1);

            ffluxPhase->set_trajectory_duplication(FFPhaseEnums::NONE);
            ffluxPhase->set_trajectory_generation(FFPhaseEnums::LAZY);
        }
        else
        {
            ffluxPhase->set_batch_size(input->ffluxOptions().batch_size());

            ffluxPhase->set_trajectory_duplication(trajDuplication);
            ffluxPhase->set_trajectory_generation(trajGeneration);
        }

        // (re)initialize the relevant output options
        stringstream outputPrefixSS;
        outputPrefixSS << "/FFluxOutput" << currentPhaseInfo(true, ffluxPhase, stage);
        input->reinitOutputOptions(outputPrefixSS.str(), stage->is_pilot_stage());
        ffluxPhase->mutable_output_options()->CopyFrom(input->getOutputOptionsMsg());
    }
}

void FFluxSupervisor::startSimulationStage()
{
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

    // set the stage output flag
    simulationStageOutputSent = false;

    // print an info message about the stage we're starting up
    Print::printf(Print::INFO, "Forward Flux stage %3d started (%s)", currentStageIndex(), currentStageInfo().c_str());

    // start the new phase
    startSimulationPhase();
}

void FFluxSupervisor::addFFluxStageOutput()
{
    // add a new phase output
    lm::fflux::io::FFluxStageOutput* newStageOutputMsg = ffluxStageOutputsWrap.Add();

    // set the new stage output to be the current phase output
    currentFFluxStageOutputWrap.setWrappedMsg(newStageOutputMsg);

    // add a new FFluxPhaseOutputList to go with this stage
    currentFFluxPhaseOutputsWrap.setWrappedField(ffluxPhaseOutputListsWrap.Add()->mutable_fflux_phase_outputs());
}

template <typename Value>
lm::fflux::input::FFluxPhaseLimit* FFluxSupervisor::buildFFluxPhaseLimit(lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit, const lm::fflux::input::FFluxPhase& ffluxPhase, FFPhaseLimEnums::StopCondition stopCondition, Value value)
{
    ffluxPhaseLimit->set_stop_condition(stopCondition);

//    ////TEMPSTART
//    if (ffluxPhase.fflux_phase_index()==1)
//    {
//        value = 10000;
//    }
//    else
//    {
//        value = 100;
//    }
//    ////TEMPSTOP

    switch (stopCondition)
    {
    case FFPhaseLimEnums::FORWARD_FLUXES: ffluxPhaseLimit->set_uvalue(value); break;
    case FFPhaseLimEnums::TRAJECTORY_COUNT: ffluxPhaseLimit->set_uvalue(value); break;
    case FFPhaseLimEnums::TIME: ffluxPhaseLimit->set_dvalue(value); break;
    }

    buildFFluxPhaseLimitTrajectoriesToRun(ffluxPhaseLimit, ffluxPhase, slots.getSimultaneousWorkUnits());

    return ffluxPhaseLimit;
}

void FFluxSupervisor::buildFFluxPhaseLimitTrajectoriesToRun(lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit, const lm::fflux::input::FFluxPhase& ffluxPhase, uint simultaneousWorkUnits)
{
    // builds the events_per_trajectory field for this limit if it hasn't already been set
    buildFFluxPhaseLimitEventsPerTrajectory(ffluxPhaseLimit, ffluxPhase, simultaneousWorkUnits);

    uint64_t simulataneousActiveTrajectories = simultaneousWorkUnits*ffluxPhase.batch_size();

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

void FFluxSupervisor::buildFFluxPhaseLimitEventsPerTrajectory(lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit, const lm::fflux::input::FFluxPhase& ffluxPhase, uint simultaneousWorkUnits)
{
    uint64_t simulataneousActiveTrajectories = simultaneousWorkUnits*ffluxPhase.batch_size();

    // events_per_trajectory can be set before running this function
    if (not ffluxPhaseLimit->has_events_per_trajectory())
    {
        if (ffluxPhase.fflux_phase_index()==0)
        {
            if (ffluxPhaseLimit->stop_condition()==FFPhaseLimEnums::FORWARD_FLUXES)
            {
                if (ffluxPhase.trajectory_generation()==FFPhaseEnums::EAGER)
                {
                    ffluxPhaseLimit->set_events_per_trajectory(ceilDiv(ffluxPhaseLimit->uvalue(), simulataneousActiveTrajectories));
//                    ffluxPhaseLimit->set_events_per_trajectory(-1);
                }
                else if (ffluxPhase.trajectory_generation()==FFPhaseEnums::LAZY)
                {
                    ffluxPhaseLimit->set_events_per_trajectory(ffluxPhaseLimit->uvalue());
                }
            }
            else throw UnimplementedException("ffluxPhaseLimit->stop_condition()==TIME, ==TRAJECTORY_COUNT currently unimplemented for fflux phase 0");
        }
        else
        {
            ffluxPhaseLimit->set_events_per_trajectory(1);
        }
    }
}

template <typename Value>
void FFluxSupervisor::addFFluxPhaseLimitsForPilotStage(lm::fflux::input::FFluxStage* stage, FFPhaseLimEnums::StopCondition stopCondition, Value phaseZeroValue, Value value)
{
    // special handling for phase zero
    FFluxPhasesWrap::const_iterator it=stage->fflux_phases().begin();
    buildFFluxPhaseLimit(stage->add_fflux_phase_limits(), *it, stopCondition, phaseZeroValue);
    it++;

    for (;it!=stage->fflux_phases().end();it++)
    {
        buildFFluxPhaseLimit(stage->add_fflux_phase_limits(), *it, stopCondition, value);
    }
}

void FFluxSupervisor::addFFluxPhaseLimitsFromInput(lm::fflux::input::FFluxStage* productionStage)
{
    // TODO: implement manually specified ffluxPhaseLimits
    //productionStage->mutable_fflux_phase_limits()->CopyFrom(input->getFFluxPhaseLimits(productionStage->tiling().id(), productionStage->basin_index()));

    // temporary placeholder
    addFFluxPhaseLimitsForPilotStage(productionStage, FFPhaseLimEnums::FORWARD_FLUXES, input->ffluxOptions().pilot_stage_count(), input->ffluxOptions().pilot_stage_count());
}

void FFluxSupervisor::addFFluxPhaseLimitsFromStageOutput(lm::fflux::input::FFluxStage* productionStage, const lm::protowrap::FFluxStageOutputWrap& stageOutput)
{
    vector<uint64_t> trajectoryCounts(optimizeTrajectoryCounts(input->errorGoal(), input->errorGoalConfidence(), stageOutput, input->productionStageCountMinimum(), input->phaseZeroSamplingMultiplier(), input->minimizeCost()));
    stringstream optimizationStatus;
    optimizationStatus.setf(std::ios::fixed, std::ios::floatfield);
    optimizationStatus.precision(2);

    const lm::protowrap::FFluxStageOutputSummaryWrap& soSummary(stageOutput.fflux_stage_output_summary());
    vector<double> costs(soSummary.costs().begin(), soSummary.costs().end());
    vector<double> probabilities(estimateBernoulliProbabilities(stageOutput));
    optimizationStatus << "Pilot stage output:\n";
    optimizationStatus << "The phase costs are:\n" << costs << "\n";
    optimizationStatus << "Conservative estimates of the phase probabilities are:\n" << probabilities << "\n";
    optimizationStatus << "Attempting to acheive error goal " << input->errorGoal() << " (confidence level " << input->errorGoalConfidence() << ") with the following optimized trajectory counts:\n" << trajectoryCounts;

    Print::printf(Print::INFO, optimizationStatus.str().c_str());

    vector<uint64_t>::const_iterator tc_it=trajectoryCounts.begin();
    FFluxPhasesWrap::const_iterator ph_it=productionStage->fflux_phases().begin();

    // special treatment for phase zero
    buildFFluxPhaseLimit(productionStage->add_fflux_phase_limits(), *ph_it, FFPhaseLimEnums::FORWARD_FLUXES, *tc_it); //input->ffluxOptions().pilot_stage_count()*input->ffluxOptions().phase_zero_sampling_multiplier());
    tc_it++, ph_it++;

    // all phases n>0
    for (;tc_it!=trajectoryCounts.end() and ph_it!=productionStage->fflux_phases().end();tc_it++, ph_it++)
    {
        buildFFluxPhaseLimit(productionStage->add_fflux_phase_limits(), *ph_it, FFPhaseLimEnums::TRAJECTORY_COUNT, *tc_it);
    }

//    // special treatment for phase zero
//    buildFFluxPhaseLimit(productionStage->add_fflux_phase_limits(), *ph_it, FFPhaseLimEnums::FORWARD_FLUXES, *tc_it); //input->ffluxOptions().pilot_stage_count()*input->ffluxOptions().phase_zero_sampling_multiplier());
//
//    optimizationStatus << *tc_it; //input->ffluxOptions().pilot_stage_count()*input->ffluxOptions().phase_zero_sampling_multiplier();
//    tc_it++, ph_it++;
//
//    // all phases n>0
//    for (;tc_it!=trajectoryCounts.end() and ph_it!=productionStage->fflux_phases().end();tc_it++, ph_it++)
//    {
//        buildFFluxPhaseLimit(productionStage->add_fflux_phase_limits(), *ph_it, FFPhaseLimEnums::TRAJECTORY_COUNT, *tc_it);
//
//        optimizationStatus << ", " << *tc_it;
//    }
}

void FFluxSupervisor::repeatFFluxPhaseLimits(lm::fflux::input::FFluxStage* stage, const lm::fflux::input::FFluxPhaseLimit& limitToRepeat)
{
    // add copies of limitToRepeat for every ffluxPhase that's missing a corresponding ffluxPhaseLimit
    for (int i=stage->fflux_phase_limits_size();i<stage->fflux_phases_size();i++)
    {
        stage->add_fflux_phase_limits()->CopyFrom(limitToRepeat);
    }
}

std::vector<double> FFluxSupervisor::estimateBernoulliProbabilities(const lm::protowrap::FFluxStageOutputWrap& stageOutput, double confidence, double minimum)
{
    const lm::protowrap::FFluxStageOutputRawWrap& soRaw(stageOutput.fflux_stage_output_raw());
    const lm::protowrap::FFluxStageOutputSummaryWrap& soSummary(stageOutput.fflux_stage_output_summary());

    vector<double> probabilities(soSummary.probabilities().begin(), soSummary.probabilities().end());
    vector<double> failedTrajectoryCounts(soRaw.failed_trajectory_counts().begin(), soRaw.failed_trajectory_counts().end());
    vector<double> successfulTrajectoryCounts(soRaw.successful_trajectory_counts().begin(), soRaw.successful_trajectory_counts().end());
    vector<double> trials(failedTrajectoryCounts + successfulTrajectoryCounts);

//    vector<double> trials;
//    add(soRaw.failed_trajectory_counts().begin(), soRaw.failed_trajectory_counts().end(), soRaw.successful_trajectory_counts().begin(), std::back_inserter(trials));

//    ////TEMPSTART
//
//    double probarr[] = {1,
//                        0.091788841786056868,
//                        0.27448083832335329,
//                        0.1359005213028652,
//                        0.15162949194547706,
//                        0.24463517433904428,
//                        0.63836902585531474,
//                        0.71211728865194213,
//                        0.85738534396809574,
//                        0.91288696210661524,
//                        0.97302793296089385,
//                        0.98923351158645279,
//                        0.99821428571428572};
//
//    vector<double> probabilities(probarr, probarr + 13);
//    ////TEMPEND

//    // make estimates more conservative by adjusting probabilities downward based on std var (ie (std err)**2)
//    probabilities = probabilities - (normalZ(.9975)/1000)*((1 - probabilities)*probabilities);

    // make estimates more conservative using the lower bound of the estimator confidence interval
    vector<double> conservativeProbabilities(bernouliCIAgrestiCoullLowerBound(probabilities, trials, confidence));

    // make sure that all of the probability estimates are at least a little above zero (if requested)
    if (minimum>=0)
    {
        for (vector<double>::iterator it=conservativeProbabilities.begin();it!=conservativeProbabilities.end();it++)
        {
            *it = max(*it, minimum);
        }
    }

    return conservativeProbabilities;
}

vector<uint64_t> FFluxSupervisor::optimizeTrajectoryCounts(double errorGoal, double errorGoalConfidence, const lm::protowrap::FFluxStageOutputWrap& stageOutput, uint64_t minimumCount, uint64_t phaseZeroSamplingMultipiler, bool minimizeCost)
{
    const lm::protowrap::FFluxStageOutputSummaryWrap& soSummary(stageOutput.fflux_stage_output_summary());
    vector<double> probabilities(estimateBernoulliProbabilities(stageOutput));

    vector<uint64_t> trajectoryCounts;
    if (minimizeCost)
    {
        vector<double> costVector(soSummary.costs().begin(), soSummary.costs().end());
        trajectoryCounts = minimizeCostTrajectoryCounts(errorGoal, errorGoalConfidence, probabilities, costVector);
    }
    else
    {
        trajectoryCounts = minimizeCountTrajectoryCounts(errorGoal, errorGoalConfidence, probabilities);
    }

    // "correct" undersampling durring phase zero
    vector<uint64_t>::iterator it=trajectoryCounts.begin();
    *it = (*it)*phaseZeroSamplingMultipiler;

    for (;it!=trajectoryCounts.end();it++) if (*it < minimumCount) *it=minimumCount;
    return trajectoryCounts;
}

vector<uint64_t> FFluxSupervisor::minimizeCostTrajectoryCounts(double errorGoal, double errorGoalConfidence, const vector<double>& probabilities, const vector<double>& costVector)
{
    valarray<double> constantFactors(getConstantFactors(probabilities));
    valarray<double> costs(costVector.data(), costVector.size());
    costs = sqrt(costs);

    double coeff = pow(normalZ(errorGoalConfidence)/errorGoal, 2)*((costs*constantFactors).sum());
    constantFactors /= costs;
    constantFactors *= coeff;

    vector<uint64_t> trajectoryCounts;
    for (int i=0;i<constantFactors.size();i++)
    {
        trajectoryCounts.push_back(static_cast<uint64_t>(ceil(constantFactors[i])));
    }
    return trajectoryCounts;
}

vector<uint64_t> FFluxSupervisor::minimizeCountTrajectoryCounts(double errorGoal, double errorGoalConfidence, const vector<double>& probabilities)
{
    valarray<double> constantFactors(getConstantFactors(probabilities));

    constantFactors *= pow(normalZ(errorGoalConfidence)/errorGoal, 2)*(constantFactors.sum());

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

    // ignore the probability from phase zero, store a fixed constant value
    constantFactors[0] = 1.0;

    // print statement for debug
    //printf("constantFactors:\n[");
    //for (int i=0;i<constantFactors.size();i++)
    //{
    //    printf("%.3f,\n", constantFactors[i]);
    //}
    //printf("]");

////     for now, skip the phase zero part
//    constantFactors[0] = 0.0;

    // take the square root
    constantFactors = sqrt(constantFactors);

    return constantFactors;
}

void FFluxSupervisor::startSimulationPhase()
{
    // add a new phase output
    addFFluxPhaseOutput();

    // Build the list of trajectories to simulate.
    buildTrajectoryList();

    // reset the phase output and termination flag
    simulationPhaseOutputSent = false;
    simulationPhaseTerminated = false;

    // print an info message about the phase we're starting up
    Print::printf(Print::INFO, "Forward Flux phase %3d started (%s)", currentFFluxPhaseIndex(), currentPhaseInfo().c_str());

    // Assign the first batch of work.
    if (terminateSimulationPhase() || assignWork())
    {
        // If .terminateSimulationPhase() or .assignWork() returned true, there was nothing to be done.
        Print::printf(Print::INFO, "No work to be performed.");
        finishSimulationPhase();
    }
}

void FFluxSupervisor::addFFluxPhaseOutput()
{
    // hand the previous FFluxPhaseOutput message off to the storage list (if this isn't the first or second phase of a simulation stage)
    if (previousFFluxPhaseOutputWrapPtr->wrappedMsg()!=NULL)
    {
        currentFFluxPhaseOutputsWrap.AddAllocated(previousFFluxPhaseOutputWrapPtr->wrappedMsg());
        previousFFluxPhaseOutputWrapPtr->setWrappedMsgNull();
    }

    // swap the subjects of the current and previous phase output wrapper pointers
    lm::protowrap::FFluxPhaseOutputWrap* tmpFFluxPhaseOutputWrapPtr = previousFFluxPhaseOutputWrapPtr;
    previousFFluxPhaseOutputWrapPtr = currentFFluxPhaseOutputWrapPtr;
    currentFFluxPhaseOutputWrapPtr = tmpFFluxPhaseOutputWrapPtr;

    // add a new phase output and set it to be the current phase output
    currentFFluxPhaseOutputsWrap.Add();
    currentFFluxPhaseOutputWrapPtr->setWrappedMsg(currentFFluxPhaseOutputsWrap.ReleaseLast());
}

void FFluxSupervisor::buildTrajectoryList()
{
    // if there is an old trajectoryList, get the trajectory count from that. Otherwise, we're at the very start of the simulation so count is 0
    uint64_t currentTrajectoryCount = trajectoryList != NULL ? trajectoryList->count() : 0;

    // set the trajectory limits/tracking for this phase
    input->reinitTrajectoryLimits(currentPhase(), currentPhaseLimit(), currentTiling());

    if (currentPhase().start_points_size() > 0)
    {
        ffluxPhaseOutputMsgCustom.mutable_successful_trajectory_end_points()->CopyFrom(currentPhase().start_points());
        setTrajectoryList(new FFluxTrajectoryList(currentTrajectoryCount, currentFFluxPhaseIndex(), currentPhase(), currentPhaseLimit(), slots.getSimultaneousWorkUnits(), *input, &ffluxPhaseOutputMsgCustom));
    }
    else if(currentFFluxPhaseIndex()==0)
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
    if (not simulationPhaseTerminated)
    {
        switch(mutableCurrentPhaseLimit()->stop_condition())
        {
        case FFPhaseLimEnums::FORWARD_FLUXES:
            simulationPhaseTerminated = (currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_count()>=mutableCurrentPhaseLimit()->uvalue());
            break;
        case FFPhaseLimEnums::TRAJECTORY_COUNT:
            // all phases during a forward flux simulation need to record at least one forward crossing or else it can't continue
            simulationPhaseTerminated = ((currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_count() > 0) and \
                                         (currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_count() + currentFFluxPhaseOutputWrapPtr->wrappedMsg()->failed_trajectories_launched_count()>=mutableCurrentPhaseLimit()->uvalue()));
            break;
        case FFPhaseLimEnums::TIME:
            // all phases during a forward flux simulation need to record at least one forward crossing or else it can't continue
            simulationPhaseTerminated = ((currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_count() > 0) and \
                                         (currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_total_time() + currentFFluxPhaseOutputWrapPtr->wrappedMsg()->failed_trajectories_launched_total_time()>=mutableCurrentPhaseLimit()->dvalue()));
            break;
        default: throw UnimplementedException("unimplemented");
        }
    }

    if (simulationPhaseTerminated) simulationPhaseEverTerminated = true;
    printFFluxLimitProgress();

    return simulationPhaseTerminated;
}

void FFluxSupervisor::printFFluxLimitProgress()
{
    if (not simulationPhaseTerminated)
    {
        // Print some performance statistics, if it has been a while.
        hrtime currentTime = getHrTime();
        if (convertHrToSeconds(currentTime-ffluxProgress_lastPrintTime) > 610.0)
        {
            Print::printf(Print::INFO, "Forward Flux Phase Limit Progress");
            Print::printf(Print::INFO, "  Phase_ID Limit_Type       Progress    Limit");
            Print::printf(Print::INFO, "----------------------------------------------------");

            switch(mutableCurrentPhaseLimit()->stop_condition())
            {
            case FFPhaseLimEnums::FORWARD_FLUXES:
                Print::printf(Print::INFO, "%10lld %-17s %12d %12d",
                    currentFFluxPhaseIndex(),
                    FFPhaseLimEnums::StopCondition_Name(mutableCurrentPhaseLimit()->stop_condition()).c_str(),
                    currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_count(),
                    mutableCurrentPhaseLimit()->uvalue());
                break;
            case FFPhaseLimEnums::TRAJECTORY_COUNT:
                Print::printf(Print::INFO, "%10lld %-17s %12d %12d",
                    currentFFluxPhaseIndex(),
                    FFPhaseLimEnums::StopCondition_Name(mutableCurrentPhaseLimit()->stop_condition()).c_str(),
                    currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_count() + currentFFluxPhaseOutputWrapPtr->wrappedMsg()->failed_trajectories_launched_count(),
                    mutableCurrentPhaseLimit()->uvalue());
                break;
            case FFPhaseLimEnums::TIME:
                Print::printf(Print::INFO, "%10lld %-17s %12.2e %12.2e",
                    currentFFluxPhaseIndex(),
                    FFPhaseLimEnums::StopCondition_Name(mutableCurrentPhaseLimit()->stop_condition()).c_str(),
                    currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_total_time() + currentFFluxPhaseOutputWrapPtr->wrappedMsg()->failed_trajectories_launched_total_time(),
                    mutableCurrentPhaseLimit()->dvalue());
                break;
            default: throw UnimplementedException("unimplemented");
            }

            ffluxProgress_lastPrintTime = getHrTime();
        }
    }
}

void FFluxSupervisor::finishSimulationPhase()
{
    // before anything else, send the phase output to the output writer (if needed)
    sendSimulationPhaseOutput();

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

void FFluxSupervisor::sendSimulationPhaseOutput()
{
    if (not simulationPhaseOutputSent)
    {
        // send the phase output to the output writer
        if ((not currentStage().is_pilot_stage()) or input->ffluxOptions().pilot_stage_output())
        {
            if (input->ffluxOptions().phase_output())
            {
                // (re)initialize the relevant output options
                stringstream outputPrefixSS;
                outputPrefixSS << "/FFluxOutput" << currentPhaseInfo(true);
                input->reinitOutputOptions(outputPrefixSS.str(), currentStage().is_pilot_stage());

                // create a handle to the relevant work unit output part
                lm::message::WorkUnitOutput* wuoPart(ffluxPhaseOutputContainingMsg.mutable_process_work_unit_output()->mutable_part_output(0));

                // set the data and output options the work unit output part
                wuoPart->set_condense_output(input->getOutputOptionsMsg().condense_output());
                wuoPart->set_record_name_prefix(input->getOutputOptionsMsg().record_name_prefix());

                // temporarily hand off the allocated phase output and send it
                wuoPart->mutable_work_unit_output_generic()->mutable_fflux_phase_outputs()->AddAllocated(currentFFluxPhaseOutputWrapPtr->wrappedMsg());
                communicator.sendMessage(outputWriterProcess, outputWriterThread, &ffluxPhaseOutputContainingMsg);
                wuoPart->mutable_work_unit_output_generic()->mutable_fflux_phase_outputs()->ReleaseLast();
                simulationPhaseOutputSent = true;
            }
        }
    }
}

void FFluxSupervisor::incrementSimulationPhase()
{
    // increment the currentFFluxPhase iterator
    currentFFluxPhaseIter++;

    // call the base class method
    lm::main::SimulationSupervisor::incrementSimulationPhase();
}

void FFluxSupervisor::finishSimulationStage()
{
    // send the stage output to the output writer (if needed)
    sendSimulationStageOutput();

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

void FFluxSupervisor::sendSimulationStageOutput()
{
    if (not simulationStageOutputSent)
    {
        // hand off the final ffluxPhaseOutputs to the repeated field wrapped by currentFFluxPhaseOutputsWrap
        if (previousFFluxPhaseOutputWrapPtr->wrappedMsg()!=NULL)
        {
            currentFFluxPhaseOutputsWrap.AddAllocated(previousFFluxPhaseOutputWrapPtr->wrappedMsg());
            previousFFluxPhaseOutputWrapPtr->setWrappedMsgNull();
        }
        if (currentFFluxPhaseOutputWrapPtr->wrappedMsg()!=NULL)
        {
            currentFFluxPhaseOutputsWrap.AddAllocated(currentFFluxPhaseOutputWrapPtr->wrappedMsg());
            currentFFluxPhaseOutputWrapPtr->setWrappedMsgNull();
        }

        // build the stage output from the phase outputs
        currentFFluxStageOutputWrap.buildFromFFluxPhaseOutputs(currentFFluxPhaseOutputsWrap);

        if ((not currentStage().is_pilot_stage()) or input->ffluxOptions().pilot_stage_output())
        {
            // (re)initialize the relevant output options
            stringstream outputPrefixSS;
            outputPrefixSS << "/FFluxOutput" << currentStageInfo(true);
            input->reinitOutputOptions(outputPrefixSS.str(), currentStage().is_pilot_stage());

            if (input->ffluxOptions().stage_output_raw())
            {
                // create a handle to the relevant work unit output part
                lm::message::WorkUnitOutput* wuoPart(ffluxStageOutputRawContainingMsg.mutable_process_work_unit_output()->mutable_part_output(0));

                // set the data and output options the work unit output part
                wuoPart->set_condense_output(input->getOutputOptionsMsg().condense_output());
                wuoPart->set_record_name_prefix(input->getOutputOptionsMsg().record_name_prefix());

                // temporarily hand off the allocated stage output and send it
                wuoPart->mutable_work_unit_output_generic()->mutable_fflux_stage_output_raws()->AddAllocated(currentFFluxStageOutputWrap.mutable_fflux_stage_output_raw()->mutableWrappedMsg());
                communicator.sendMessage(outputWriterProcess, outputWriterThread, &ffluxStageOutputRawContainingMsg);
                wuoPart->mutable_work_unit_output_generic()->mutable_fflux_stage_output_raws()->ReleaseLast();
                simulationStageOutputSent = true;
            }
            if (input->ffluxOptions().stage_output_summary())
            {
                // create a handle to the relevant work unit output part
                lm::message::WorkUnitOutput* wuoPart(ffluxStageOutputSummaryContainingMsg.mutable_process_work_unit_output()->mutable_part_output(0));

                // set the data and output options the work unit output part
                wuoPart->set_condense_output(input->getOutputOptionsMsg().condense_output());
                wuoPart->set_record_name_prefix(input->getOutputOptionsMsg().record_name_prefix());

                // temporarily hand off the allocated stage output and send it
                wuoPart->mutable_work_unit_output_generic()->mutable_fflux_stage_output_summaries()->AddAllocated(currentFFluxStageOutputWrap.mutable_fflux_stage_output_summary()->mutableWrappedMsg());
                communicator.sendMessage(outputWriterProcess, outputWriterThread, &ffluxStageOutputSummaryContainingMsg);
                wuoPart->mutable_work_unit_output_generic()->mutable_fflux_stage_output_summaries()->ReleaseLast();
                simulationStageOutputSent = true;
            }
        }
    }
}

void FFluxSupervisor::incrementSimulationStage()
{
    // increment the currentFFluxPhase iterator
    currentFFluxStageIter++;
}

// methods that handle setting up RunWorkUnit messages
void FFluxSupervisor::buildRunWorkUnitParts(lm::message::RunWorkUnit* msg, uint minWorkUnits)
{
    trajectoryList->addWorkUnitParts(msg->work_unit_id(), msg, minWorkUnits*currentPhase().batch_size());

    // Set the limit tracking messages, if any
    input->copyLimitTrackingsTo(msg);
}

// methods that handle FinishedWorkUnit messages
//void FFluxSupervisor::receivedFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg)
//{
//    // If we are not performing a checkpoint, distribute more work.
//    if (!performingCheckpoint)
//    {
//        // Fill the newly freed slot with a work unit. If there are more trajectories than slots, this is guaranteed to use the slot we just freed. Otherwise it will be the "coldest" (longest unoccupied) slot
//        if (terminateSimulationPhase() || assignWork())
//        {
//            finishSimulationPhase();
//        }
//    }
//    // Otherwise, see if all outstanding work units have finished.
//    else if (!slots.hasBusySlots())
//    {
//        Print::printf(Print::INFO, "Creating a checkpoint, pausing work.");
//
//        // Send a message to the output writer to save a checkpoint. Calling .mutable_perform_checkpointing() initializes the message
//        lm::message::Message msgp;
//        msgp.mutable_perform_checkpointing();
//        communicator.sendMessage(outputWriterProcess, outputWriterThread, &msgp);
//    }
//
//    Print::printf(Print::VERBOSE_DEBUG, "Work unit %d finished in %0.3f s.",msg.work_unit_id(),msg.run_time());
//
//    // deal with the individual parts of the work unit at the fflux supervisor level
//    for (int i=0;i<msg.part_status_size();i++)
//    {
//        receivedFinishedWorkUnitPart(msg.part_status(i));
//    }
//
//    // collect global performance stats for printPerformanceStatistics
//    stats_workUnits++;
//    stats_minWorkUnitId = std::min(stats_minWorkUnitId,(long long)msg.work_unit_id());
//    stats_maxWorkUnitId = std::max(stats_maxWorkUnitId,(long long)msg.work_unit_id());
//    stats_workUnitsSteps += msg.steps();
//    stats_workUnitTime += msg.run_time();
//    for (int i=0; i<msg.part_status_size(); i++)
//        stats_workUnitsParts++;
//
//    // Update the trajectory list.
//    trajectoryList->workUnitFinished(msg);
//
//    // Update the slots list.
//    slots.workUnitFinished(msg);
//}


void FFluxSupervisor::receivedFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg)
{
    PROF_BEGIN(PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT);

    // deal with the individual parts of the work unit at the fflux supervisor level
    for (int i=0;i<msg.part_status_size();i++)
    {
        receivedFinishedWorkUnitPart(msg.part_status(i));
    }

//    // If the phase "plan" calls for it, generate replacement trajectories
//    if (currentFFluxPhaseIter->trajectory_generation()==FFPhaseEnums::LAZY)
//    {
//        trajectoryList->initTrajectories(msg.part_status_size());
//    }

    // call the base class function
    lm::main::SimulationSupervisor::receivedFinishedWorkUnit(msg);

    PROF_END(PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT);
}

void FFluxSupervisor::receivedFinishedWorkUnitPart(const lm::message::WorkUnitStatus& wusMsg)
{
    if (not trajectoryList->isTrajectoryAborted(wusMsg.final_state().trajectory_id()))   // and wusMsg.status()==lm::message::WorkUnitStatus::LIMIT_REACHED)
    {
        if (currentFFluxPhaseIndex()==0)
        {
            PROF_BEGIN(PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT_PHASE_ZERO);

            receivedFinishedWorkUnitPartPhaseZero(wusMsg);

            PROF_END(PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT_PHASE_ZERO);
        }
        else if (wusMsg.status()==lm::message::WorkUnitStatus::LIMIT_REACHED)
        {
            ////TEMPSTART
            if (currentFFluxPhaseIndex()==1)
            {
                PROF_BEGIN(PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT_PHASE_ONE);

                // update the phase output
                currentFFluxPhaseOutputWrapPtr->addEndPoint(wusMsg.final_state(), *trajectoryList->getTrajectoryForFinishedWorkUnit(wusMsg.final_state().trajectory_id()));

                PROF_END(PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT_PHASE_ONE);
            }
            else
            {
            ////TEMPEND
                PROF_BEGIN(PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT_PHASE_N);

                // update the phase output
                currentFFluxPhaseOutputWrapPtr->addEndPoint(wusMsg.final_state(), *trajectoryList->getTrajectoryForFinishedWorkUnit(wusMsg.final_state().trajectory_id()));

                PROF_END(PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT_PHASE_N);
            }
        }
    }
}

void FFluxSupervisor::receivedFinishedWorkUnitPartPhaseZero(const lm::message::WorkUnitStatus& wusMsg)
{
    // get the relevant Trajectory instance
    lm::trajectory::Trajectory* trajectory = trajectoryList->getTrajectoryForFinishedWorkUnit(wusMsg.final_state().trajectory_id());

    // keep track of how much time each phase 0 trajectory spent in the region of a basin other than its initial basin
    lm::fflux::FFluxPhaseZeroTrajectory* phaseZeroTrajectory = static_cast<lm::fflux::FFluxPhaseZeroTrajectory*>(trajectory);
    phaseZeroTrajectory->accumulateTimeInOtherBasins(wusMsg.final_state());

    // update the phase output
    currentFFluxPhaseOutputWrapPtr->addEndPointPhaseZero(wusMsg.final_state(), trajectory, input->ffluxOptions().phase_zero_burn_in_count());
}

// accessors
/*
 * a short string with some info about the current phase
 */
std::string FFluxSupervisor::currentPhaseInfo(bool path, const lm::fflux::input::FFluxPhase* phase, const lm::fflux::input::FFluxStage* stage) const
{
    // if a phase and/or a stage has not been passed, use the current ones
    const lm::fflux::input::FFluxPhase& _phase(phase!=NULL ? *phase : currentPhase());
    const lm::fflux::input::FFluxStage& _stage(stage!=NULL ? *stage : currentStage());
    int64_t phaseIndex = _phase.fflux_phase_index();
    
    stringstream phaseInfo;
    if (path)
    {
        phaseInfo << currentStageInfo(true, stage);
        phaseInfo << "/Phases/" << phaseIndex;    //setfill('0') << setw(7) << phaseIndex;
    }
    else
    {
        phaseInfo.setf(std::ios::fixed, std::ios::floatfield);
        phaseInfo.precision(2);
        if (currentFFluxPhaseIndex()==0)
        {
            phaseInfo << "first_edge_value: " << setw(7) << _stage.tiling().edges(0);
        }
        else
        {
            phaseInfo << "starting_edge_value: " << setw(7) << _stage.tiling().edges(phaseIndex - 1);
            phaseInfo << ", final_edge_value: " << setw(7) << _stage.tiling().edges(phaseIndex);
        }
        
        const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit(_stage.fflux_phase_limits(phaseIndex));
        phaseInfo << ", phase_limit: " << FFPhaseLimEnums::StopCondition_Name(ffluxPhaseLimit.stop_condition());
        phaseInfo << " >= " << (ffluxPhaseLimit.stop_condition()==FFPhaseLimEnums::TIME ? ffluxPhaseLimit.dvalue() : ffluxPhaseLimit.uvalue());
    }
    
    return phaseInfo.str();
}

/*
 * a short string with some info about the current stage
 */
std::string FFluxSupervisor::currentStageInfo(bool path, const lm::fflux::input::FFluxStage* stage) const
{
    // if a stage has not been passed, use the current one
    const lm::fflux::input::FFluxStage& _stage(stage!=NULL ? *stage : currentStage());
    
    stringstream stageInfo;
    if (path)
    {
        stageInfo << "/Tilings/" << _stage.tiling().id();     //setfill('0') << setw(7) << _stage.tiling().id();
        stageInfo << "/Basins/" << _stage.tiling().current_basin_index();    //setfill('0') << setw(7) << _stage.tiling().current_basin_index();
        stageInfo << "/Stages";
        if (_stage.is_pilot_stage())
        {
            stageInfo << "/Pilot";
        }
        else if (_stage.has_pilot_stage())
        {
            stageInfo << "/Production";
        }
    }
    else 
    {
        stageInfo << "tiling_id: " << _stage.tiling().id();
        stageInfo << ", basin_index: " << _stage.tiling().current_basin_index();
        if (_stage.is_pilot_stage())
        {
            stageInfo << ", stage_type: " << "pilot";
        }
        else if (_stage.has_pilot_stage())
        {
            stageInfo << ", stage_type: " << "production";
        }
    }

    return stageInfo.str();
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


// TODO: work out something better than copying and pasting 90% of this function
void FFluxSupervisor::buildRunWorkUnitHeader(lm::message::RunWorkUnit* msg)
{
    // Set the work unit id.
    msg->set_work_unit_id(workUnitCount++);

    // Set the source process/thread.
    msg->set_supervisor_process(communicator.getSourceProcess());
    msg->set_supervisor_thread(communicator.getSourceThread());

    // Set the writer process/thread.
    msg->set_output_process(outputWriterProcess);
    msg->set_output_thread(outputWriterThread);

    // Set the limits.
    input->copyLimitsTo(msg);

    // Set the output options.
    msg->mutable_output_options()->CopyFrom(currentPhase().output_options());

    // Set the maximum number of steps for the work unit.
    msg->set_max_steps(input->getStepsPerWorkUnit());
}

}
}
