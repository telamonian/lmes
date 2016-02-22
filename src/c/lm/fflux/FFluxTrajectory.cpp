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
Trajectory(id,input,reversed),ffluxPhase(ffluxPhase),lastLimitTime(0.0)
{
    initLimits();
    // tied up with time tracking during fflux phase 0
    setFinalLimitID(0);
}

FFluxTrajectory::FFluxTrajectory(uint64_t id, uint ffluxPhase, const lm::input::Input& input, TrajectoryState* initialState):
Trajectory(id,input,initialState),ffluxPhase(ffluxPhase),lastLimitTime(getSimTime())
{
    // Limit setting code
    initLimits();
}

FFluxTrajectory::~FFluxTrajectory()
{
}

void FFluxTrajectory::initLimits()
{
    // TODO: this needs to be moved somewhere else, probably forward flux supervisor.
    getRunMsg()->mutable_work_unit(0)->mutable_limits()->Clear();
    // we are dealing with a combinatoric case where both either ffluxPhase is zero or it isn't, and the tiling arrangment is ASCENDING or it isn't
    // each of the 4 sets of possible pairs of true/false values corresponds to one of the numbers 0-3
    switch ((ffluxPhase!=0)<<1|input.tilings.getCurrentTiling()->getArrangement()!=lm::io::Tilings::ASCENDING)
    {
    case 0: // ffluxphase==0 and tilings.getCurrentTiling().getArrangement()==lm::io::Tilings::ASCENDING
    {
        // increasing edge 0 limit
        lm::io::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_limits()->add_increasing_order_parameter_limit();
        iopl->set_arrangement(lm::io::TrajectoryLimits::ASCENDING);
        iopl->set_limit_id(0);
        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));

        // decreasing edge 0 limit
        lm::io::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_limits()->add_decreasing_order_parameter_limit();
        dopl->set_arrangement(lm::io::TrajectoryLimits::ASCENDING);
        dopl->set_limit_id(0);
        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));

        // increasing final edge limit
        iopl = getRunMsg()->mutable_limits()->add_increasing_order_parameter_limit();
        iopl->set_arrangement(lm::io::TrajectoryLimits::ASCENDING);
        iopl->set_limit_id(input.tilings.getCurrentTiling()->getEdgesCount() - 1);
        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        iopl->add_value(input.tilings.getCurrentTiling()->getFinalEdge());
        break;
    }
    case 1: // ffluxphase==0 and tilings.getCurrentTiling().getArrangement()==lm::io::Tilings::DESCENDING
    {
        // decreasing edge 0 limit
        lm::io::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_limits()->add_decreasing_order_parameter_limit();
        dopl->set_arrangement(lm::io::TrajectoryLimits::DESCENDING);
        dopl->set_limit_id(0);
        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));

        // increasing edge 0 limit
        lm::io::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_limits()->add_increasing_order_parameter_limit();
        iopl->set_arrangement(lm::io::TrajectoryLimits::DESCENDING);
        iopl->set_limit_id(0);
        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));

        // decreasing final edge limit
        dopl = getRunMsg()->mutable_limits()->add_decreasing_order_parameter_limit();
        dopl->set_arrangement(lm::io::TrajectoryLimits::DESCENDING);
        dopl->set_limit_id(input.tilings.getCurrentTiling()->getEdgesCount() - 1);
        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        dopl->add_value(input.tilings.getCurrentTiling()->getFinalEdge());
        break;
    }
    case 2: // ffluxphase!=0 and tilings.getCurrentTiling().getArrangement()==lm::io::Tilings::ASCENDING
    {
        // decreasing edge 0 limit
        lm::io::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_limits()->add_decreasing_order_parameter_limit();
        dopl->set_arrangement(lm::io::TrajectoryLimits::ASCENDING);
        dopl->set_limit_id(0);
        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));

        // increasing current phase edge limit
        lm::io::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_limits()->add_increasing_order_parameter_limit();
        iopl->set_arrangement(lm::io::TrajectoryLimits::ASCENDING);
        iopl->set_limit_id(ffluxPhase);
        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(ffluxPhase));
        break;
    }
    case 3: // ffluxphase!=0 and tilings.getCurrentTiling().getArrangement()==lm::io::Tilings::DESCENDING
    {
        // increasing edge 0 limit
        lm::io::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_limits()->add_increasing_order_parameter_limit();
        iopl->set_arrangement(lm::io::TrajectoryLimits::DESCENDING);
        iopl->set_limit_id(0);
        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));

        // decreasing current phase edge limit
        lm::io::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_limits()->add_decreasing_order_parameter_limit();
        dopl->set_arrangement(lm::io::TrajectoryLimits::DESCENDING);
        dopl->set_limit_id(ffluxPhase);
        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(ffluxPhase));
        break;
    }
    }
}

bool FFluxTrajectory::fluxedBackward()
{
    if (input.tilings.getCurrentTiling()->getArrangement()==lm::io::Tilings::ASCENDING)
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
    if (input.tilings.getCurrentTiling()->getArrangement()==lm::io::Tilings::ASCENDING)
    {
        return (getFinalLimitType()==lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER);
    }
    else
    {
        return (getFinalLimitType()==lm::io::TrajectoryLimits::DECREASINGORDERPARAMETER);
    }
}

// accessor definitions
uint FFluxTrajectory::getFFluxPhase()
{
    return ffluxPhase;
}

lm::io::TrajectoryLimits::LimitType FFluxTrajectory::getFinalLimitType()
{
    return getState()->final_limit_type();
}

double FFluxTrajectory::getLastLimitTime()
{
    return lastLimitTime;
}

void FFluxTrajectory::getLastSpeciesCounts(lm::io::FFluxOutput::TrajectoryOutput* trajectoryOutputBuf)
{
    uint offset = (getSpeciesCounts()->number_entries() - 1)*(getSpeciesCounts()->number_species());
    for (int i=0; i<getSpeciesCounts()->number_species(); i++)
    {
        trajectoryOutputBuf->add_species_count(getSpeciesCounts()->species_count(i + offset));
    }
}

bool FFluxTrajectory::hasElapsed(double time)
{
    return (getSimTime()>=time);
}

// mutator definitions
void FFluxTrajectory::setLastLimitTime(double lLT)
{
    lastLimitTime = lLT;
}

}
}
