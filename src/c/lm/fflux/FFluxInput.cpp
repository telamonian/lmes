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
#include "lm/fflux/FFluxInput.h"
#include "lm/fflux/input/FFluxOptions.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/OutputOptions.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/option/SimulationParameters.h"
#include "lm/Print.h"
#include "lm/limit/TrajectoryLimits.h"
#include "lm/Types.h"

using lm::fflux::input::FFluxOptions;
using lm::limit::LimitValueT;

namespace lm {
namespace fflux {

FFluxInput::FFluxInput(const lm::io::hdf5::Hdf5File& file): Input(file),_ffluxPhaseLimitLists(_ffluxOptions.mutable_fflux_phase_limit_lists()) {}

void FFluxInput::init(const lm::io::hdf5::Hdf5File& file)
{
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

    parseAndSet("phaseZeroBurnInCount", &FFluxOptions::set_phase_zero_burn_in_count, _ffluxOptions);

    // set a default precision
    if (not hasPrecisionGoal() and not hasUserDefinedFFluxPhaseLimitLists()) _ffluxOptions.set_precision_goal(.05);

    // check the fflux options we just parsed for consistency
    if (hasPrecisionGoal() and hasUserDefinedFFluxPhaseLimitLists()) throw ConsistencyException("precisionGoal and an explicit set of ffluxPhaseLimits cannot both be set in forward flux simulation input");
}

void FFluxInput::reinitOutputOptions(std::string& recordNamePrefix)
{
    outputOptions.Clear();

    outputOptions.set_record_name_prefix(recordNamePrefix);
    outputOptions.set_condense_output(true);
}

//bool FFluxInput::parseAndSetFFluxPhaseLimit(const std::string key, const std::string debugString)
//{
//    if (simulationParameters.count(key))
//    {
//        typename PairVector<uint, typename LimitValueT<LT>::type>::T idLimitVec(simulationParameters.parsePairVector<uint, typename LimitValueT<LT>::type>(key, debugString));
//        for (typename PairVector<uint, typename LimitValueT<LT>::type>::iterator it(idLimitVec.begin()); it!=idLimitVec.end(); it++)
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
