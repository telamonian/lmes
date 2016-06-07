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
#include <limits>
#include <map>
#include <string>

#include "lm/ClassFactory.h"
#include "lm/fflux/FFluxSupervisor.h"
#include "lm/fflux/FFluxTrajectoryList.h"
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
#include "lm/Print.h"
#include "lm/resource/ResourceMap.h"
#include "lm/tiling/Tiling.h"

using std::map;
using std::string;
using lm::resource::ResourceMap;

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

FFluxSupervisor::FFluxSupervisor(): ffluxPhase(0), trajectoryList(NULL)
{
}

FFluxSupervisor::~FFluxSupervisor()
{
}

//void FFluxSupervisor::buildRunWorkUnitLimits(lm::message::RunWorkUnit* msg)
//{
//    // Set the limits in the RunWorkUnit header.
//    msg->mutable_trajectory_limits()->CopyFrom(trajectoryLimits.buf());
//}

//void FFluxSupervisor::setLimits()
//{
//    trajectoryLimits.clear();
//    const lm::tiling::Tiling& tiling = input->getTilings().getCurrentTiling();
//
//    if (ffluxPhase==0)
//    {
//        tiling.addLimitMsg(trajectoryLimits, 0, EH::INCREASING);
//        tiling.addLimitMsg(trajectoryLimits, 0, EH::DECREASING);
//
//        tiling.addLimitMsg(trajectoryLimits, tiling.getLastEdgeIndex(), EH::INCREASING);
//    }
//    else
//    {
//        tiling.addLimitMsg(trajectoryLimits, 0, EH::DECREASING);
//
//        tiling.addLimitMsg(trajectoryLimits, ffluxPhase, EH::INCREASING);
//    }
//
////    switch ((ffluxPhase!=0)<<1|input.tilings.getCurrentTiling()->getSortOrder()!=lm::input::Tilings::ASCENDING)
////    {
////    case 0: // ffluxphase==0 and tilings.getCurrentTiling().getSortOrder()==lm::input::Tilings::ASCENDING
////    {
////        // increasing edge 0 limit
////        lm::input::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_increasing_order_parameter_limit();
////        iopl->set_arrangement(lm::input::TrajectoryLimits::ASCENDING);
////        iopl->set_limit_id(0);
////        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
////
////        // decreasing edge 0 limit
////        lm::input::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_decreasing_order_parameter_limit();
////        dopl->set_arrangement(lm::input::TrajectoryLimits::ASCENDING);
////        dopl->set_limit_id(0);
////        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
////
////        // increasing final edge limit
////        iopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_increasing_order_parameter_limit();
////        iopl->set_arrangement(lm::input::TrajectoryLimits::ASCENDING);
////        iopl->set_limit_id(input.tilings.getCurrentTiling()->getEdgesCount() - 1);
////        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        iopl->add_value(input.tilings.getCurrentTiling()->getFinalEdge());
////        break;
////    }
////    case 1: // ffluxphase==0 and tilings.getCurrentTiling().getSortOrder()==lm::input::Tilings::DESCENDING
////    {
////        // decreasing edge 0 limit
////        lm::input::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_decreasing_order_parameter_limit();
////        dopl->set_arrangement(lm::input::TrajectoryLimits::DESCENDING);
////        dopl->set_limit_id(0);
////        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
////
////        // increasing edge 0 limit
////        lm::input::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_increasing_order_parameter_limit();
////        iopl->set_arrangement(lm::input::TrajectoryLimits::DESCENDING);
////        iopl->set_limit_id(0);
////        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
////
////        // decreasing final edge limit
////        dopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_decreasing_order_parameter_limit();
////        dopl->set_arrangement(lm::input::TrajectoryLimits::DESCENDING);
////        dopl->set_limit_id(input.tilings.getCurrentTiling()->getEdgesCount() - 1);
////        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        dopl->add_value(input.tilings.getCurrentTiling()->getFinalEdge());
////        break;
////    }
////    case 2: // ffluxphase!=0 and tilings.getCurrentTiling().getSortOrder()==lm::input::Tilings::ASCENDING
////    {
////        // decreasing edge 0 limit
////        lm::input::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_decreasing_order_parameter_limit();
////        dopl->set_arrangement(lm::input::TrajectoryLimits::ASCENDING);
////        dopl->set_limit_id(0);
////        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
////
////        // increasing current phase edge limit
////        lm::input::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_increasing_order_parameter_limit();
////        iopl->set_arrangement(lm::input::TrajectoryLimits::ASCENDING);
////        iopl->set_limit_id(ffluxPhase);
////        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(ffluxPhase));
////        break;
////    }
////    case 3: // ffluxphase!=0 and tilings.getCurrentTiling().getSortOrder()==lm::input::Tilings::DESCENDING
////    {
////        // increasing edge 0 limit
////        lm::input::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_increasing_order_parameter_limit();
////        iopl->set_arrangement(lm::input::TrajectoryLimits::DESCENDING);
////        iopl->set_limit_id(0);
////        iopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        iopl->add_value(input.tilings.getCurrentTiling()->getEdge(0));
////
////        // decreasing current phase edge limit
////        lm::input::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getRunMsg()->mutable_work_unit(0)->mutable_limits()->add_decreasing_order_parameter_limit();
////        dopl->set_arrangement(lm::input::TrajectoryLimits::DESCENDING);
////        dopl->set_limit_id(ffluxPhase);
////        dopl->set_order_parameter_id(input.tilings.getCurrentTiling()->getOrderParameterID());
////        dopl->add_value(input.tilings.getCurrentTiling()->getEdge(ffluxPhase));
////        break;
////    }
////    }
//}

//void FFluxSupervisor::finishSimulation()
//{
//	// Create the output message.
//	lm::message::Message msgp;
//	lm::message::ProcessWorkUnitOutput* msg = msgp.add_process_work_unit_output();
//	msg->set_work_unit_id(999999999999999);
//
//	// Initialize the fflux output data
//	lm::io::FFluxOutput* ffluxOutput = NULL;
//	ffluxOutput = msg->mutable_fflux_output();
//
//	// Assign the fflux output data
//	*ffluxOutput = *(static_cast<lm::fflux::FFluxTrajectoryList*>(trajectoryList)->getFFluxOutput());
//
//	// Send the message
//	communicator.sendMessageToMasterOutput(&msgp);
//
//	SimulationSupervisor::finishSimulation();
//}

void FFluxSupervisor::buildSimulationPhaseList()
{
    simulationPhaseList.push_back(new lm::input::SimulationPhase);
    lm::input::SimulationPhase* phase = simulationPhaseList.back();

    phase->set_id(0);
    lm::trajectory::Trajectory initialTrajectory(0, phase->id(), *input);
    for (uint64_t i=::replicates.front(); i<=::replicates.back(); i++)
    {
        phase->add_trajectory_states()->CopyFrom(initialTrajectory.getState());
    }
    phase->mutable_trajectory_limits()->CopyFrom(input->getTrajectoryLimitsMsg());
    phase->mutable_output_options()->CopyFrom(input->getOutputOptionsMsg());
}

void FFluxSupervisor::buildTrajectoryList()
{
    setTrajectoryList(new FFluxTrajectoryList(simulationPhase, *input, communicator, slots.getSimultaneousWorkUnits()));
}

void FFluxSupervisor::finishSimulation()
{
    // Create the output message.
    lm::message::Message msg;
    lm::message::ProcessWorkUnitOutput* pwoMsg = msg.mutable_process_work_unit_output();
    pwoMsg->set_work_unit_id(std::numeric_limits<int64_t>::max());
    lm::message::WorkUnitOutput* wuoMsg = pwoMsg->add_part_output();

    // Initialize/assign the fflux output data
    lm::io::FFluxOutput* ffluxOutputBuf = wuoMsg->mutable_fflux_output();
    ffluxOutputBuf->CopyFrom(*(static_cast<lm::fflux::FFluxTrajectoryList*>(trajectoryList)->getFFluxOutput()));

    // Send the message
    communicator.sendMessageToMasterOutput(&msg);

    SimulationSupervisor::finishSimulation();
}

void FFluxSupervisor::incrementFFluxPhase()
{
    ffluxPhase++;


    static_cast<lm::fflux::FFluxTrajectoryList*>(trajectoryList)->incrementFFluxPhase();
}

void FFluxSupervisor::receivedProcessWorkUnitOutput(lm::message::Message& msg)
{
    // Loop over every output in the message.
    lm::message::ProcessWorkUnitOutput pwuMsg = msg.process_work_unit_output();
    for (int i=0; i<pwuMsg.part_output_size(); i++)
    {
        lm::message::WorkUnitOutput wuoMsg = pwuMsg.part_output(i);
        if (wuoMsg.has_species_counts())
        {
            (static_cast<FFluxTrajectoryList*>(trajectoryList))->ffluxOutputAddTrajectory(wuoMsg.species_counts(), lm::io::FFluxOutput::RUNNING);
        }
        else if (wuoMsg.has_species_time_series())
        {
            (static_cast<FFluxTrajectoryList*>(trajectoryList))->ffluxOutputAddTrajectory(wuoMsg.species_time_series(), lm::io::FFluxOutput::RUNNING);
        }
    }
}

void FFluxSupervisor::receivedStartedOutputWriter(const lm::message::StartedOutputWriter& msg)
{
    Print::printf(Print::INFO, "Output writer started: %d:%d.",msg.process(),msg.thread());
    hasOutputWriterStarted = true;

    // set output process/thread to that of this supervisor, while keeping track of the real values
    outputWriterProcess = communicator.getSourceProcess();
    outputWriterThread = communicator.getSourceThread();
//    outputWriterProcess = msg.process();
//    outputWriterThread = msg.thread();
    communicator.setMasterOutputEndpoint(msg.process(), msg.thread());
    startSimulationIfAllWorkersStarted();
}

void FFluxSupervisor::resetFFluxPhase()
{
    ffluxPhase = 0;
}

void FFluxSupervisor::startSimulation()
{
    // Check for some error conditions.
    if (outputWriterProcess == -1 || outputWriterThread == -1)
        throw new Exception("Forward flux supervisor could not start the simulation, no output writer available.");

    Print::printf(Print::INFO, "Forward flux supervisor starting simulation.");

    // Call the base class method.
    SimulationSupervisor::startSimulation();
}

void FFluxSupervisor::setTrajectoryList(lm::trajectory::TrajectoryList* newTrajectoryList)
{
    lm::main::SimulationSupervisor::setTrajectoryList(newTrajectoryList);
    trajectoryList = static_cast<lm::fflux::FFluxTrajectoryList*>(lm::main::SimulationSupervisor::trajectoryList);
}

}
}
