/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
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
#include <map>
#include <string>
#include <vector>

#include "lm/EnumHelper.h"
#include "lm/fflux/input/FFluxInput.h"
#include "lm/fflux/input/FFluxOptions.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/OutputOptions.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/main/Globals.h"
#include "lm/option/SimulationParameters.h"
#include "lm/Print.h"
#include "lm/limit/TrajectoryLimits.h"
#include "lm/Types.h"

using lm::fflux::input::FFluxOptions;
using lm::limit::LimitElement;
using std::string;
using std::vector;

namespace lm {
namespace fflux {
namespace input {

//FFluxInput::FFluxInput(): _ffluxPhaseLimitLists(_ffluxOptions.mutable_fflux_phase_limit_lists())
//{
////    stepsPerWorkUnit = (uint64_t)1e15;
//}
//
//FFluxInput::FFluxInput(const lm::io::hdf5::Hdf5File& file): _ffluxPhaseLimitLists(_ffluxOptions.mutable_fflux_phase_limit_lists())
//{
////    stepsPerWorkUnit = (uint64_t)1e15;
//    readHDF5Input(file);
//}

FFluxInput::FFluxInput(): Input(), _ffluxPhaseLimitLists(_ffluxOptions.mutable_fflux_phase_limit_lists())
{
}

FFluxInput::FFluxInput(const vector<string>& inputFilenames): Input(), _ffluxPhaseLimitLists(_ffluxOptions.mutable_fflux_phase_limit_lists())
{
    init(inputFilenames);
}


void FFluxInput::readHDF5Input(const lm::io::hdf5::Hdf5File& file)
{
    simulationParameters.rFF(file);

    // run some initializers from the base class (but skip Limits and OutputOptions, as these need to be set every phase rather than just once)
    initReactionModel(file);
    initDiffusionModel(file);
    initOrderParameters(file);
    initTilings(file);
    initWorkUnitParameters(file);

    // run some fflux specific intializers
    initFFluxOptions(file);

    // although reinitOutputOptions() will be run at least once more before any related values are actually used, run it once here so the sanity check works correctly
    reinitOutputOptions("", false);

    // warn the user about any unrecognized/unparsed simulation parameters
    initSanityCheck();
}

// Get the Forward Flux specific options.
void FFluxInput::initFFluxOptions(const lm::io::hdf5::Hdf5File& file)
{
    parseAndSet("batchSize", &FFluxOptions::set_batch_size, _ffluxOptions);

    parseAndSet("errorGoal", &FFluxOptions::set_error_goal, _ffluxOptions);
    parseAndSet("errorGoalConfidence", &FFluxOptions::set_error_goal_confidence, _ffluxOptions);

    parseAndSet("pilotStageCount", &FFluxOptions::set_pilot_stage_count, _ffluxOptions);
    parseAndSet("productionStageCountMinimum", &FFluxOptions::set_production_stage_count_minimum, _ffluxOptions);
    parseAndSet("phaseZeroBurnInCount", &FFluxOptions::set_phase_zero_burn_in_count, _ffluxOptions);

    // TODO: figure out how to properly control landscape error and remove this
    parseAndSet("phaseZeroSamplingMultiplier", &FFluxOptions::set_phase_zero_sampling_multiplier, _ffluxOptions);

    parseAndSet("ffluxPilotOutput", &FFluxOptions::set_pilot_stage_output, _ffluxOptions);
    parseAndSet("ffluxPhaseOutput", &FFluxOptions::set_phase_output, _ffluxOptions);
    parseAndSet("ffluxStageOutputRaw", &FFluxOptions::set_stage_output_raw, _ffluxOptions);
    parseAndSet("ffluxStageOutputSummary", &FFluxOptions::set_stage_output_summary, _ffluxOptions);

    parseAndSet("ffluxMinimizeCost", &FFluxOptions::set_minimize_cost, _ffluxOptions);

    // if we haven't gotten a errorGoal or any user defined phase limits, explicitly set errorGoal so that hasErrorGoal() returns true
    if ((not hasErrorGoal()) and (not hasUserDefinedFFluxPhaseLimitLists()))
    {
        _ffluxOptions.set_error_goal(_ffluxOptions.default_instance().error_goal());
        //_ffluxOptions.set_error_goal(_ffluxOptions.GetDescriptor()->FindFieldByName("error_goal")->default_value_double());
    }

    // check the fflux options we just parsed for consistency
    if (hasErrorGoal() and hasUserDefinedFFluxPhaseLimitLists()) throw ConsistencyException("errorGoal and an explicit set of ffluxPhaseLimits cannot both be set in forward flux simulation input");
}

void FFluxInput::readSFileInput(lm::io::sfile::SFile& file)
{
    bool recordParsed;
    // Read all of the records.
    while (!file.isEof())
    {
        // Read the next record.
        recordParsed = false;
        lm::io::sfile::SFileRecord r = file.readNextSFileRecord();

        // See if this is an SimulationInput record.
        recordParsed |= readSFileInputRecord(file, r, "protobuf:lm.input.SimulationInput", simulationInput);

        // See if this is a FFluxSimulationInput record.
        recordParsed |= readSFileInputRecord(file, r, "protobuf:lm.fflux.input.FFluxSimulationInput", ffluxSimulationInput);

        if (not recordParsed)
        {
            // Skip the record.
            file.skip(r.dataSize);
        }
    }
}

void FFluxInput::reinitOutputOptions(const std::string& recordNamePrefix, bool isPilotStage)
{
    outputOptionsMsg.Clear();

    outputOptionsMsg.set_record_name_prefix(pathJoin(recordNamePrefixGlobal, recordNamePrefix));
    outputOptionsMsg.set_condense_output(true);

    // set output options for the pilot stage only if pilot stage output is explicitly requested
    if ((not isPilotStage) or ffluxOptions().pilot_stage_output())
    {
        // Flags that control whether output is recorded for the initial and/or the final state of every trajectory.
        bool defaultWriteState = false;
        parseAndSet("writeInitialTrajectoryState", &lm::input::OutputOptions::set_write_initial_trajectory_state, outputOptionsMsg, &defaultWriteState);
        parseAndSet("writeFinalTrajectoryState", &lm::input::OutputOptions::set_write_final_trajectory_state, outputOptionsMsg, &defaultWriteState);

        // Flag that globally controls whether any limit tracking data collected during a trajectory is written out directly to disk.
        parseAndSet("writeLimitTracking", &lm::input::OutputOptions::set_write_limit_tracking, outputOptionsMsg);

        // Specify the period at which various outputs should be written out. Leave a WriteInterval unset to suppress its related output
        degreeAdvancementPresent = parseAndSet("degreeAdvancementWriteInterval", &lm::input::OutputOptions::set_degree_advancement_write_interval, outputOptionsMsg);
        parseAndSet("latticeWriteInterval", &lm::input::OutputOptions::set_lattice_write_interval, outputOptionsMsg);
        parseAndSet("orderParameterWriteInterval", &lm::input::OutputOptions::set_order_parameter_write_interval, outputOptionsMsg);
        parseAndSet("writeInterval", &lm::input::OutputOptions::set_species_write_interval, outputOptionsMsg);
    }
    else
    {
        outputOptionsMsg.set_write_initial_trajectory_state(false);
        outputOptionsMsg.set_write_final_trajectory_state(false);
    }
}

void FFluxInput::reinitTrajectoryLimits(const lm::fflux::input::FFluxPhase& ffluxPhase, const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit, const lm::tiling::Tiling& tiling)
{
    if (ffluxPhase.fflux_phase_index()==0)
    {
        reinitTrajectoryLimitsPhaseZero(ffluxPhase, ffluxPhaseLimit, tiling);
    }
    else
    {
        trajectoryLimits.Clear();
        limitTrackingListWrap.Clear();

        // - if currentFFluxPhaseIndex() > 0, we can use addTileExitLimitsMsg() in a straightforward way to set the needed limits. Two limits are set:
        //     - if limit id==0 is triggered, this indicates that the trajectory fluxed backwards
        //     - if limit id==1 is triggered, this indicates that the trajectory fluxed forwards
        trajectoryLimits.addTileExitLimitsMsg(tiling, 0, ffluxPhase.fflux_phase_index());
        limitTrackingListWrap.addTrackingMsg(trajectoryLimits.findMsg(0), true, true, ffluxPhaseLimit.events_per_trajectory());
        limitTrackingListWrap.addTrackingMsg(trajectoryLimits.findMsg(1), true, true, ffluxPhaseLimit.events_per_trajectory());

//        printf(trajectoryLimits.findMsg(0)->DebugString().c_str());
//        printf(trajectoryLimits.findMsg(1)->DebugString().c_str());
    }
}

void FFluxInput::reinitTrajectoryLimitsPhaseZero(const lm::fflux::input::FFluxPhase& ffluxPhase, const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit, const lm::tiling::Tiling& tiling)
{
    trajectoryLimits.Clear();
    limitTrackingListWrap.Clear();

    // - first we set a limit with id==0
    //     - this limit is the important one. a triggering of this limit corresponds to one of the flux events that we're trying to sample during phase 0
    trajectoryLimits.addTileExitLimitsMsg(tiling, -1, 0, false, true);
    if (ffluxPhaseLimit.events_per_trajectory() < 0)
    {
        limitTrackingListWrap.addTrackingMsgNonterminating(trajectoryLimits.findMsg(0), true, true);
    }
    else
    {
        limitTrackingListWrap.addTrackingMsg(trajectoryLimits.findMsg(0), true, true, ffluxPhaseLimit.events_per_trajectory());
    }

    // - next, we set two more limits with id==1 and id==2
    //     - these limits are used to help track which basin was last visited by a trajectory
    //     - limit_id==1: tracks flux back into the starting basin
    //     - limit_id==2: tracks flux into the basin opposite from the starting basin
    trajectoryLimits.addTileExitLimitsMsg(tiling, 0, tiling.edges().lastIndex());
    limitTrackingListWrap.addTrackingMsgNonterminating(trajectoryLimits.findMsg(1), true, true);
    limitTrackingListWrap.addTrackingMsgNonterminating(trajectoryLimits.findMsg(2), true, true);

//    printf(trajectoryLimits.findMsg(0)->DebugString().c_str());
//    printf(trajectoryLimits.findMsg(1)->DebugString().c_str());
//    printf(trajectoryLimits.findMsg(2)->DebugString().c_str());
}

}
}
}