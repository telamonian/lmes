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
#include <map>
#include <string>

#include "lm/ClassFactory.h"
#include "lm/Print.h"
#include "lm/io/OutputWriter.h"
#include "lm/main/Main.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Message.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/StartedOutputWriter.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/fflux/FFluxSupervisor.h"
#include "lm/fflux/FFluxTrajectoryList.h"
#include "lm/resource/ResourceMap.h"

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

FFluxSupervisor::FFluxSupervisor()
{
}

FFluxSupervisor::~FFluxSupervisor()
{
}

void FFluxSupervisor::finishSimulation()
{
	// Create the output message.
	lm::message::Message msgp;
	lm::message::ProcessWorkUnitOutput* msg = msgp.add_process_work_unit_output();
	msg->set_work_unit_id(999999999999999);

	// Initialize the fflux output data
	lm::io::FFluxOutput* ffluxOutput = NULL;
	ffluxOutput = msg->mutable_fflux_output();

	// Assign the fflux output data
	*ffluxOutput = *(static_cast<lm::fflux::FFluxTrajectoryList*>(trajectoryList)->getFFluxOutput());

	// Send the message
	communicator.sendMessageToMasterOutput(&msgp);

	SimulationSupervisor::finishSimulation();
}

void FFluxSupervisor::receivedProcessWorkUnitOutput(lm::message::Message& msg)
{
    // Loop over every output in the message.
    for (int i=0; i<msg.process_work_unit_output_size(); i++)
    {
        if (msg.process_work_unit_output(i).has_species_counts())
        {
            (static_cast<FFluxTrajectoryList*>(trajectoryList))->ffluxOutputAddTrajectory(msg.process_work_unit_output(i).species_counts(), lm::io::FFluxOutput::RUNNING);
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
    communicator.setMasterOutputEndpoint(msg.process(), msg.thread());
    startSimulationIfAllWorkersStarted();
}

void FFluxSupervisor::startSimulation()
{
    // Check for some error conditions.
    if (outputWriterProcess == -1 || outputWriterThread == -1)
        throw new Exception("Forward flux supervisor could not start the simulation, no output writer available.");

    Print::printf(Print::INFO, "Forward flux supervisor starting simulation.");

    // Create the new trajectory list.
    trajectoryList = new FFluxTrajectoryList(communicator, slots.getNumberSlots(),*input);

    // Call the base class method.
    SimulationSupervisor::startSimulation();
}


}
}
