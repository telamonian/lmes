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

#include "hrtime.h"
#include "lm/EnumHelper.h"
#include "lm/Exceptions.h"
#include "lm/MPI.h"
#include "lm/Print.h"
#include "lm/input/DiffusionModel.pb.h"
#include "lm/io/hdf5/HDF5.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/main/Main.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Communicator.h"
#include "lm/message/FinishedCheckpointing.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ResourcesAvailable.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/message/StartedWorkUnitRunner.pb.h"
#include "lm/message/WorkUnit.pb.h"
#include "lm/protowrap/Repeated.h"
#include "lm/resource/ComputeResources.h"
#include "lm/resource/ResourceMap.h"
#include "lm/slot/Slot.h"
#include "lm/slot/SlotList.h"

using lm::resource::ComputeResources;
using lm::resource::ResourceMap;
using std::string;

namespace lm {
namespace main {

// if >0, we use a hand-rolled mpi receive polling scheme in order to reduce the supervisor cpu%
int SimulationSupervisor::getRecvSleepMilliseconds()
{
    return 5;
}

SimulationSupervisor::SimulationSupervisor()
:communicator(lm::MPI::worldRank,THREAD_ID),hasCheckpointSignalerStarted(false),hasOutputWriterStarted(false),haveAllWorkUnitRunnersStarted(false),
 input(NULL),outputWriterClassName(""),outputWriterProcess(-1),outputWriterThread(-1),performingCheckpoint(false),
 resourceMap(NULL),simulationInputFilename(""),simulationOutputFilename(""),simulationPhaseIndex(0),simulationRunning(true),
 simulationPhaseEverTerminated(false),slots(&communicator),solverClassName(""),trajectoryList(NULL),useCPUAffinity(false),workUnitCount(0)
{
    resetPerformanceStatistics();
}

SimulationSupervisor::~SimulationSupervisor()
{
    destructInput();
    destructTrajectory();
}

void SimulationSupervisor::init()
{
    // Initialize the input object with the input file.
    setInput(new lm::input::Input(lm::io::hdf5::Hdf5File(simulationInputFilename)));
}

void SimulationSupervisor::wake() throw(lm::thread::PthreadException)
{
    lm::message::Message msg;
    msg.mutable_ping_target()->set_id(0);
    communicator.sendMessage(communicator.getSourceProcess(), communicator.getSourceThread(), &msg);
}

int SimulationSupervisor::run()
{
    try
    {
        Print::printf(Print::INFO, "Supervisor %d:%d started.", lm::MPI::worldRank, threadNumber);
        // Loop reading messages.
        lm::message::Message message;
        while (running && simulationRunning)
        {
            // Read the next message.
            communicator.receiveMessage(&message, getRecvSleepMilliseconds());

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

        Print::printf(Print::INFO, "Supervisor %d:%d finished.", lm::MPI::worldRank, threadNumber);

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
    Print::printf(Print::INFO, "Resource controller %d:%d on %s registered with %d cpu core(s) and %d gpu device(s).", msg.controller_process(), msg.controller_thread(), msg.hostname().c_str(), msg.cpu_size(), msg.gpu_size());
    if (resourceMap->registerResources(msg))
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
        ComputeResources resources = resourceMap->reserveCPUCores(communicator.getSourceProcess(),1);
        Print::printf(Print::INFO, "Reserved core %d on %d:%d for the output writer.", resources.cpuCores[0], resources.controller_process, resources.controller_thread);

        // Start the output writer.
        lm::message::Message msg;
        msg.mutable_start_output_writer()->set_use_cpu_affinity(useCPUAffinity);
        msg.mutable_start_output_writer()->set_cpu(resources.cpuCores[0]);
        msg.mutable_start_output_writer()->set_output_filename(simulationOutputFilename);
        msg.mutable_start_output_writer()->set_output_writer_class(outputWriterClassName);
        communicator.sendMessage(resources.controller_process, resources.controller_thread, &msg);
    }
        // Otherwise, just use core 0 on the Supervisor process
    else
    {
        Print::printf(Print::INFO, "Output writer is sharing core %d on process %d.", 0, communicator.getSourceProcess());
        // Start the output writer.
        lm::message::Message msg;
        msg.mutable_start_output_writer()->set_use_cpu_affinity(useCPUAffinity);
        msg.mutable_start_output_writer()->set_cpu(0);
        msg.mutable_start_output_writer()->set_output_filename(simulationOutputFilename);
        msg.mutable_start_output_writer()->set_output_writer_class(outputWriterClassName);
        // thread 1 should be the resource controller
        communicator.sendMessage(communicator.getSourceProcess(), 1, &msg);
    }
}

void SimulationSupervisor::startCheckpointSignaler()
{
    //See if we need to start a checkpoint signaler.
    if (checkpointInterval > 0)
    {
        // Get the resource controller for the eprocess.
        ComputeResources resources = resourceMap->getController(communicator.getSourceProcess());

        // Start the checkpoint signaler.
        lm::message::Message msg;
        msg.mutable_start_checkpoint_signaler()->set_supervisor_process(communicator.getSourceProcess());
        msg.mutable_start_checkpoint_signaler()->set_supervisor_thread(communicator.getSourceThread());
        msg.mutable_start_checkpoint_signaler()->set_checkpoint_interval(checkpointInterval);
        communicator.sendMessage(resources.controller_process, resources.controller_thread, &msg);
    }
    else
    {
        hasCheckpointSignalerStarted = true;
    }
}

void SimulationSupervisor::startWorkUnitRunners()
{
    map<int,ComputeResources> allResources = resourceMap->getAvailableResources();
    slots.createAllSlots(allResources, cpuCoresPerRunner, gpuDevicesPerRunner, useCPUAffinity, solverClassName, *input);
}

void SimulationSupervisor::receivedStartedOutputWriter(const lm::message::StartedOutputWriter& msg)
{
    Print::printf(Print::INFO, "Output writer started: %d:%d.",msg.process(),msg.thread());
    hasOutputWriterStarted = true;
    outputWriterProcess = msg.process();
    outputWriterThread = msg.thread();
    startSimulationIfAllWorkersStarted();
}

void SimulationSupervisor::receivedStartedCheckpointSignaler(const lm::message::StartedCheckpointSignaler& msg)
{
    Print::printf(Print::INFO, "Checkpoint signaller started: %d:%d.",msg.process(),msg.thread());
    hasCheckpointSignalerStarted = true;
    startSimulationIfAllWorkersStarted();
}

void SimulationSupervisor::receivedStartedWorkUnitRunner(const lm::message::StartedWorkUnitRunner & msg)
{
    Print::printf(Print::INFO, "Work unit runner started: %d:%d.",msg.process(),msg.thread());

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
    if (terminateSimulationPhase() || assignWork())
    {
        // If .terminateSimulationPhase() or .assignWork() returned true, there was nothing to be done.
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

    // Update the trajectory list.
    trajectoryList->workUnitFinished(msg);

    // Update the slots list.
    slots.workUnitFinished(msg);

    // If we are not performing a checkpoint, distribute more work.
    if (!performingCheckpoint)
    {
        // Fill the newly freed slot with a work unit. If there are more trajectories than slots, this is guaranteed to use the slot we just freed. Otherwise it will be the "coldest" (longest unoccupied) slot
        if (terminateSimulationPhase() || assignWork())
        {
            finishSimulationPhase();
        }
    }
    // Otherwise, see if all outstanding work units have finished.
    else if (!slots.hasBusySlots())
    {
        Print::printf(Print::INFO, "Creating a checkpoint, pausing work.");

        // Send a message to the output writer to save a checkpoint. Calling .mutable_perform_checkpointing() initializes the message
        lm::message::Message msgp;
        msgp.mutable_perform_checkpointing();
        communicator.sendMessage(outputWriterProcess, outputWriterThread, &msgp);
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

    // Set the source process/thread.
    msg->set_supervisor_process(communicator.getSourceProcess());
    msg->set_supervisor_thread(communicator.getSourceThread());

    // Set the writer process/thread.
    msg->set_output_process(outputWriterProcess);
    msg->set_output_thread(outputWriterThread);

    // Set the limits.
    input->copyLimitsTo(msg);

    // Set the output options.
    msg->mutable_output_options()->CopyFrom(input->getOutputOptionsMsg());

    // Set the maximum number of steps for the work unit.
    msg->set_max_steps(input->getStepsPerWorkUnit());
}

void SimulationSupervisor::buildRunWorkUnitParts(lm::message::RunWorkUnit* msg, uint minWorkUnits)
{
    trajectoryList->addWorkUnitParts(msg->work_unit_id(), msg, minWorkUnits);

    // Set the limit tracking messages, if any
    input->copyLimitTrackingsTo(msg);
}

/*
 * - terminateSimulationPhase() serves as a hook for more complex phase-ending behavior in subclassed Supervisors.
 *     - All non-trivial versions should set the simulationPhaseEverTerminated (see FFluxSupervisor for an example)
 */
bool SimulationSupervisor::terminateSimulationPhase()
{
    // simulationPhaseEverTerminated = false;
    return false;
}

void SimulationSupervisor::finishSimulationPhase()
{
    // If we need to perform another phase, do so, otherwise stop the simulation.
    if (performAnotherSimulationPhase())
    {
        incrementSimulationPhase();
        startSimulationPhase();
    }
    else
    {
        finishSimulation();
    }
}

bool SimulationSupervisor::performAnotherSimulationPhase()
{
    return false;
}

void SimulationSupervisor::incrementSimulationPhase()
{
    simulationPhaseIndex++;
}

void SimulationSupervisor::finishSimulation()
{
    if (trajectoryList->getTrajectoryMap(lm::trajectory::Trajectory::RUNNING)->size() > 0)
    {
        trajectoryList->setAll(lm::trajectory::Trajectory::RUNNING, lm::trajectory::Trajectory::ABORTED);
    }
    if (trajectoryList->getTrajectoryMap(lm::trajectory::Trajectory::ABORTED)->size() > 0)
    {
        // If the simulation phase was ever forcibly terminated, make sure we clean up any running trajectories appropriately
        if (simulationPhaseEverTerminated)
        {
            return (void)0;
        }
            // Otherwise, the default supervisor behavior is to throw an exception if there are trajectories still running at the end of a phase
        else
        {
            throw ConsistencyException("At end of simulation, there were %d trajectories still running (should be 0)", trajectoryList->getTrajectoryMap(lm::trajectory::Trajectory::RUNNING)->size());
        }
    }

    Print::printf(Print::INFO, "Simulation finished.");

    // Mark that the simulation is finished so we exit our message loop.
    simulationRunning = false;

    // Stop all of the resource controllers.
    map<int,ComputeResources> resources = resourceMap->getAvailableResources();
    for (map<int,ComputeResources>::iterator it=resources.begin(); it != resources.end(); it++)
    {
        // Send a message for the resource controller to stop.
        lm::message::Message msg;
        msg.mutable_stop_resource_controller()->set_abort(false);
        communicator.sendMessage(it->second.controller_process, it->second.controller_thread, &msg);
    }
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
    if (terminateSimulationPhase() || assignWork())
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

// setters/destructors for attributes that may be shadowed by derived class attributes
void SimulationSupervisor::setInput(lm::input::Input* newInput)
{
    destructInput();
    input = newInput;
}

void SimulationSupervisor::setTrajectoryList(lm::trajectory::TrajectoryList* newTrajectoryList)
{
    if (trajectoryList != NULL)
    {
        if (trajectoryList->getTrajectoryMap(lm::trajectory::Trajectory::RUNNING)->size() > 0)
        {
            // If the simulation phase was ever forcibly terminated, make sure we clean up any running trajectories appropriately
            if (simulationPhaseEverTerminated)
            {
                // Keep track of any outstanding work units. Important for coordinating clean program termination across all nodes
                newTrajectoryList->takeTrajectories(trajectoryList, lm::trajectory::Trajectory::ABORTED, lm::trajectory::Trajectory::ABORTED);
                newTrajectoryList->takeTrajectories(trajectoryList, lm::trajectory::Trajectory::RUNNING, lm::trajectory::Trajectory::ABORTED);
                newTrajectoryList->takeWorkUnitsRunning(trajectoryList);
            }
                // Otherwise, the default supervisor behavior is to throw an exception if there are trajectories still running at the end of a phase
            else
            {
                throw ConsistencyException("At end of simulation phase, there were %d trajectories still running (should be 0)", trajectoryList->getTrajectoryMap(lm::trajectory::Trajectory::RUNNING)->size());
            }
        }
    }

    destructTrajectory();
    trajectoryList = newTrajectoryList;
}

// private
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
