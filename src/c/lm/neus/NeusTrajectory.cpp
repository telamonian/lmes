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

#include "lm/neus/NeusTrajectory.h"
#include "lm/input/DiffusionModel.pb.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/input/Tilings.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

using lm::input::DiffusionModel;
using lm::input::ReactionModel;
using lm::io::TrajectoryState;
using std::map;
using std::string;

namespace lm {
namespace neus {

//FFluxTrajectory::FFluxTrajectory(uint64_t id,uint ffluxPhase,const ReactionModel& reactionModel,const DiffusionModel& diffusionModel,map<string,string>& simulationParameters,lm::tiling::Tilings& tilings,bool reversed):
//Trajectory(id,reactionModel,diffusionModel,simulationParameters,reversed),ffluxPhase(ffluxPhase),tilings(tilings)
//{
//    initLimits();
//}
//
//FFluxTrajectory::FFluxTrajectory(uint64_t id,uint ffluxPhase,const ReactionModel& reactionModel,const DiffusionModel& diffusionModel,map<string,string>& simulationParameters,lm::tiling::Tilings& tilings,TrajectoryState* zerothState):
//Trajectory(id,reactionModel,diffusionModel,simulationParameters,zerothState),ffluxPhase(ffluxPhase),tilings(tilings)
//{
//    // Limit setting code
//    initLimits();
//}

NeusTrajectory::NeusTrajectory(uint64_t id,uint ffluxPhase,lm::input::Input& input,bool reversed):
Trajectory(id,input,reversed),ffluxPhase(ffluxPhase)
{
    initLimits();
}

NeusTrajectory::NeusTrajectory(uint64_t id,uint ffluxPhase,lm::input::Input& input,TrajectoryState* zerothState):
Trajectory(id,input,zerothState),ffluxPhase(ffluxPhase)
{
    // Limit setting code
    initLimits();
}

NeusTrajectory::~NeusTrajectory()
{
}

bool NeusTrajectory::fluxedBackward()
{
    if (input.tilings[0]->getArrangement()==lm::input::Tilings::ASCENDING)
    {
        return (getFinalLimitType()==lm::input::TrajectoryLimits::DECREASINGORDERPARAMETER);
    }
    else
    {
        return (getFinalLimitType()==lm::input::TrajectoryLimits::INCREASINGORDERPARAMETER);
    }
}

bool NeusTrajectory::fluxedForward()
{
    if (input.tilings[0]->getArrangement()==lm::input::Tilings::ASCENDING)
    {
        return (getFinalLimitType()==lm::input::TrajectoryLimits::INCREASINGORDERPARAMETER);
    }
    else
    {
        return (getFinalLimitType()==lm::input::TrajectoryLimits::DECREASINGORDERPARAMETER);
    }
}

lm::input::TrajectoryLimits::LimitType NeusTrajectory::getFinalLimitType()
{
    return getState()->final_limit_type();
}

uint NeusTrajectory::getSimSteps()
{
    return getState()->cme_state().species_counts().number_entries();
}

double NeusTrajectory::getSimTime()
{
    return getState()->cme_state().species_counts().time(getState()->cme_state().species_counts().time_size() - 1);
}

bool NeusTrajectory::hasElapsed(double time)
{
    return (getSimTime()>=time);
}

void NeusTrajectory::initLimits()
{
    getRunMsg()->mutable_limits()->Clear();
    switch ((ffluxPhase!=0)<<1|input.tilings[0]->getArrangement()!=lm::input::Tilings::ASCENDING) // each of the 4 sets of possible pairs of true/false values corresponds to one of the numbers 0-3
    {
    case 0: // ffluxphase==0 and tilings[0].getArrangement()==lm::input::Tilings::ASCENDING
    {
        lm::input::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_limits()->add_increasing_order_parameter_limit();
        iopl->set_arrangement(lm::input::TrajectoryLimits::ASCENDING);
        iopl->set_order_parameter_id(input.tilings[0]->getOrderParameterID());
        iopl->add_value(input.tilings[0]->getEdge(0));
        break;
    }
    case 1: // ffluxphase==0 and tilings[0].getArrangement()==lm::input::Tilings::DESCENDING
    {
        lm::input::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_limits()->add_decreasing_order_parameter_limit();
        dopl->set_arrangement(lm::input::TrajectoryLimits::DESCENDING);
        dopl->set_order_parameter_id(input.tilings[0]->getOrderParameterID());
        dopl->add_value(input.tilings[0]->getEdge(0));
        break;
    }
    case 2: // ffluxphase!=0 and tilings[0].getArrangement()==lm::input::Tilings::ASCENDING
    {
        lm::input::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_limits()->add_decreasing_order_parameter_limit();
        dopl->set_order_parameter_id(input.tilings[0]->getOrderParameterID());
        dopl->add_value(input.tilings[0]->getEdge(0));
        dopl->set_arrangement(lm::input::TrajectoryLimits::ASCENDING);
        lm::input::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_limits()->add_increasing_order_parameter_limit();
        iopl->set_arrangement(lm::input::TrajectoryLimits::ASCENDING);
        iopl->set_order_parameter_id(input.tilings[0]->getOrderParameterID());
        iopl->add_value(input.tilings[0]->getEdge(ffluxPhase));
        break;
    }
    case 3: // ffluxphase!=0 and tilings[0].getArrangement()==lm::input::Tilings::DESCENDING
    {
        lm::input::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_limits()->add_increasing_order_parameter_limit();
        iopl->set_arrangement(lm::input::TrajectoryLimits::DESCENDING);
        iopl->set_order_parameter_id(input.tilings[0]->getOrderParameterID());
        iopl->add_value(input.tilings[0]->getEdge(0));
        lm::input::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_limits()->add_decreasing_order_parameter_limit();
        dopl->set_arrangement(lm::input::TrajectoryLimits::DESCENDING);
        dopl->set_order_parameter_id(input.tilings[0]->getOrderParameterID());
        dopl->add_value(input.tilings[0]->getEdge(ffluxPhase));
        break;
    }
    }
}

}
}
