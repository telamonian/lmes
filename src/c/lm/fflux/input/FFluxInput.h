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
 * Author(s): Max Klein
 */
#ifndef LM_FFLUX_FFLUXINPUT_H_
#define LM_FFLUX_FFLUXINPUT_H_

#include <list>
#include <map>
#include <string>

#include "lm/fflux/input/FFluxOptions.pb.h"
#include "lm/fflux/input/FFluxPhase.pb.h"
#include "lm/fflux/input/FFluxPhaseLimit.pb.h"
#include "lm/fflux/input/FFluxStage.pb.h"
#include "lm/fflux/io/FFluxStageOutput.pb.h"
#include "lm/input/Input.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/BoundaryConditions.pb.h"
#include "lm/input/DiffusionModel.pb.h"
#include "lm/input/OrderParameters.pb.h"
#include "lm/input/OutputOptions.pb.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/input/SimulationParameters.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/oparam/OParams.h"
#include "lm/option/SimulationParameters.h"
#include "lm/tiling/Tilings.h"
#include "lm/limit/TrajectoryLimits.h"
#include "lm/Types.h"

namespace lm {
namespace fflux {
namespace input {

class FFluxInput : public lm::input::Input
{
public:
    FFluxInput();
    FFluxInput(const lm::io::hdf5::Hdf5File& file);
    virtual ~FFluxInput() {};

// (re)initializers
    virtual void reinitOutputOptions(const std::string& recordNamePrefix, bool isPilotStage);
    virtual void reinitTrajectoryLimits(const lm::fflux::input::FFluxPhase& ffluxPhase, const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit, const lm::tiling::Tiling& tiling);
    virtual void reinitTrajectoryLimitsPhaseZero(const lm::fflux::input::FFluxPhase& ffluxPhase, const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit, const lm::tiling::Tiling& tiling);

// accessors
    const lm::fflux::input::FFluxOptions& ffluxOptions() const {return _ffluxOptions;}

    uint64_t batchSize() const {return ffluxOptions().batch_size();}
    bool minimizeCost() const {return ffluxOptions().minimize_cost();}
    uint64_t phaseZeroBurnInCount() const {return ffluxOptions().phase_zero_burn_in_count();}
    uint64_t phaseZeroSamplingMultiplier() const {return ffluxOptions().phase_zero_sampling_multiplier();}
    uint64_t pilotStageCount() const {return ffluxOptions().pilot_stage_count();}
    double errorGoal() const {return ffluxOptions().error_goal();}
    double errorGoalConfidence() const {return ffluxOptions().error_goal_confidence();}
    uint64_t productionStageCountMinimum() const {return ffluxOptions().production_stage_count_minimum();}
    const lm::protowrap::Repeated<lm::fflux::input::FFluxPhaseLimitList>& userDefinedFFluxPhaseLimitLists() const {return _ffluxPhaseLimitLists;}

    bool hasErrorGoal() const {return ffluxOptions().has_error_goal();}
    bool hasErrorGoalConfidence() const {return ffluxOptions().has_error_goal_confidence();}
    bool hasUserDefinedFFluxPhaseLimitLists() const {return (ffluxOptions().fflux_phase_limit_lists_size() > 0);}
    bool hasPilotStageCount() const {return ffluxOptions().has_pilot_stage_count();}
    bool hasPhaseZeroBurnInCount() const {return ffluxOptions().has_phase_zero_burn_in_count();}

protected:
    virtual void readHDF5Input(const lm::io::hdf5::Hdf5File& file);
    virtual void initFFluxOptions(const lm::io::hdf5::Hdf5File& file);
//    bool parseAndSetFFluxPhaseLimit(const std::string key, const std::string debugString);

protected:
    lm::fflux::input::FFluxOptions _ffluxOptions;
    lm::protowrap::Repeated<lm::fflux::input::FFluxPhaseLimitList> _ffluxPhaseLimitLists;
};

}
}
}

#endif // LM_FFLUX_FFLUXINPUT_H_
