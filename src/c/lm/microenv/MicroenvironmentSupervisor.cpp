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
pdeSlots(&communicator),pdeSolverClassName(""),pdeTrajectoryList(NULL)
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

}
}
