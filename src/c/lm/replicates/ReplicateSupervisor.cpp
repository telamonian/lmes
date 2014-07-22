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
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/replicates/ReplicateSupervisor.h"
#include "lm/replicates/ReplicateTrajectoryList.h"
#include "lm/resource/ResourceMap.h"

using std::map;
using std::string;
using lm::resource::ResourceMap;

namespace lm {
namespace replicates {

bool ReplicateSupervisor::registered=ReplicateSupervisor::registerClass();

bool ReplicateSupervisor::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::main::SimulationSupervisor","lm::replicates::ReplicateSupervisor",&ReplicateSupervisor::allocateObject);
    return true;
}

void* ReplicateSupervisor::allocateObject()
{
    return new ReplicateSupervisor();
}

ReplicateSupervisor::ReplicateSupervisor()
:outputWriterProcess(0),outputWriterThread(3) //TODO: fix to -1,-1 once the slot code has been fixed
{
}

ReplicateSupervisor::~ReplicateSupervisor()
{
    if (trajectories != NULL) delete trajectories; trajectories = NULL;
}

void ReplicateSupervisor::allResourcesRegistered()
{
    // Reserve a core for the output writer.
    ResourceMap::ComputeResources resources = resourceMap->reserveCPUCores(communicator.getSourceProcess(),1);
    Print::printf(Print::INFO, "Reserved core %d on %d:%d for the output writer.", resources.cpuCores[0], resources.controller_process, resources.controller_thread);

    // Start the output writer.
    lm::message::Message msg;
    msg.mutable_start_output_writer()->set_use_cpu_affinity(useCPUAffinity);
    msg.mutable_start_output_writer()->set_cpu(resources.cpuCores[0]);
    msg.mutable_start_output_writer()->set_output_filename(simulationOutputFilename);
    msg.mutable_start_output_writer()->set_output_writer_class(outputWriterClassName);
    communicator.sendMessage(resources.controller_process, resources.controller_thread, &msg);
    // Call the base allResourcesRegistered method.
    SimulationSupervisor::allResourcesRegistered();
}

void ReplicateSupervisor::outputWriterStarted(const lm::message::StartedOutputWriter& msg)
{
    outputWriterProcess = msg.process();
    outputWriterThread = msg.thread();
}

void ReplicateSupervisor::startSimulation()
{
    // Check for some error conditions.
    if (outputWriterProcess == -1 || outputWriterThread == -1)
        throw new Exception("ReplicateSupervisor could not start the simulation, no output writer available.");

    Print::printf(Print::INFO, "Replicate supervisor starting simulation.");

    // Create the new trajectory list.
    trajectories = new ReplicateTrajectoryList(::replicates.front(), ::replicates.back(), simulationParameterMap, reactionModel);

    // Get the trajectories template msg so that we can set some default values in it
    lm::message::RunWorkUnit* runWorkUnitMsg = trajectories->getRunWorkUnitMsg();
	// Set the default source process/thread
	runWorkUnitMsg->set_supervisor_process(communicator.getSourceProcess());
	runWorkUnitMsg->set_supervisor_thread(communicator.getSourceThread());
    // Set the default writer process/thread
	runWorkUnitMsg->set_output_process(outputWriterProcess);
	runWorkUnitMsg->set_output_thread(outputWriterThread);
	// Set the default work unit-specific limits
	runWorkUnitMsg->set_max_steps(100);
	// Set the default trajectory limits
//	initLimits();

	//// TEMP : replace; hardcoded increasing/decreasing limits for timing comparisons with forward flux
	limits.add_decreasing_species_count(0);
	limits.add_increasing_species_count(0);
	double a_incr = 0;
	setTestCaseLimits(NULL, &a_incr);
	//// TEMP

	lm::io::TrajectoryLimits* trajectoryLimits = new lm::io::TrajectoryLimits(limits);
	runWorkUnitMsg->set_allocated_limits(trajectoryLimits);

	// Now that the template msg has been set properly, initialize the trajectory list
	trajectories->init();

    // Call the base class method
    SimulationSupervisor::startSimulation();
}

void ReplicateSupervisor::workUnitStarted(const lm::message::StartedWorkUnit& msg)
{
    Print::printf(Print::INFO, "Work unit %d started.",msg.work_unit_id());
}

//// TEMP : remove
void ReplicateSupervisor::setTestCaseLimits(double* a_decr, double* a_incr)
{
	limits.set_decreasing_species_count(0, -9999);
	limits.set_increasing_species_count(0, -9999);
	if (a_decr!=NULL)
	{
		limits.set_decreasing_species_count(0, *a_decr);
	}
	if (a_incr!=NULL)
	{
		limits.set_increasing_species_count(0, *a_incr);
	}
}
//// TEMP

//void ReplicateSupervisor::workUnitFinished(const lm::message::FinishedWorkUnit& msg)
//{
//    Print::printf(Print::INFO, "Work unit %d finished in %0.3f s.",msg.work_unit_id(),msg.run_time());
//
//    if (msg.status() == lm::message::FinishedWorkUnit::LIMIT_REACHED)
//    {
//        trajectories->updateTrajectoryStatus(msg.final_state().trajectory_id(), TrajectoryList::FINISHED);
//        trajectories->updateTrajectoryState(msg.final_state().trajectory_id(), msg.final_state());
//    }
//    else
//    {
//        trajectories->updateTrajectoryStatus(msg.final_state().trajectory_id(), TrajectoryList::WAITING);
//        trajectories->updateTrajectoryState(msg.final_state().trajectory_id(), msg.final_state());
//    }
//    // Free the slot that the returning work unit just ran on
//    slots.free(msg.process(), msg.thread());
//
//    // Get next available slot. If there are more trajectories than slots, this is guaranteed to be the slot we just freed. Otherwise it will be the "coldest" (longest unoccupied) slot
//    lm::resource::Slot * workSlot = slots.alloc();
//    if (workSlot==NULL) Print::printf(Print::ERROR, "Slot allocation error (there was no free slot even though a slot should have been freed immediately prior)");
//
//    // Get the next trajectory to run, if there is one.
//    int nextTrajectory = trajectories->nextTrajectoryToRun();
//    if (nextTrajectory >= 0)
//    {
//        // Check for some error conditions.
//        if (outputWriterProcess == -1 || outputWriterThread == -1)
//            throw new Exception("ReplicateSupervisor could not start the simulation, no output writer available.");
//
//        // Send the start work unit message.
//        lm::message::Message msg;
//        lm::message::RunWorkUnit& run = *msg.mutable_run_work_unit();
//        run.set_work_unit_id(workUnitCount++);
//        run.set_supervisor_process(communicator.getSourceProcess());
//        run.set_supervisor_thread(communicator.getSourceThread());
//        run.set_output_process(outputWriterProcess);
//        run.set_output_thread(outputWriterThread);
//        run.set_max_steps(100);
//        *run.mutable_initial_state() = trajectories->getTrajectoryState(nextTrajectory);
//        *run.mutable_limits() = limits;
//        Print::printf(Print::INFO, "Sending message to start work unit %d with trajectory %d on slot %d:%d.", run.work_unit_id(), nextTrajectory, workSlot->getSlotKey()[0], workSlot->getSlotKey()[1]);
//        communicator.sendMessage(workSlot->getSlotKey()[0], workSlot->getSlotKey()[1], &msg);
//        trajectories->updateTrajectoryStatus(nextTrajectory, TrajectoryList::RUNNING);
//    }
//    else
//    {
//        finishSimulation();
//    }
//
//}

}
}
