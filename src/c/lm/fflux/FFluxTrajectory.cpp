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
#include <csignal>
#include <cstdio>
#include <list>
#include <map>
#include <string>

#include "lm/fflux/FFluxTrajectory.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/Tilings.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

using lm::io::DiffusionModel;
using lm::io::ReactionModel;
using lm::io::TrajectoryState;
using std::map;
using std::string;

namespace lm {
namespace fflux {

FFluxTrajectory::FFluxTrajectory(uint64_t id, uint ffluxPhase, const lm::input::Input& input, bool reversed):
Trajectory(id,input,reversed),input(input),ffluxPhase(ffluxPhase)
{
    initLimits();
}

FFluxTrajectory::FFluxTrajectory(uint64_t id, uint ffluxPhase, const lm::input::Input& input, TrajectoryState* zerothState):
Trajectory(id,input,zerothState),input(input),ffluxPhase(ffluxPhase)
{
    // Limit setting code
    initLimits();
}

FFluxTrajectory::~FFluxTrajectory()
{
}

bool FFluxTrajectory::fluxedBackward()
{
    if (input.getTilings().getCurrentTiling()->getArrangement()==lm::io::Tilings::ASCENDING)
    {
        return (getFinalLimitType()==lm::io::TrajectoryLimits::DECREASINGORDERPARAMETER);
    }
    else
    {
        return (getFinalLimitType()==lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER);
    }
}

bool FFluxTrajectory::fluxedForward()
{
    if (input.getTilings().getCurrentTiling()->getArrangement()==lm::io::Tilings::ASCENDING)
    {
        return (getFinalLimitType()==lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER);
    }
    else
    {
        return (getFinalLimitType()==lm::io::TrajectoryLimits::DECREASINGORDERPARAMETER);
    }
}

lm::io::TrajectoryLimits::LimitType FFluxTrajectory::getFinalLimitType()
{
    return state.limit_reached();
}

void FFluxTrajectory::getLastSpeciesCounts(lm::io::FFluxOutput::TrajectoryOutput* trajectoryOutputBuf)
{
    lm::io::SpeciesCounts speciesCounts = state.cme_state().species_counts();
    uint offset = (speciesCounts.number_entries() - 1)*(speciesCounts.number_species());
    for (int i=0; i<speciesCounts.number_species(); i++)
    {
        trajectoryOutputBuf->add_species_count(speciesCounts.species_count(i + offset));
    }
}

uint FFluxTrajectory::getSimSteps()
{
    return state.cme_state().species_counts().number_entries();
}

double FFluxTrajectory::getSimTime()
{
    return state.cme_state().species_counts().time(state.cme_state().species_counts().time_size() - 1);
}

bool FFluxTrajectory::hasElapsed(double time)
{
    return (getSimTime()>=time);
}

void FFluxTrajectory::initLimits()
{
    /* TODO: this needs to be moved somewhere else, probably forward flux supervisor.
     *
    getRunMsg()->mutable_work_unit(0)->mutable_limits()->Clear();
    // we are dealing with a combinatoric case where both either ffluxPhase is zero or it isn't, and the tiling arrangment is ASCENDING or it isn't
    // each of the 4 sets of possible pairs of true/false values corresponds to one of the numbers 0-3
    switch ((ffluxPhase!=0)<<1|input.tilings.getCurrentTiling()->getArrangement()!=lm::io::Tilings::ASCENDING)
    {
    case 0: // ffluxphase==0 and tilings.getCurrentTiling().getArrangement()==lm::io::Tilings::ASCENDING
    {
        lm::io::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_increasing_order_parameter_limit();
        iopl->set_arrangement(lm::io::TrajectoryLimits::ASCENDING);
        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
        break;
    }
    case 1: // ffluxphase==0 and tilings.getCurrentTiling().getArrangement()==lm::io::Tilings::DESCENDING
    {
        lm::io::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_decreasing_order_parameter_limit();
        dopl->set_arrangement(lm::io::TrajectoryLimits::DESCENDING);
        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
        break;
    }
    case 2: // ffluxphase!=0 and tilings.getCurrentTiling().getArrangement()==lm::io::Tilings::ASCENDING
    {
        lm::io::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_decreasing_order_parameter_limit();
        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
        dopl->set_arrangement(lm::io::TrajectoryLimits::ASCENDING);
        lm::io::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_increasing_order_parameter_limit();
        iopl->set_arrangement(lm::io::TrajectoryLimits::ASCENDING);
        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(ffluxPhase));
        break;
    }
    case 3: // ffluxphase!=0 and tilings.getCurrentTiling().getArrangement()==lm::io::Tilings::DESCENDING
    {
        lm::io::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_increasing_order_parameter_limit();
        iopl->set_arrangement(lm::io::TrajectoryLimits::DESCENDING);
        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
        lm::io::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_decreasing_order_parameter_limit();
        dopl->set_arrangement(lm::io::TrajectoryLimits::DESCENDING);
        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(ffluxPhase));
        break;
    }
    }
    */
}

}
}
