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
#include "lm/main/Globals.h"
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
 ffluxStageOutputsWrap(&ffluxStageOutputsMsg),ffluxPhaseTerminated(false),simulationPhaseOutputSent(false),simulationStageOutputSent(false),input(NULL),trajectoryList(NULL)
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

    simulationStartTime = getHrTime();
    Print::printf(Print::INFO, "Simulation started.");

    // call the function which starts the simulation stage (which will then call startSimulationPhase())
    startSimulationStage();
}

void FFluxSupervisor::initSimulationStageList()
{
    // sanity check the input (make we sure we have at least one tiling, etc)
    sanityCheckInput();

    // build the stage list
    for (vector<uint64_t>::const_iterator replicateIt = replicates.begin();replicateIt!=replicates.end();++replicateIt)
    {
        for (lm::tiling::Tilings::const_iterator tilingIt=input->getTilings().begin();tilingIt!=input->getTilings().end();++tilingIt)
        {
            for (int basinIndex=0;basinIndex<tilingIt->second->basins().size();basinIndex++)
            {
                // initialize a stage (and possibly also its pilot stage)
                lm::fflux::input::FFluxStage* productionStage = buildProductionStage(ffluxStageListMsg.add_fflux_stages(), *replicateIt, *tilingIt->second, basinIndex);

                // place a ptr to the stage in the execution order (the pilot stage ptr, if any, will be placed before the production stage pointer when buildProductionStage is run)
                ffluxStageExecutionOrder.push_back(productionStage);
            }
        }
    }
    currentFFluxStageIter = ffluxStageExecutionOrder.begin();
}

void FFluxSupervisor::initSimulationStageListCustom()
{
    // copy the stage list over from input
    ffluxStageListMsg.CopyFrom(input->ffluxSimulationInput().fflux_stage_list());

    for (vector<uint64_t>::const_iterator replicateIt = replicates.begin();replicateIt!=replicates.end();++replicateIt)
    {
        // iterate over the stages in ffluxStageListMsg.fflux_stages()
        for (FFluxStagesWrap::iterator stageIt=ffluxStageListMsg.mutable_fflux_stages()->begin(); stageIt!=ffluxStageListMsg.mutable_fflux_stages()->end(); stageIt++)
        {
            stageIt->set_replicate_id(*replicateIt);
            // TODO: implement pilot stages in initSimulationStageListCustom
            bool pilotRunRequired = false;

            // if the stage doesn't already have a tiling set, use the one from the simulation input
            if (not stageIt->has_tiling())
            {
                addTiling(&*stageIt, input->getTilings().at(stageIt->tiling_id()), stageIt->basin_id());
            }

            // iterate over the phases in stageIt->fflux_phases()
            for (FFluxPhasesWrap::iterator phaseIt=stageIt->mutable_fflux_phases()->begin(); phaseIt!=stageIt->mutable_fflux_phases()->end(); phaseIt++)
            {
                if (not phaseIt->has_output_options())
                {
                    // if the phase doesn't already have output options set, set them in the standard way based on the simulation input
                    addOptions(&*phaseIt, *stageIt);
                }

                if (not phaseIt->has_fflux_phase_limit())
                {
                    // If the phase doesn't already have a limit set, build it
                    buildFFluxPhaseLimit(phaseIt->mutable_fflux_phase_limit(), *phaseIt, FFPhaseLimEnums::TRAJECTORY_COUNT, input->productionStageCountMinimum());
                }
                else if (not phaseIt->fflux_phase_limit().has_events_per_trajectory())
                {
                    // If the phase has a limit but the number of runs/runners is not set, automatically figure it out
                    buildFFluxPhaseLimitTrajectoriesToRun(phaseIt->mutable_fflux_phase_limit(), *phaseIt, slots.getSimultaneousWorkUnits());
                }
            }

            // place a ptr to the stage in the execution order (the pilot stage ptr, if any, will be placed before the production stage pointer)
            ffluxStageExecutionOrder.push_back(&*stageIt);
        }
    }
    currentFFluxStageIter = ffluxStageExecutionOrder.begin();
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

lm::fflux::input::FFluxStage* FFluxSupervisor::buildProductionStage(lm::fflux::input::FFluxStage* productionStage, uint64_t replicateID, const lm::tiling::Tiling& tiling, int64_t basinIndex)
{
    productionStage->set_replicate_id(replicateID);
    productionStage->set_name("Production");
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

    // use the tiling wrapper copy to set the appropriate basin_id in the tiling. This will also reverse the tiling, if needed
    tilingWrapCopy.setBasin(basinIndex);

    stage->set_basin_id(basinIndex);
    stage->set_tiling_id(tilingWrapCopy.id());
}

lm::fflux::input::FFluxStage* FFluxSupervisor::addPilotStage(lm::fflux::input::FFluxStage* productionStage)
{
    lm::fflux::input::FFluxStage* pilotStage = productionStage->mutable_pilot_stage();
    pilotStage->set_name("Pilot");
    pilotStage->set_is_pilot_stage(true);

    pilotStage->set_replicate_id(productionStage->replicate_id());
    pilotStage->mutable_tiling()->CopyFrom(productionStage->tiling());
    pilotStage->set_basin_id(productionStage->basin_id());

    addFFluxPhases(pilotStage, FFPhaseEnums::LAZY, FFPhaseEnums::UNIFORM_RANDOM);

    addFFluxPhaseLimitsForPilotStage(pilotStage, FFPhaseLimEnums::FORWARD_FLUXES, static_cast<uint64_t>(round(input->ffluxOptions().pilot_stage_count()*input->phaseZeroSamplingMultiplier())), input->ffluxOptions().pilot_stage_count());    //input->ffluxOptions().pilot_stage_count()*input->ffluxOptions().phase_zero_sampling_multiplier(), input->ffluxOptions().pilot_stage_count());

    return pilotStage;
}

void FFluxSupervisor::addFFluxPhases(lm::fflux::input::FFluxStage* stage, FFPhaseEnums::TrajectoryGeneration trajGeneration, FFPhaseEnums::TrajectoryDuplication trajDuplication)
{
    for (int i=0;i<stage->tiling().edges_size();i++)
    {
        lm::fflux::input::FFluxPhase* ffluxPhase = stage->add_fflux_phases();

        ffluxPhase->set_phase_id(i);
        ffluxPhase->set_basin_id(stage->basin_id());
        ffluxPhase->set_tiling_id(stage->tiling().id());
        ffluxPhase->set_tile_id(i);

        // set ffluxPhase values that depend on whether phaseID==0 or phaseID > 0
        if (i==0)
        {
            ffluxPhase->set_trajectory_duplication(FFPhaseEnums::NONE);
            ffluxPhase->set_trajectory_generation(FFPhaseEnums::LAZY);
        }
        else
        {
            ffluxPhase->set_trajectory_duplication(trajDuplication);
            ffluxPhase->set_trajectory_generation(trajGeneration);
        }

        // (re)initialize the relevant output options
        addOptions(ffluxPhase, *stage);
    }
}

void FFluxSupervisor::addOptions(lm::fflux::input::FFluxPhase* phase, const lm::fflux::input::FFluxStage& stage)
{
    // (re)initialize the general simulation options
    input->reinitOptions(phase->phase_id());
    phase->mutable_options()->CopyFrom(input->getOptions());

    // (re)initialize the relevant output options
    input->reinitOutputOptions(phaseInfo(true, phase, &stage), stage.is_pilot_stage());
    phase->mutable_output_options()->CopyFrom(input->getOutputOptionsMsg());
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

    uint64_t simulataneousActiveTrajectories = simultaneousWorkUnits*ffluxPhase.options().parts_per_work_unit();

    if (ffluxPhase.trajectory_generation()==FFPhaseEnums::EAGER)
    {
        // EAGER is only implemented for certain ffluxPhaseLimit.stop_condition() values
        if (ffluxPhaseLimit->stop_condition()==FFPhaseLimEnums::TRAJECTORY_COUNT or (ffluxPhaseLimit->stop_condition()==FFPhaseLimEnums::FORWARD_FLUXES and ffluxPhase.phase_id()==0))
        {
            // given that our trajectory limits are set up to observe x events per trajectory, run ceil(y/x) trajectories to ensure that we observe at least y events total
            ffluxPhaseLimit->set_trajectories_per_phase(ceilDiv(ffluxPhaseLimit->uvalue(), ffluxPhaseLimit->events_per_trajectory()));
        }
        else throw UnimplementedException("In Forward Flux phase %d, ffluxPhase.trajectory_generation()==EAGER is only implemented for certain ffluxPhaseLimit.stop_condition() values (ie those that let us calculate the necessary trajectory count up front). Attempting to use unimplemented ffluxPhaseLimit.stop_condition(): %s", ffluxPhase.phase_id(), FFPhaseLimEnums::StopCondition_Name(ffluxPhaseLimit->stop_condition()).c_str());
    }
    else if (ffluxPhase.trajectory_generation()==FFPhaseEnums::LAZY)
    {
        return ffluxPhaseLimit->set_trajectories_per_phase(simulataneousActiveTrajectories);
    }
    else throw UnimplementedException("unimplemented");
}

void FFluxSupervisor::buildFFluxPhaseLimitEventsPerTrajectory(lm::fflux::input::FFluxPhaseLimit* ffluxPhaseLimit, const lm::fflux::input::FFluxPhase& ffluxPhase, uint simultaneousWorkUnits)
{
    uint64_t simulataneousActiveTrajectories = simultaneousWorkUnits*ffluxPhase.options().parts_per_work_unit();

    // events_per_trajectory can be set before running this function
    if (not ffluxPhaseLimit->has_events_per_trajectory())
    {
        if (ffluxPhase.phase_id()==0)
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
    //productionStage->mutable_fflux_phase_limits()->CopyFrom(input->getFFluxPhaseLimits(productionStage->tiling().id(), productionStage->basin_id()));

    // temporary placeholder
    addFFluxPhaseLimitsForPilotStage(productionStage, FFPhaseLimEnums::FORWARD_FLUXES, input->ffluxOptions().pilot_stage_count(), input->ffluxOptions().pilot_stage_count());
}

void FFluxSupervisor::addFFluxPhaseLimitsFromStageOutput(lm::fflux::input::FFluxStage* productionStage, const lm::protowrap::FFluxStageOutputWrap& stageOutput)
{
    // calculate the optimum trajectory counts for the production stage from the pilot stage output
    vector<uint64_t> trajectoryCounts(optimizeTrajectoryCounts(input->errorGoal(), input->errorGoalConfidence(), stageOutput, input->productionStageCountMinimum(), input->phaseZeroSamplingMultiplier(), input->minimizeCost()));

    // print some info about the pilot stage (costs, probabilities, etc) to the log (ie stdout)
    Print::printf(Print::INFO, stageLogPilot(stageOutput, input->errorGoal(), input->errorGoalConfidence(), trajectoryCounts).c_str());

    // zip over the trajectory counts and the phase specification msgs
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

vector<double> FFluxSupervisor::estimateBernoulliProbabilities(const lm::protowrap::FFluxStageOutputWrap& stageOutput, double confidence, double minimum)
{
    vector<double> weights(stageOutput.fflux_stage_output_summary().weights().begin(), stageOutput.fflux_stage_output_summary().weights().end());

    vector<double> failedTrajectoryCounts(stageOutput.fflux_stage_output_raw().failed_trajectory_counts().begin(), stageOutput.fflux_stage_output_raw().failed_trajectory_counts().end());
    vector<double> successfulTrajectoryCounts(stageOutput.fflux_stage_output_raw().successful_trajectory_counts().begin(), stageOutput.fflux_stage_output_raw().successful_trajectory_counts().end());
    vector<double> trials(failedTrajectoryCounts + successfulTrajectoryCounts);

    // make estimates more conservative using the lower bound of the estimator confidence interval. Note that this is not the correct treatment for phase zero
    vector<double> conservativeWeights(bernouliCIAgrestiCoullLowerBound(weights, trials, confidence));

    // make sure that all of the probability estimates are at least a little above zero (if requested)
    if (minimum>=0)
    {
        for (vector<double>::iterator it=conservativeWeights.begin();it!=conservativeWeights.end();it++)
        {
            *it = max(*it, minimum);
        }
    }

    // phase zero is, for now, already conservatively estimated at the end of FFluxPhaseOutputWrap::addEndPointPhaseZero
    // TODO: implement full on resampling based conservative estimation for phase zero weight
    conservativeWeights[0] = weights[0];

    return conservativeWeights;
}

vector<uint64_t> FFluxSupervisor::optimizeTrajectoryCounts(double errorGoal, double errorGoalConfidence, const lm::protowrap::FFluxStageOutputWrap& stageOutput, uint64_t minimumCount, double phaseZeroSamplingMultiplier, bool minimizeCost)
{
    vector<double> weights(estimateBernoulliProbabilities(stageOutput));
    vector<double> variances(stageOutput.fflux_stage_output_raw().variances().begin(), stageOutput.fflux_stage_output_raw().variances().end());

    vector<uint64_t> trajectoryCounts;
    if (minimizeCost)
    {
        vector<double> costVector(stageOutput.fflux_stage_output_summary().costs().begin(), stageOutput.fflux_stage_output_summary().costs().end());
        trajectoryCounts = minimizeCostTrajectoryCounts(errorGoal, errorGoalConfidence, weights, variances, costVector);
    }
    else
    {
        trajectoryCounts = minimizeCountTrajectoryCounts(errorGoal, errorGoalConfidence, weights, variances);
    }

    // "correct" undersampling durring phase zero
    vector<uint64_t>::iterator it=trajectoryCounts.begin();
    *it = static_cast<uint64_t>(round((*it)*phaseZeroSamplingMultiplier));

    for (;it!=trajectoryCounts.end();it++) if (*it < minimumCount) *it=minimumCount;
    return trajectoryCounts;
}

vector<uint64_t> FFluxSupervisor::minimizeCostTrajectoryCounts(double errorGoal, double errorGoalConfidence, const vector<double>& weights, const vector<double>& variances, const vector<double>& costVector)
{
    valarray<double> constantFactors(getConstantFactors(weights, variances));
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

vector<uint64_t> FFluxSupervisor::minimizeCountTrajectoryCounts(double errorGoal, double errorGoalConfidence, const vector<double>& weights, const vector<double>& variances)
{
    valarray<double> constantFactors(getConstantFactors(weights, variances));

    constantFactors *= pow(normalZ(errorGoalConfidence)/errorGoal, 2)*(constantFactors.sum());

    vector<uint64_t> trajectoryCounts;
    for (int i=0;i<constantFactors.size();i++)
    {
        trajectoryCounts.push_back(static_cast<uint64_t>(ceil(constantFactors[i])));
    }
    return trajectoryCounts;
}

valarray<double> FFluxSupervisor::getConstantFactors(const vector<double>& weights, const vector<double>& variances)
{
    valarray<double> constantFactors(weights.data(), weights.size());

    // calculate (1 - p)/p, the constant factor in the FFPilot optimizing equation. Doesn't produce the correct value for phase zero
    constantFactors = (1.0 - constantFactors)/constantFactors;

    // calculate variance/(weight^2), the phase zero constant factor
    constantFactors[0] = variances[0]/(pow(weights[0], 2));

    // take the square root
    constantFactors = sqrt(constantFactors);

    return constantFactors;
}

void FFluxSupervisor::startSimulationPhase()
{
    // add a new phase output
    addFFluxPhaseOutput();

    // set the first trajectory id of this phase in the phase output. If there is an old trajectoryList, get the trajectory count from that. Otherwise, we're at the very start of the simulation so count is 0
    currentFFluxPhaseOutputWrapPtr->set_first_trajectory_id(trajectoryList != NULL ? trajectoryList->count() : 0);

    // Build the list of trajectories to simulate.
    buildTrajectoryList();

    // reset the phase output and termination flag
    simulationPhaseOutputSent = false;
    ffluxPhaseTerminated = false;

    // print an info message about the phase we're starting up
    Print::printf(Print::INFO, "Forward Flux phase %3d started (%s)", currentFFluxPhaseID(), phaseInfo().c_str());

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

    // add a new phase output and set some informational fields
    lm::fflux::io::FFluxPhaseOutput* newFFluxPhaseOutputPtr = currentFFluxPhaseOutputsWrap.Add();
    newFFluxPhaseOutputPtr->set_phase_id(currentFFluxPhaseID());
    newFFluxPhaseOutputPtr->set_basin_id(currentStage().basin_id());
    newFFluxPhaseOutputPtr->set_tiling_id(currentStage().tiling_id());

    // set the new phase output to be the current phase output
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
        setTrajectoryList(new FFluxTrajectoryList(currentTrajectoryCount, currentFFluxPhaseID(), currentPhase(), currentPhaseLimit(), slots.getSimultaneousWorkUnits(), *input));  //, &ffluxPhaseOutputMsgCustom));
    }
    else if(currentFFluxPhaseID()==0)
    {
        setTrajectoryList(new FFluxTrajectoryList(currentTrajectoryCount, currentFFluxPhaseID(), currentPhase(), currentPhaseLimit(), slots.getSimultaneousWorkUnits(), *input, currentTiling().currentBasin()));
    }
    else
    {
        setTrajectoryList(new FFluxTrajectoryList(currentTrajectoryCount, currentFFluxPhaseID(), currentPhase(), currentPhaseLimit(), slots.getSimultaneousWorkUnits(), *input, previousPhaseOutput()));
    }
}

bool FFluxSupervisor::_terminateSimulationPhase()
{
    if (not ffluxPhaseTerminated)
    {
        switch(mutableCurrentPhaseLimit()->stop_condition())
        {
        case FFPhaseLimEnums::FORWARD_FLUXES:
            ffluxPhaseTerminated = (currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_count()>=mutableCurrentPhaseLimit()->uvalue());
            break;
        case FFPhaseLimEnums::TRAJECTORY_COUNT:
            // all phases during a forward flux simulation need to record at least one forward crossing or else it can't continue
            ffluxPhaseTerminated = ((currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_count() > 0) and \
                                         (currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_count() + currentFFluxPhaseOutputWrapPtr->wrappedMsg()->failed_trajectories_launched_count()>=mutableCurrentPhaseLimit()->uvalue()));
            break;
        case FFPhaseLimEnums::TIME:
            // all phases during a forward flux simulation need to record at least one forward crossing or else it can't continue
            ffluxPhaseTerminated = ((currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_count() > 0) and \
                                         (currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_total_time() + currentFFluxPhaseOutputWrapPtr->wrappedMsg()->failed_trajectories_launched_total_time()>=mutableCurrentPhaseLimit()->dvalue()));
            break;
        default: throw UnimplementedException("unimplemented");
        }
    }
    printFFluxLimitProgress();

    return ffluxPhaseTerminated;
}

void FFluxSupervisor::printFFluxLimitProgress()
{
    if (not ffluxPhaseTerminated)
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
                    currentFFluxPhaseID(),
                    FFPhaseLimEnums::StopCondition_Name(mutableCurrentPhaseLimit()->stop_condition()).c_str(),
                    currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_count(),
                    mutableCurrentPhaseLimit()->uvalue());
                break;
            case FFPhaseLimEnums::TRAJECTORY_COUNT:
                Print::printf(Print::INFO, "%10lld %-17s %12d %12d",
                    currentFFluxPhaseID(),
                    FFPhaseLimEnums::StopCondition_Name(mutableCurrentPhaseLimit()->stop_condition()).c_str(),
                    currentFFluxPhaseOutputWrapPtr->wrappedMsg()->successful_trajectories_launched_count() + currentFFluxPhaseOutputWrapPtr->wrappedMsg()->failed_trajectories_launched_count(),
                    mutableCurrentPhaseLimit()->uvalue());
                break;
            case FFPhaseLimEnums::TIME:
                Print::printf(Print::INFO, "%10lld %-17s %12.2e %12.2e",
                    currentFFluxPhaseID(),
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

    // if we need to perform another phase, initialize the next phase and switch over to it
    if (incrementSimulationPhase())
        startSimulationPhase();
    // otherwise, finish up this stage of the simulation
    else
        finishSimulationStage();
}

void FFluxSupervisor::sendSimulationPhaseOutput()
{
    if (not simulationPhaseOutputSent)
    {
        // first set the final trajectory id of this phase in the phase output (subtracting one from count since trajectoryList postcrements to get a trajectory_id)
        currentFFluxPhaseOutputWrapPtr->set_final_trajectory_id(trajectoryList->count() - 1);

        // send the phase output to the output writer
        if ((not currentStage().is_pilot_stage()) or input->ffluxOptions().pilot_stage_output())
        {
            if (input->ffluxOptions().phase_output())
            {
                // (re)initialize the relevant output options
                input->reinitOutputOptions(phaseInfo(true), currentStage().is_pilot_stage());

                // create a handle to the relevant work unit output part
                lm::message::WorkUnitOutput* wuoPart(ffluxPhaseOutputContainingMsg.mutable_process_work_unit_output()->mutable_part_output(0));

                // set the data and output options the work unit output part
                wuoPart->set_condense_output(input->getOutputOptionsMsg().condense_output());
                wuoPart->set_record_name_prefix(input->getOutputOptionsMsg().record_name_prefix());

                // temporarily hand off the allocated phase output and send it
                wuoPart->mutable_work_unit_output_generic()->mutable_fflux_phase_outputs()->AddAllocated(currentFFluxPhaseOutputWrapPtr->wrappedMsg());
                communicator->sendMessage(outputWriterAddress, &ffluxPhaseOutputContainingMsg);
                wuoPart->mutable_work_unit_output_generic()->mutable_fflux_phase_outputs()->ReleaseLast();
            }
        }
        // even if we're not actually sending phase output, make sure the sent flag is set after sendSimulationPhaseOutput has run. Needed for setting the phase's final trajectory id
        simulationPhaseOutputSent = true;
    }
}

bool FFluxSupervisor::incrementSimulationPhase()
{
    if (isCurrentPhaseLast())
    {
        return false;
    }
    else
    {
        // increment the currentFFluxPhase iterator
        currentFFluxPhaseIter++;

        // set the new phase ID
        simulationPhaseID = currentFFluxPhaseID();

        // call the base class method
        lm::main::SimulationSupervisor::incrementSimulationPhase();

        return true;
    }
}

void FFluxSupervisor::finishSimulationStage()
{
    // send the stage output to the output writer (if needed)
    sendSimulationStageOutput();

    // if we need to perform another stage, increment the stage-related iterators
    if (incrementSimulationStage())
        startSimulationStage();
    // otherwise, stop the simulation
    else
        finishSimulation();
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

        // add some stage output to the log file
        if (not currentStage().is_pilot_stage())
        {
            // pilot stage log output is handled in .addFFluxPhaseLimitsFromStageOutput(...)
            Print::printf(Print::INFO, stageLogProduction(currentFFluxStageOutputWrap).c_str());
        }

        if ((not currentStage().is_pilot_stage()) or input->ffluxOptions().pilot_stage_output())
        {
            // (re)initialize the relevant output options
            input->reinitOutputOptions(currentStageInfo(true), currentStage().is_pilot_stage());

            if (input->ffluxOptions().stage_output_raw())
            {
                // create a handle to the relevant work unit output part
                lm::message::WorkUnitOutput* wuoPart(ffluxStageOutputRawContainingMsg.mutable_process_work_unit_output()->mutable_part_output(0));

                // set the data and output options the work unit output part
                wuoPart->set_condense_output(input->getOutputOptionsMsg().condense_output());
                wuoPart->set_record_name_prefix(input->getOutputOptionsMsg().record_name_prefix());

                // temporarily hand off the allocated stage output and send it
                wuoPart->mutable_work_unit_output_generic()->mutable_fflux_stage_output_raws()->AddAllocated(currentFFluxStageOutputWrap.mutable_fflux_stage_output_raw()->mutableWrappedMsg());
                communicator->sendMessage(outputWriterAddress, &ffluxStageOutputRawContainingMsg);
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
                communicator->sendMessage(outputWriterAddress, &ffluxStageOutputSummaryContainingMsg);
                wuoPart->mutable_work_unit_output_generic()->mutable_fflux_stage_output_summaries()->ReleaseLast();
                simulationStageOutputSent = true;
            }
        }
    }
}

bool FFluxSupervisor::incrementSimulationStage()
{
    if (isCurrentStageLast())
    {
        return false;
    }
    else
    {
        // increment the currentFFluxPhase iterator
        currentFFluxStageIter++;

        return true;
    }
}

void FFluxSupervisor::finishSimulation()
{
    Print::printf(Print::INFO, "Forward Flux supervisor finished in %0.2f seconds.", timeElapsed());
    lm::main::SimulationSupervisor::finishSimulation();
}

void FFluxSupervisor::receivedFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg)
{
    PROF_BEGIN(PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT);

    // deal with the individual parts of the work unit at the fflux supervisor level
    if (currentFFluxPhaseID()==0)
    {
        for (int i=0;i<msg.part_status_size();i++)
        {
            receivedFinishedWorkUnitPartPhaseZero(msg.part_status(i));
        }
    }
    else
    {
        for (int i=0;i<msg.part_status_size();i++)
        {
            receivedFinishedWorkUnitPart(msg.part_status(i));
        }
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

void FFluxSupervisor::receivedFinishedWorkUnitPartPhaseZero(const lm::message::WorkUnitStatus& wusMsg)
{
    if (not trajectoryList->isTrajectoryAborted(wusMsg.final_state().trajectory_id()))
    {
        PROF_BEGIN(PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT_PHASE_ZERO);

        // get the relevant Trajectory instance
        lm::trajectory::Trajectory* trajectory = trajectoryList->getTrajectoryForFinishedWorkUnit(wusMsg.final_state().trajectory_id());

        // keep track of how much time each phase 0 trajectory spent in the region of a basin other than its initial basin
        lm::fflux::FFluxPhaseZeroTrajectory* phaseZeroTrajectory = static_cast<lm::fflux::FFluxPhaseZeroTrajectory*>(trajectory);
        phaseZeroTrajectory->processState(wusMsg.final_state());

        // update the phase output
        currentFFluxPhaseOutputWrapPtr->addEndPointPhaseZero(wusMsg.final_state(), trajectory, input->ffluxOptions().phase_zero_burn_in_count());

        PROF_END(PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT_PHASE_ZERO);
    }
}

void FFluxSupervisor::receivedFinishedWorkUnitPart(const lm::message::WorkUnitStatus& wusMsg)
{
    if (not trajectoryList->isTrajectoryAborted(wusMsg.final_state().trajectory_id()) and wusMsg.status()==lm::message::WorkUnitStatus::LIMIT_REACHED)
    {
        PROF_BEGIN(PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT_PHASE_N);

        // update the phase output
        currentFFluxPhaseOutputWrapPtr->addEndPoint(wusMsg.final_state(), *trajectoryList->getTrajectoryForFinishedWorkUnit(wusMsg.final_state().trajectory_id()));

        PROF_END(PROF_FFLUX_RECEIVED_FINISHED_WORK_UNIT_PHASE_N);
    }
}

// accessors
/*
 * a short string with some info about the current phase
 * phase and stage are const, but we use pointers instead of ref in order to allow passing of NULL
 */
std::string FFluxSupervisor::phaseInfo(bool path, const lm::fflux::input::FFluxPhase* phase, const lm::fflux::input::FFluxStage* stage) const
{
    // if a phase and/or a stage has not been passed, use the current ones
    const lm::fflux::input::FFluxPhase& _phase(phase!=NULL ? *phase : currentPhase());
    const lm::fflux::input::FFluxStage& _stage(stage!=NULL ? *stage : currentStage());
    int64_t phaseID = _phase.phase_id();
    
    stringstream phaseInfo;
    if (path)
    {
        phaseInfo << currentStageInfo(true, stage);
        phaseInfo << "/Phases/" << phaseID;    //setfill('0') << setw(7) << phaseID;
    }
    else
    {
        phaseInfo.setf(std::ios::fixed, std::ios::floatfield);
        phaseInfo.precision(2);
        if (currentFFluxPhaseID()==0)
        {
            phaseInfo << "zeroth_edge: " << setw(7) << _stage.tiling().edges(0);
        }
        else
        {
            phaseInfo << "starting_edge: " << setw(7) << _stage.tiling().edges(phaseID - 1);
            phaseInfo << ", goal_edge: " << setw(7) << _stage.tiling().edges(phaseID);
        }
        
        const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit(_phase.has_fflux_phase_limit() ? _phase.fflux_phase_limit() : _stage.fflux_phase_limits(phaseID));
        phaseInfo << ", phase_limit: " << FFPhaseLimEnums::StopCondition_Name(ffluxPhaseLimit.stop_condition());
        phaseInfo << " >= " << (ffluxPhaseLimit.stop_condition()==FFPhaseLimEnums::TIME ? ffluxPhaseLimit.dvalue() : ffluxPhaseLimit.uvalue());
    }
    
    return phaseInfo.str();
}

/*
 * a short string with some info about the current stage
 * stage is const, but we use pointers instead of ref in order to allow passing of NULL
 */
std::string FFluxSupervisor::currentStageInfo(bool path, const lm::fflux::input::FFluxStage* stage) const
{
    // if a stage has not been passed, use the current one
    const lm::fflux::input::FFluxStage& _stage(stage!=NULL ? *stage : currentStage());
    
    stringstream stageInfo;
    if (path)
    {
        stageInfo << "/Simulations/" << _stage.replicate_id();
        stageInfo << "/Tilings/"     << _stage.tiling().id();     //setfill('0') << setw(7) << _stage.tiling().id();
        stageInfo << "/Basins/"      << _stage.tiling().current_basin_id();    //setfill('0') << setw(7) << _stage.tiling().current_basin_id();
        stageInfo << "/Stages/"      << _stage.name();
    }
    else 
    {
        stageInfo << "replicate_id: " << _stage.replicate_id();
        stageInfo << ", tiling_id: "  << _stage.tiling().id();
        stageInfo << ", basin_id: "   << _stage.tiling().current_basin_id();
        stageInfo << ", stage_type: " << _stage.name();
    }

    return stageInfo.str();
}

std::string FFluxSupervisor::stageLogPilot(const lm::protowrap::FFluxStageOutputWrap& stageOutput, double errorGoal, double errorGoalConfidence, vector<uint64_t>& trajectoryCounts) const
{
    stringstream stageLog;
    stageLog.unsetf(std::ios::floatfield);                  // allow for dynamic choice between float and sci format
    //stageLog.setf(std::ios::fixed, std::ios::floatfield); // force float format
    stageLog.precision(3);

    stageLog << "Pilot stage output:\n";
    stageLog << "The phase costs are:\n" << stageOutput.fflux_stage_output_summary().costs() << "\n";
    stageLog << "The phase weight sample variances are:\n" << stageOutput.fflux_stage_output_raw().variances() << "\n";
    stageLog << "Conservative estimates of the phase weights are:\n" << estimateBernoulliProbabilities(stageOutput) << "\n";
    stageLog << "Attempting to achieve error goal " << errorGoal << " (confidence level " << errorGoalConfidence << ") with the following optimized trajectory counts:\n" << trajectoryCounts << "\n";

    return stageLog.str();
}

std::string FFluxSupervisor::stageLogProduction(const lm::protowrap::FFluxStageOutputWrap& stageOutput) const
{
    stringstream stageLog;
    stageLog.unsetf(std::ios::floatfield);
    stageLog.precision(3);

    stageLog << "Production stage output:\n";
    stageLog << "The phase costs are:\n" << stageOutput.fflux_stage_output_summary().costs() << "\n";
    stageLog << "The phase weights are:\n" << stageOutput.fflux_stage_output_summary().weights() << "\n";
    stageLog << "The first passage times to each tile edge are:\n" << stageOutput.fflux_stage_output_summary().first_passage_times() << "\n";
    stageLog << "The overall first passage time from the starting basin to the last tile edge is:\n" << stageOutput.fflux_stage_output_summary().first_passage_times(stageOutput.fflux_stage_output_summary().first_passage_times_size() - 1) << "\n";

    return stageLog.str();
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
