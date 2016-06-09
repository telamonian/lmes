/*
 * Copyright 2016 Johns Hopkins University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
 *
 * Author(s): Elijah Roberts
 */

#include <map>
#include <string>

#include "hrtime.h"
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
#include "lm/microenv/MicroenvironmentSupervisor.h"
#include "lm/microenv/MicroenvironmentTrajectoryList.h"
#include "lm/resource/ResourceMap.h"
#include "lm/slot/SlotList.h"

using std::map;
using std::string;
using lm::resource::ResourceMap;

namespace lm {
namespace microenv {

bool MicroenvironmentSupervisor::registered=MicroenvironmentSupervisor::registerClass();

bool MicroenvironmentSupervisor::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::main::SimulationSupervisor","lm::microenv::MicroenvironmentSupervisor",&MicroenvironmentSupervisor::allocateObject);
    return true;
}

void* MicroenvironmentSupervisor::allocateObject()
{
    return new MicroenvironmentSupervisor();
}

MicroenvironmentSupervisor::MicroenvironmentSupervisor()
:simulationStartTime(0),numberReplicates(replicates.size()),currentReplicateIndex(0),numberTimesteps(10),currentTimestep(0),
pdeSlots(&communicator),pdeSolverClassName(""),pdeTrajectoryList(NULL),
stats_pdeWorkUnitsSteps(0),stats_pdeWorkUnitsTime(0.0)
{
#ifdef OPT_AVX
    pdeSolverClassName = "lm::avx::ExplicitFiniteDifferenceSolverAVX";
#else
    pdeSolverClassName = "lm::pde::ExplicitFiniteDifferenceSolver";
#endif
}

MicroenvironmentSupervisor::~MicroenvironmentSupervisor()
{
    if (pdeTrajectoryList != NULL) delete pdeTrajectoryList; pdeTrajectoryList = NULL;
    if (trajectoryList != NULL) delete trajectoryList; trajectoryList = NULL;
}

void MicroenvironmentSupervisor::startWorkUnitRunners()
{
    // Start the work unit runners for the PDE solvers.
    ComputeResources pdeResources = resourceMap->reserveCPUCores(1);
    pdeSlots.createAllSlots(pdeResources, 1, 0, useCPUAffinity, pdeSolverClassName, *input);

    // Start the work unit runners for the ME solvers using the base supervisor.
    SimulationSupervisor::startWorkUnitRunners();
}

void MicroenvironmentSupervisor::receivedStartedWorkUnitRunner(const lm::message::StartedWorkUnitRunner & msg)
{
    Print::printf(Print::INFO, "Work unit runner %d on process (%d:%d) reported to supervisor.",msg.work_unit_runner_id(),msg.process(),msg.thread());

    if (pdeSlots.isManagingSlot(msg.work_unit_runner_id()))
        pdeSlots.markSlotStarted(msg);
    else
        slots.markSlotStarted(msg);

    if (!slots.hasUnstartedSlots() && !pdeSlots.hasUnstartedSlots())
    {
        haveAllWorkUnitRunnersStarted = true;
        startSimulationIfAllWorkersStarted();
    }
}

void MicroenvironmentSupervisor::startSimulation()
{
    simulationStartTime=getHrTime();

    // Check for some error conditions.
    if (outputWriterProcess == -1 || outputWriterThread == -1)
        throw new Exception("MicroenvironmentSupervisor could not start the simulation, no output writer available.");

    Print::printf(Print::INFO, "Microenvironment supervisor starting simulation.");

    // Call the base class method
    SimulationSupervisor::startSimulation();
}

void MicroenvironmentSupervisor::startSimulationPhase()
{
    // See if we should start of a new replicate or continue with the current one.
    if (currentTimestep == 0)
        startNewReplicate();
    else
        continueCurrentReplicate();

    // Assign the first batch of work.
    if (assignWork())
    {
        finishSimulationPhase();
    }
}

void MicroenvironmentSupervisor::startNewReplicate()
{
    // Create a new trajectory list.
    buildTrajectoryList();

    // Create a new diffusion grid.

    // Run the diffusion solver for a timestep.
}

void MicroenvironmentSupervisor::continueCurrentReplicate()
{
    printf("Updating to timestep %d\n",currentTimestep);

    // Reconcile the cells and the diffusion grid.

    // Update the trajectory list to run for another timestep.

    // Run the diffusion solver for another timestep.
}

void MicroenvironmentSupervisor::buildTrajectoryList()
{
    // Free the old trajectory lists, if they exist.
    if (pdeTrajectoryList != NULL) delete pdeTrajectoryList; pdeTrajectoryList = NULL;
    if (trajectoryList != NULL) delete trajectoryList; trajectoryList = NULL;

    // Allocate the new lists.
    pdeTrajectoryList = new MicroenvironmentTrajectoryList(*input, replicates[currentReplicateIndex]);
    trajectoryList = new MicroenvironmentTrajectoryList(*input, replicates[currentReplicateIndex]);
}

void MicroenvironmentSupervisor::receivedFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg)
{
    // Collect global performance stats.
    stats_workUnits++;
    stats_minWorkUnitId = std::min(stats_minWorkUnitId,(long long)msg.work_unit_id());
    stats_maxWorkUnitId = std::max(stats_maxWorkUnitId,(long long)msg.work_unit_id());
    for (int i=0; i<msg.part_status_size(); i++)
        stats_workUnitsParts++;

    // See if this is a pde work unit or a me work unit.
    if (pdeSlots.isRunningWorkUnit(msg.work_unit_id()))
    {
        // Collect some additional stats.
        stats_pdeWorkUnitsSteps += msg.steps();
        stats_pdeWorkUnitsTime += msg.run_time();

        // Update the trajectory list.
        pdeTrajectoryList->workUnitFinished(msg);

        // Update the slots list.
        pdeSlots.workUnitFinished(msg);
    }
    else
    {
        // Collect some additional stats.
        stats_workUnitsSteps += msg.steps();
        stats_workUnitTime += msg.run_time();

        // Update the trajectory list.
        trajectoryList->workUnitFinished(msg);

        // Update the slots list.
        slots.workUnitFinished(msg);
    }

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
    else if (!slots.hasBusySlots() && !pdeSlots.hasBusySlots())
    {
        Print::printf(Print::INFO, "Creating a checkpoint, pausing work.");

        // Send a message to the output writer to save a checkpoint.
        lm::message::Message msgp;
        lm::message::PerformCheckpointing* msg = msgp.mutable_perform_checkpointing();
        communicator.sendMessage(outputWriterProcess, outputWriterThread, &msgp);
    }
}

bool MicroenvironmentSupervisor::assignWork()
{
    // Go though the available slots and fill them with work units.
    while (true)
    {
        // Assign any work, if we can.
        if (pdeSlots.hasFreeSlots() && pdeTrajectoryList->areAnyWaiting())
        {

        }
        else if (slots.hasFreeSlots() && trajectoryList->areAnyWaiting())
        {
            // Create the run work unit message.
            lm::message::Message msg;
            lm::message::RunWorkUnit* rwuMsg = msg.mutable_run_work_unit();

            // Build the run work units message.
            buildRunWorkUnitHeader(rwuMsg);

            // Get the free slot.
            const lm::slot::Slot slot = slots.getFreeSlot();

            // Build the work unit parts.
            buildRunWorkUnitParts(rwuMsg, slot.getSimultaneousWorkUnits());
            if (rwuMsg->part_size() == 0) throw Exception("consistency error in MicroenvironmentSupervisor::assignWork, the work unit had no parts");

            // Run the work unit.
            slots.runWorkUnit(&msg);
        }
        else
        {
            // Return if we are done with all the work yet.
            return (pdeTrajectoryList->areAllFinished() && trajectoryList->areAllFinished());
        }
    }
}

bool MicroenvironmentSupervisor::performAnotherSimulationPhase()
{
    // Return true if we have either another timestep or another replicate to run.
    return ((currentTimestep+1) < numberTimesteps || (currentReplicateIndex+1) < numberReplicates);
}

void MicroenvironmentSupervisor::incrementSimulationPhase()
{
    SimulationSupervisor::incrementSimulationPhase();

    // Increment the timestep.
    currentTimestep++;

    // If all of the timesteps are done for this replicate, move to the next.
    if (currentTimestep >= numberTimesteps)
    {
        currentTimestep = 0;
        currentReplicateIndex++;
    }
}

void MicroenvironmentSupervisor::finishSimulation()
{
    Print::printf(Print::INFO, "MicroenvironmentSupervisor supervisor finished %u timesteps for %u replicates in %0.2f seconds.", numberTimesteps, numberReplicates, convertHrToSeconds(getHrTime()-simulationStartTime));
    SimulationSupervisor::finishSimulation();
}

void MicroenvironmentSupervisor::printPerformanceStatistics(bool flush)
{
    // See if we should display and reset the performance stats.
    hrtime currentTime = getHrTime();
    if (flush || convertHrToSeconds(currentTime-stats_lastPrintTime) > 60.0)
    {
        if (stats_workUnits > 0)
        {
            Print::printf(Print::INFO, "Finished %lld work units (ids in range %lld to %lld) with %lld parts in the last %0.1f seconds.",stats_workUnits,stats_minWorkUnitId,stats_maxWorkUnitId,stats_workUnitsParts,convertHrToSeconds(currentTime-stats_lastPrintTime));
            if (stats_workUnitsSteps > 0) Print::printf(Print::INFO, "ME solvers performed %lld steps in %0.3e seconds (%0.3e steps/second).", stats_workUnitsSteps, stats_workUnitTime, double(stats_workUnitsSteps)/stats_workUnitTime);
            if (stats_pdeWorkUnitsSteps > 0) Print::printf(Print::INFO, "PDEE solvers performed %lld steps in %0.3e seconds (%0.3e steps/second).", stats_pdeWorkUnitsSteps, stats_pdeWorkUnitsTime, double(stats_pdeWorkUnitsSteps)/stats_pdeWorkUnitsTime);
        }
        stats_lastPrintTime = currentTime;
        resetPerformanceStatistics();
    }
}

void MicroenvironmentSupervisor::resetPerformanceStatistics()
{
    SimulationSupervisor::resetPerformanceStatistics();

    stats_pdeWorkUnitsSteps = 0LL;
    stats_pdeWorkUnitsTime = 0.0;
}

}
}
