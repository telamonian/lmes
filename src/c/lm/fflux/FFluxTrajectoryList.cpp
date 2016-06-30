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
#include <algorithm>
#include <cmath>
#include <csignal>
#include <functional>
#include <list>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include "lm/EnumHelper.h"
#include "lm/Exceptions.h"
#include "lm/fflux/FFluxTrajectoryList.h"
#include "lm/fflux/input/FFluxPhase.pb.h"
#include "lm/io/CMEState.pb.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/FFluxOutput.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/main/Globals.h"
#include "lm/main/Main.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/Print.h"
#include "lm/protowrap/Repeated.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/tiling/Tilings.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using lm::input::DiffusionModel;
using lm::input::ReactionModel;
using lm::protowrap::Repeated;
using std::map;
using std::string;
using std::vector;

namespace lm {
namespace fflux {

FFluxTrajectoryList::FFluxTrajectoryList(const lm::fflux::input::FFluxInput& input, const lm::fflux::input::FFluxPhase& ffluxPhase, const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit, uint64_t totalSlots)
{

}

FFluxTrajectoryList::FFluxTrajectoryList(const lm::fflux::input::FFluxInput& input, const lm::fflux::input::FFluxPhase& ffluxPhase, const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit, uint64_t totalSlots, const lm::protowrap::FFluxPhaseOutput& previousPhaseOutput)
{

    for (uint64_t i=0;i<getTrajectoriesToStart(ffluxPhase, ffluxPhaseLimit, totalSlots);i++)
    {
        initTrajectory(input, simulationPhaseIndex, DEFAULT_TRAJECTORY_ID);
    }
}

uint64_t FFluxTrajectoryList::getTrajectoriesToStart(const lm::fflux::input::FFluxPhase& ffluxPhase, const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit, uint64_t totalSlots)
{
    if (ffluxPhase.trajectory_generation()==FFluxPhaseEnums::EAGER)
    {
        // EAGER is only implemented for certain ffluxPhaseLimit.stop_condition() values
        if (ffluxPhaseLimit.stop_condition()==FFPhaseLimEnums::TRAJECTORY_COUNT)
        {
            return ffluxPhaseLimit.uvalue();
        }
        else throw UnimplementedException("ffluxPhase.trajectory_generation()==EAGER is only implemented for certain ffluxPhaseLimit.stop_condition() values (ie those that let us calculate the necessary trajectory count up front). Attempting to use unimplemented ffluxPhaseLimit.stop_condition(): %d", ffluxPhaseLimit.stop_condition());
    }
    else if (ffluxPhase.trajectory_generation()==FFluxPhaseEnums::LAZY)
    {
        #ifdef OPT_AVX

        // TODO: replace this kludge that detects whether an AVX-type solver has been selected
        string solverClassNameLower;
        std::transform(solverClassName.begin(), solverClassName.end(), solverClassNameLower, ::tolower);
        return totalSlots*ffluxPhase.batch_size() * ((solverClassNameLower.find("avx")!=string::npos) ? DOUBLES_PER_AVX : 1);

        #else
        return totalSlots*ffluxPhase.batch_size();
        #endif
    }
}

}
}
