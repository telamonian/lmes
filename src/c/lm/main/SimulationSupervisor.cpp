/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
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

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "hrtime.h"
#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/hdf5/HDF5.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/main/Globals.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Communicator.h"
#include "lm/message/Endpoint.pb.h"
#include "lm/message/FinishedCheckpointing.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ResourcesAvailable.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/message/StartedWorkUnitRunner.pb.h"
#include "lm/message/WorkUnit.pb.h"
#include "lm/resource/ComputeResources.h"
#include "lm/resource/ResourceMap.h"
#include "lm/slot/Slot.h"
#include "lm/slot/SlotList.h"

using lm::message::Communicator;
using lm::message::Endpoint;
using lm::resource::ComputeResources;
using lm::resource::ResourceMap;
using std::string;
using std::vector;

namespace lm {
namespace main {

SimulationSupervisor::SimulationSupervisor()
:communicator(NULL),hasCheckpointSignalerStarted(false),hasOutputWriterStarted(false),haveAllWorkUnitRunnersStarted(false),
 input(NULL),outputWriterClassName(""),performingCheckpoint(false),
 simulationInputFilename(""),simulationOutputFilename(""),simulationPhase(0),simulationRunning(true),slots(),
 solverClassName(""),trajectoryList(NULL),useCPUAffinity(false),workUnitCount(0)
{
    // Create the communicator.
    communicator = lm::message::Communicator::createObjectOfDefaultSubclass(true);

    resetPerformanceStatistics();
}

SimulationSupervisor::~SimulationSupervisor()
{
    if (input != NULL) delete input; input = NULL;
    if (trajectoryList != NULL) delete trajectoryList; trajectoryList = NULL; // since Supervisors call new to allocate their TrajectoryLists, this needs to be here
    if (communicator != NULL) delete communicator; communicator = NULL;
}

void SimulationSupervisor::init()
{
    // Initialize the input object with the input filenames.
    input = new lm::input::Input(simulationInputFilenames);
}

void SimulationSupervisor::wake() throw(lm::thread::PthreadException)
{
    lm::message::Message msg;
    msg.mutable_ping_target()->set_id(0);
    communicator->sendMessage(communicator->getSourceAddress(), &msg);
}

int SimulationSupervisor::run()
{
    try
    {
        Print::printf(Print::INFO, "Supervisor %s started.", Communicator::printableAddress(communicator->getSourceAddress()).c_str());
        // Loop reading messages.
        lm::message::Message message;
        while (running && simulationRunning)
        {
            // Read the next message.
            communicator->receiveMessage(&message);

            // Do something with the message.
            if (message.has_resources_available())
            {
                receivedResourceAvailable(message.resources_available());
            }
            else if (message.has_started_output_writer())
            {
                receivedStartedOutputWriter(message.started_output_writer());
            }
            else if (message.has_started_checkpoint_signaler())
            {
                receivedStartedCheckpointSignaler(message.started_checkpoint_signaler());
            }
            else if (message.has_started_work_unit_runner())
            {
                receivedStartedWorkUnitRunner(message.started_work_unit_runner());
            }
            else if (message.has_started_work_unit())
            {
                receivedStartedWorkUnit(message.started_work_unit());
            }
            else if (message.has_finished_work_unit())
            {
                receivedFinishedWorkUnit(message.finished_work_unit());
            }
            else if (message.has_perform_checkpointing())
            {
                receivedPerformCheckpointing(message.perform_checkpointing());
            }
            else if (message.has_finished_checkpointing())
            {
                receivedFinishedCheckpointing(message.finished_checkpointing());
            }
            else if (message.has_process_work_unit_output() > 0)
            {
                receivedProcessWorkUnitOutput(message);
            }
                // hook for adding generic behavior to this loop in child Supervisors
            else if (receivedOther(message))
            {
            }
            else if (message.has_ping_target())
            {
            }
            else
            {
                Print::printf(Print::ERROR, "Supervisor received an unknown message: {\n%s}",message.DebugString().c_str());
            }

            // Print any performance statistics.
            printPerformanceStatistics();

            // Clear the message object so it can be used again.
            message.Clear();
        }

        // Flush any performance statistics.
        printPerformanceStatistics(true);

        Print::printf(Print::INFO, "Supervisor %s finished.", Communicator::printableAddress(communicator->getSourceAddress()).c_str());

        return 0;
    }
    catch (lm::Exception e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (lm::Exception* e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e->what(), __FILE__, __LINE__);
    }
    catch (std::exception& e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (...)
    {
        Print::printf(Print::FATAL, "Unknown Exception during execution (%s:%d)", __FILE__, __LINE__);
    }
    return -1;
}

void SimulationSupervisor::receivedResourceAvailable(const lm::message::ResourcesAvailable& msg)
{
    Print::printf(Print::INFO, "Resource controller %s on %s registered with %d cpu core(s) and %d gpu device(s).", Communicator::printableAddress(msg.controller_address()).c_str(), msg.hostname().c_str(), msg.cpu_cores_size(), msg.gpu_devices_size());
    if (resourceMap.registerResources(msg))
    {
        allResourcesRegistered();
    }
}

void SimulationSupervisor::allResourcesRegistered()
{
    // Start the work unit runners.
    Print::printf(Print::INFO, "All resources registered with supervisor, starting workers.");

    startOutputWriter();
    startCheckpointSignaler();
    startWorkUnitRunners();
}

void SimulationSupervisor::startOutputWriter()
{
    // Reserve a core for the output writer if the option is set.
    if (shouldReserveOutputCore)
    {
        // Start the output writer.
        ComputeResources resources = resourceMap.reserveCPUCores(communicator->getHostname(), 1);
        lm::message::Message msg;
        msg.mutable_start_output_writer()->set_use_cpu_affinity(useCPUAffinity);
        msg.mutable_start_output_writer()->set_cpu(resources.cpuCores[0]);
        msg.mutable_start_output_writer()->set_output_filename(simulationOutputFilename);
        msg.mutable_start_output_writer()->set_output_writer_class(outputWriterClassName);
        communicator->sendMessage(resources.controllerAddress, &msg);
        Print::printf(Print::INFO, "Reserved core %d on %s for the output writer.", resources.cpuCores[0], Communicator::printableAddress(resources.controllerAddress).c_str());
    }
    else
    {
        // Start the output writer.
        ComputeResources resources = resourceMap.reserveCPUCores(communicator->getHostname(), 0);
        lm::message::Message msg;
        msg.mutable_start_output_writer()->set_output_filename(simulationOutputFilename);
        msg.mutable_start_output_writer()->set_output_writer_class(outputWriterClassName);
        // thread 1 should be the resource controller
        communicator->sendMessage(resources.controllerAddress, &msg);
        Print::printf(Print::INFO, "Output writer is sharing resources on %s.", Communicator::printableAddress(resources.controllerAddress).c_str());
    }
}

void SimulationSupervisor::startCheckpointSignaler()
{
    //See if we need to start a checkpoint signaler.
    if (checkpointInterval > 0)
    {
        // Start the checkpoint signaler.
        ComputeResources resources = resourceMap.reserveCPUCores(communicator->getHostname(), 0);
        lm::message::Message msg;
        msg.mutable_start_checkpoint_signaler()->set_checkpoint_interval(checkpointInterval);
        communicator->sendMessage(resources.controllerAddress, &msg);
    }
    else
    {
        hasCheckpointSignalerStarted = true;
    }
}

void SimulationSupervisor::startWorkUnitRunners()
{
    map<string,ComputeResources> allResources = resourceMap.getAvailableResources();
    slots.createAllSlots(allResources, cpuCoresPerRunner, gpuDevicesPerRunner, useCPUAffinity, solverClassName, *input);
}

void SimulationSupervisor::receivedStartedOutputWriter(const lm::message::StartedOutputWriter& msg)
{
    Print::printf(Print::INFO, "Output writer started: %s.", Communicator::printableAddress(msg.address()).c_str());
    hasOutputWriterStarted = true;
    outputWriterAddress = msg.address();
    startSimulationIfAllWorkersStarted();
}

void SimulationSupervisor::receivedStartedCheckpointSignaler(const lm::message::StartedCheckpointSignaler& msg)
{
    Print::printf(Print::INFO, "Checkpoint signaller started: %s.", Communicator::printableAddress(msg.address()).c_str());
    hasCheckpointSignalerStarted = true;
    startSimulationIfAllWorkersStarted();
}

void SimulationSupervisor::receivedStartedWorkUnitRunner(const lm::message::StartedWorkUnitRunner& msg)
{
    Print::printf(Print::INFO, "Work unit runner started: %s.", Communicator::printableAddress(msg.address()).c_str());

    slots.markSlotStarted(msg);
    if (!slots.hasUnstartedSlots())
    {
        haveAllWorkUnitRunnersStarted = true;
        startSimulationIfAllWorkersStarted();
    }
}

void SimulationSupervisor::startSimulationIfAllWorkersStarted()
{
    if (haveAllWorkUnitRunnersStarted && hasOutputWriterStarted && hasCheckpointSignalerStarted)
    {
        Print::printf(Print::INFO, "All supervisor workers have started, beginning simulation.");
        startSimulation();
    }
}

void SimulationSupervisor::startSimulation()
{
    Print::printf(Print::INFO, "Simulation started.");
    startSimulationPhase();
}

void SimulationSupervisor::startSimulationPhase()
{
    // Build the list of trajectories to simulate.
    buildTrajectoryList();

    // Assign the first batch of work.
    if (assignWork())
    {
        // If assign work returned true, there was nothing to be done.
        Print::printf(Print::INFO, "No work to be performed.");
        finishSimulationPhase();
    }
}

void SimulationSupervisor::receivedStartedWorkUnit(const lm::message::StartedWorkUnit& msg)
{
    Print::printf(Print::VERBOSE_DEBUG, "Work unit %d started.",msg.work_unit_id());
}

void SimulationSupervisor::receivedFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg)
{
    Print::printf(Print::VERBOSE_DEBUG, "Work unit %d finished in %0.3f s.",msg.work_unit_id(),msg.run_time());

    // collect global performance stats for printPerformanceStatistics
    stats_workUnits++;
    stats_minWorkUnitId = std::min(stats_minWorkUnitId,(long long)msg.work_unit_id());
    stats_maxWorkUnitId = std::max(stats_maxWorkUnitId,(long long)msg.work_unit_id());
    stats_workUnitsSteps += msg.steps();
    stats_workUnitTime += msg.run_time();
    for (int i=0; i<msg.part_status_size(); i++)
        stats_workUnitsParts++;

// TODO: decide if the exists() check code is necessary, and if so fold it into FFluxTrajectoryList
//    // If the trajectory associated with the finished work unit exists...
//    if (trajectoryList->exists(msg.final_state().trajectory_id()))
//    {
//        // ...update the trajectory based on the results of the work unit
//        trajectoryList->workUnitFinished(msg);
//    }
//    // Otherwise, assume that the associated trajectory has already been deleted and so skip reading in this result
//    // The exists() check ensures that hangover results from older fflux phases aren't recorded as belonging to a newer phase

    // Update the trajectory list.
    trajectoryList->workUnitFinished(msg);

    // Update the slots list.
    slots.workUnitFinished(msg);

    // If we are not performing a checkpoint, distribute more work.
    if (!performingCheckpoint)
    {
        // Fill the newly freed slot with a work unit. If there are more trajectories than slots, this is guaranteed to use the slot we just freed. Otherwise it will be the "coldest" (longest unoccupied) slot
        if (assignWork())
        {
            finishSimulationPhase();
        }
    }

        // Otherwise, see if all outstanding work units have finished.
    else if (!slots.hasBusySlots())
    {
        Print::printf(Print::INFO, "Creating a checkpoint, pausing work.");

        // Send a message to the output writer to save a checkpoint.
        lm::message::Message msgp;
        msgp.mutable_perform_checkpointing();
        communicator->sendMessage(outputWriterAddress, &msgp);
    }
}

bool SimulationSupervisor::assignWork()
{
    // Go though the available slots and fill them with work units.
    while (true)
    {
        // Allocate the next free slot, if there is one. AssignWork should return from here, except for when it's the end of a simulation phase or the end of the program.
        if (!slots.hasFreeSlots()) return false;

        // Create the run work unit message.
        lm::message::Message msg;
        lm::message::RunWorkUnit* rwuMsg = msg.mutable_run_work_unit();

        // Build the run work units message.
        buildRunWorkUnitHeader(rwuMsg);

        // Get the free slot.
        const lm::slot::Slot slot = slots.getFreeSlot();

        // Build the work unit parts.
        buildRunWorkUnitParts(rwuMsg, slot.getSimultaneousWorkUnits());

        // See if there were any parts to run.
        if (rwuMsg->part_size() > 0)
        {
            // Run the work unit.
            slots.runWorkUnit(&msg);
        }
        else
        {
            // If there were no work units to run, see if it was because they are all finsished.
            if (trajectoryList->areAllFinished())
            {
                return true;	// When there's no more trajectories to run and it's time for the program to shut down, assignWork should return from here
            }
            else
            {
                return false;	// Some trajectories are still running, there may still be more work units to come
            }
        }
    }
}

void SimulationSupervisor::buildRunWorkUnitHeader(lm::message::RunWorkUnit* msg)
{
    // Set the work unit id.
    msg->set_work_unit_id(workUnitCount++);

    // Set the writer address.
    msg->mutable_output_address()->CopyFrom(outputWriterAddress);

    // Set the limits.
    buildRunWorkUnitLimits(msg);

    // Set the output options.
    msg->mutable_output_options()->CopyFrom(input->getOutputOptionsMsg());

    // Set the maximum number of steps for the work unit.
    msg->set_max_steps(input->getStepsPerWorkUnit());
}

void SimulationSupervisor::buildRunWorkUnitLimits(lm::message::RunWorkUnit* msg)
{
    // Set the limits.
    msg->mutable_trajectory_limits()->CopyFrom(input->getTrajectoryLimits());
}

void SimulationSupervisor::buildRunWorkUnitParts(lm::message::RunWorkUnit* msg, uint minWorkUnits)
{
    trajectoryList->addWorkUnitParts(msg->work_unit_id(), msg, minWorkUnits);
}

void SimulationSupervisor::finishSimulationPhase()
{
    // If we need to perform another phase, do so, otherwsise stop th simulation.
    if (incrementSimulationPhase())
        startSimulationPhase();
    else
        finishSimulation();
}

void SimulationSupervisor::destroyTrajectoryList()
{
    if (trajectoryList != NULL) delete trajectoryList; trajectoryList = NULL;
}

bool SimulationSupervisor::incrementSimulationPhase()
{
    simulationPhase++;
    trajectoryList->incrementSimulationPhase();
    return false;
}

void SimulationSupervisor::finishSimulation()
{
    Print::printf(Print::INFO, "Simulation finished.");

    // Mark that the simulation is finished so we exit our message loop.
    simulationRunning = false;

    // Stop all of the resource controllers.
    map<string,ComputeResources> resources = resourceMap.getAvailableResources();
    for (map<string,ComputeResources>::iterator it=resources.begin(); it != resources.end(); it++)
    {
        // Send a message for the resource controller to stop.
        lm::message::Message msg;
        msg.mutable_stop_resource_controller()->set_abort(false);
        communicator->sendMessage(it->second.controllerAddress, &msg);
    }

    // Delete the list of trajectories.
    destroyTrajectoryList();
}

void SimulationSupervisor::receivedPerformCheckpointing(const lm::message::PerformCheckpointing& msg)
{
    if (simulationRunning)
    {
        // Mark that we need to perform a checkpoint, so distribution of work units should pause until checkpointing is finished.
        performingCheckpoint = true;
    }
}

void SimulationSupervisor::receivedFinishedCheckpointing(const lm::message::FinishedCheckpointing& msg)
{
    Print::printf(Print::INFO, "Finished creating a checkpoint, resuming work.");

    // Mark that we are done checkpointing.
    performingCheckpoint = false;

    // Resume distribution of work.
    if (assignWork())
    {
        Print::printf(Print::INFO, "Simulation finished.");
        finishSimulation();
    }
}

void SimulationSupervisor::receivedProcessWorkUnitOutput(lm::message::Message& msg)
{
}

bool SimulationSupervisor::receivedOther(lm::message::Message& msg)
{
    return false;
}

void SimulationSupervisor::printPerformanceStatistics(bool flush)
{
    // See if we should display and reset the performance stats.
    hrtime currentTime = getHrTime();
    if (flush || convertHrToSeconds(currentTime-stats_lastPrintTime) > 60.0)
    {
        if (stats_workUnits > 0)
        {
            Print::printf(Print::INFO, "Finished %lld work units (ids in range %lld to %lld) with %lld parts in the last %0.1f seconds. %lld steps in %0.3e seconds (%0.3e steps/second).",stats_workUnits,stats_minWorkUnitId,stats_maxWorkUnitId,stats_workUnitsParts,convertHrToSeconds(currentTime-stats_lastPrintTime), stats_workUnitsSteps, stats_workUnitTime, double(stats_workUnitsSteps)/stats_workUnitTime);
        }
        stats_lastPrintTime = currentTime;
        resetPerformanceStatistics();
    }
}

void SimulationSupervisor::resetPerformanceStatistics()
{
    stats_lastPrintTime = getHrTime();
    stats_workUnits = 0;
    stats_workUnitsParts = 0;
    stats_minWorkUnitId = std::numeric_limits<long long>::max();
    stats_maxWorkUnitId = 0;
    stats_workUnitsSteps = 0;
    stats_workUnitTime = 0.0;
}

}
}
