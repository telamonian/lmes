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

namespace lm {
namespace fflux {
namespace input {

FFluxInput::FFluxInput(): _ffluxPhaseLimitLists(_ffluxOptions.mutable_fflux_phase_limit_lists())
{
    stepsPerWorkUnit = (int)1e15;
}

FFluxInput::FFluxInput(const lm::io::hdf5::Hdf5File& file): _ffluxPhaseLimitLists(_ffluxOptions.mutable_fflux_phase_limit_lists())
{
    stepsPerWorkUnit = (int)1e15;
    init(file);
}

void FFluxInput::init(const lm::io::hdf5::Hdf5File& file)
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
}

// Get the Forward Flux specific options.
void FFluxInput::initFFluxOptions(const lm::io::hdf5::Hdf5File& file)
{
    parseAndSet("precisionGoal", &FFluxOptions::set_precision_goal, _ffluxOptions);
    parseAndSet("precisionGoalConfidence", &FFluxOptions::set_precision_goal_confidence, _ffluxOptions);

    parseAndSet("pilotStageCount", &FFluxOptions::set_pilot_stage_count, _ffluxOptions);
    parseAndSet("phaseZeroBurnInCount", &FFluxOptions::set_phase_zero_burn_in_count, _ffluxOptions);

    parseAndSet("ffluxPilotOutput", &FFluxOptions::set_pilot_stage_output, _ffluxOptions);
    parseAndSet("ffluxPhaseOutput", &FFluxOptions::set_phase_output, _ffluxOptions);
    parseAndSet("ffluxStageOutputRaw", &FFluxOptions::set_stage_output_raw, _ffluxOptions);
    parseAndSet("ffluxStageOutputSummary", &FFluxOptions::set_stage_output_summary, _ffluxOptions);

    // if we haven't gotten a precisionGoal or any user defined phase limits, explicitly set precisionGoal so that hasPrecisionGoal() returns true
    if ((not hasPrecisionGoal()) and (not hasUserDefinedFFluxPhaseLimitLists()))
    {
        _ffluxOptions.set_precision_goal(_ffluxOptions.default_instance().precision_goal());
        //_ffluxOptions.set_precision_goal(_ffluxOptions.GetDescriptor()->FindFieldByName("precision_goal")->default_value_double());
    }

    // check the fflux options we just parsed for consistency
    if (hasPrecisionGoal() and hasUserDefinedFFluxPhaseLimitLists()) throw ConsistencyException("precisionGoal and an explicit set of ffluxPhaseLimits cannot both be set in forward flux simulation input");
}

void FFluxInput::reinitOutputOptions(const std::string& recordNamePrefix)
{
    outputOptionsMsg.Clear();

    outputOptionsMsg.set_record_name_prefix(pathJoin(recordNamePrefixGlobal, recordNamePrefix));
    outputOptionsMsg.set_condense_output(true);
    outputOptionsMsg.set_write_initial_trajectory_state(false);
    outputOptionsMsg.set_write_final_trajectory_state(false);

    // Specify how often the species counts should be written to output
    parseAndSet("writeInterval", &lm::input::OutputOptions::set_species_write_interval, outputOptionsMsg);

    // Specify how often the species counts at all of the lattice points should be written out during an RDME simulation
    parseAndSet("latticeWriteInterval", &lm::input::OutputOptions::set_lattice_write_interval, outputOptionsMsg);

    // Specify how often various (optional) specialized simulation outputs should be written out. Leave unset to supress these outputs completely.
    degreeAdvancementPresent = parseAndSet("degreeAdvancementWriteInterval", &lm::input::OutputOptions::set_degree_advancement_write_interval, outputOptionsMsg);
    parseAndSet("orderParameterWriteInterval", &lm::input::OutputOptions::set_order_parameter_write_interval, outputOptionsMsg);
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
        limitTrackingListWrap.addTrackingMsg(trajectoryLimits.findMsg(0), false, true, ffluxPhaseLimit.events_per_trajectory());
        limitTrackingListWrap.addTrackingMsg(trajectoryLimits.findMsg(1), false, true, ffluxPhaseLimit.events_per_trajectory());
    }
}

void FFluxInput::reinitTrajectoryLimitsPhaseZero(const lm::fflux::input::FFluxPhase& ffluxPhase, const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit, const lm::tiling::Tiling& tiling)
{
    trajectoryLimits.Clear();
    limitTrackingListWrap.Clear();

    // - first we set a limit with id==0
    //     - this limit is the important one. a triggering of this limit corresponds to one of the flux events that we're trying to sample during phase 0
    trajectoryLimits.addTileExitLimitsMsg(tiling, -1, 0, false, true);
    limitTrackingListWrap.addTrackingMsg(trajectoryLimits.findMsg(0), false, true, ffluxPhaseLimit.events_per_trajectory());

    // - next, we set two more limits with id==1 and id==2
    //     - these limits are used to help track which basin was last visited by a trajectory
    //     - limit_id==1: tracks flux back into the starting basin
    //     - limit_id==2: tracks flux into the basin opposite from the starting basin
    trajectoryLimits.addTileExitLimitsMsg(tiling, 0, tiling.edges().lastIndex());
    limitTrackingListWrap.addTrackingMsgNonterminating(trajectoryLimits.findMsg(1), false, true);
    limitTrackingListWrap.addTrackingMsgNonterminating(trajectoryLimits.findMsg(2), false, true);
}

//bool FFluxInput::parseAndSetFFluxPhaseLimit(const std::string key, const std::string debugString)
//{
//    if (simulationParameters.count(key))
//    {
//        typename PairVector<uint, typename LimitElement<LT>::type>::T idLimitVec(simulationParameters.parsePairVector<uint, typename LimitElement<LT>::type>(key, debugString));
//        for (typename PairVector<uint, typename LimitElement<LT>::type>::iterator it(idLimitVec.begin()); it!=idLimitVec.end(); it++)
//        {
//            trajectoryLimits.addLimitMsg<LT>(it->first, it->second, sc, includeEndpoint);
//        }
//        return idLimitVec.size() > 0;
//    }
//    else
//    {
//        return false;
//    }
//}

}
}
}