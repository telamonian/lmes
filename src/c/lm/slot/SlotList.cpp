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

#include <climits>
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <string>
#include <vector>
#include "lm/Exceptions.h"
#include "lm/Math.h"
#include "lm/MPI.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/message/Communicator.h"
#include "lm/resource/ComputeResources.h"
#include "lm/slot/Slot.h"
#include "lm/slot/SlotList.h"
#include "lm/thread/Thread.h"

using lm::resource::ComputeResources;
using lm::thread::PthreadException;
using lm::slot::Slot;
using std::string;
using std::vector;

namespace lm {
namespace slot {

SlotList::SlotList(lm::message::Communicator * communicator): communicator(communicator)
{
}

SlotList::~SlotList()
{
}

void SlotList::createAllSlots(map<int,ComputeResources> & allResources, double cpusPerSlot, double gpusPerSlot, bool useCPUAffinity, string solver, const lm::input::Input& input)
{
    int nextSlotId = 0;
    for (map<int,ComputeResources>::iterator it=allResources.begin(); it != allResources.end(); it++)
	{
        nextSlotId += createProcessSlots(nextSlotId, it->first, it->second, cpusPerSlot, gpusPerSlot, useCPUAffinity, solver, input);
	}
}

int SlotList::createProcessSlots(int startingSlotId, int process, ComputeResources resources, double cpusPerSlot, double gpusPerSlot, bool useCPUAffinity, string solver, const lm::input::Input& input)
{
    // Make sure we are using the correct process.
    if (process != resources.controller_process)
        throw Exception("Mismatch between slot process and controller process.",process,resources.controller_process);

    // Make sure that a least one compute resource was being requested, otherwise we can create infinite slots.
    if (cpusPerSlot == 0.0 && gpusPerSlot == 0.0)
        throw Exception("No compute resources were requested for each work unit runner.");

    // Figure out the constraints on the number of slots.
    int cpuSlotsConstraint = (cpusPerSlot > 0)?(int(floor(double(resources.cpuCores.size())/cpusPerSlot))):(INT_MAX);
    int gpuSlotsConstraint = (gpusPerSlot > 0)?(int(floor(double(resources.gpuDevices.size())/gpusPerSlot))):(INT_MAX);

    // Loop while we have enough resources to create another slot.
    int i;
    for (i=0; i<min(cpuSlotsConstraint,gpuSlotsConstraint); i++)
    {
        // Create the resources object for the slot.
        ComputeResources slotResources;
        slotResources.hostname = resources.hostname;
        slotResources.controller_process = resources.controller_process;
        slotResources.controller_thread = resources.controller_thread;

        // Assign the cpu resources.
        if (cpusPerSlot >= 1.0)
        {
            // Assign sequential resources to the same slot.
            int cpuResourcesToAssign=int(floor(cpusPerSlot));
            for (int j=0; j<cpuResourcesToAssign; j++)
            {
                slotResources.cpuCores.push_back(resources.cpuCores[i*cpuResourcesToAssign+j]);
            }
        }
        else if (cpusPerSlot > 0.0)
        {
            // Assign the same resource to multiple slots using a round-robin approach.
            slotResources.cpuCores.push_back(resources.cpuCores[i%resources.cpuCores.size()]);
        }

        // Assign the gpu resources.
        if (gpusPerSlot >= 1.0)
        {
            // Assign sequential resources to the same slot.
            int gpuResourcesToAssign=int(floor(gpusPerSlot));
            for (int j=0; j<gpuResourcesToAssign; j++)
            {
                slotResources.gpuDevices.push_back(resources.gpuDevices[i*gpuResourcesToAssign+j]);
            }
        }
        else if (gpusPerSlot > 0.0)
        {
            // Assign the same resource to multiple slots using a round-robin approach.
            slotResources.gpuDevices.push_back(resources.gpuDevices[i%resources.gpuDevices.size()]);
        }

        // Create the message to start the workers.
        lm::message::Message msg;

        // Create the slot.
        createSlot(startingSlotId+i, slotResources, useCPUAffinity, &msg, solver, input);

        // Send the message to create all of the work units runners for this process.
        communicator->sendMessage(resources.controller_process, resources.controller_thread, &msg);

    }

    // Return the number of slots that were created.
    return i;
}

void SlotList::createSlot(int slotId, ComputeResources resources, bool useCPUAffinity, lm::message::Message* msg, string solver, const lm::input::Input& input)
{
    Print::printf(Print::INFO, "Creating slot %d on process (%d:%d) using resources: %s.", slotId, resources.controller_process, resources.controller_thread, resources.toString().c_str());

    // Add the slots to our list.
    slots.push_back(Slot(slotId,resources));

    // Add the start work unit runner message for the slot.
    lm::message::StartWorkUnitRunner* s = msg->add_start_work_unit_runner();
    s->set_work_unit_runner_id(slotId);
    s->set_use_cpu_affinity(useCPUAffinity);
    for (vector<int>::iterator it=resources.cpuCores.begin(); it != resources.cpuCores.end(); it++)
        s->add_cpu(*it);
    for (vector<int>::iterator it=resources.gpuDevices.begin(); it != resources.gpuDevices.end(); it++)
        s->add_gpu(*it);
    s->set_solver(solver);
    if (input.hasReactionModel())
        s->mutable_reaction_model()->CopyFrom(input.getReactionModel());
    if (input.hasDiffusionModel())
        s->mutable_diffusion_model()->CopyFrom(input.getDiffusionModel());
    if (input.hasOrderParameters())
        s->mutable_order_parameters()->CopyFrom(input.getOrderParametersMsg());
    if (input.hasTilings())
        s->mutable_tilings()->CopyFrom(input.getTilingsMsg());
}

void SlotList::markSlotStarted(const lm::message::StartedWorkUnitRunner& msg)
{
    // Mark the work unit runner as started.
    if (msg.work_unit_runner_id() < 0 || msg.work_unit_runner_id() >= (int)slots.size()) throw Exception("Invalid work unit runner id received in started work unit runner message",msg.work_unit_runner_id());
    if (slots[msg.work_unit_runner_id()].status != Slot::NOT_STARTED) throw Exception("Work unit runner was previosuly started",msg.work_unit_runner_id());
    slots[msg.work_unit_runner_id()].status = Slot::FREE;
    slots[msg.work_unit_runner_id()].workUnitRunnerEndpoint.process = msg.process();
    slots[msg.work_unit_runner_id()].workUnitRunnerEndpoint.thread = msg.thread();
    slots[msg.work_unit_runner_id()].simultaneousWorkUnits = msg.simultaneous_work_units();
}

bool SlotList::hasUnstartedSlots()
{
    // Check all of the slots to see if anything is unstarted.
    for (size_t i=0; i<slots.size(); i++)
    {
        if (slots[i].status == Slot::NOT_STARTED)
            return true;
    }
    return false;
}

bool SlotList::hasFreeSlots()
{
    // Check all of the slots to see if anything is free.
    for (size_t i=0; i<slots.size(); i++)
    {
        if (slots[i].status == Slot::FREE)
            return true;
    }
    return false;
}

const Slot& SlotList::getFreeSlot()
{
    // Find the first free slot.
    for (size_t i=0; i<slots.size(); i++)
    {
        if (slots[i].status == Slot::FREE)
            return slots[i];
    }
    throw Exception("No free slots available.");
}

bool SlotList::hasBusySlots()
{
    // Check all of the slots to see if anything is free.
    for (size_t i=0; i<slots.size(); i++)
    {
        if (slots[i].status == Slot::BUSY)
            return true;
    }
    return false;
}

void SlotList::runWorkUnit(lm::message::Message* runWorkUnitMsg)
{
    int64_t workUnitId = (int64_t)runWorkUnitMsg->run_work_unit().work_unit_id();

    // Make sure we are not processing this work unit.
    if (workUnitToSlotMap.count(workUnitId) > 0)
        throw new Exception("The work unit is already assigned to a work unit runner",workUnitId,workUnitToSlotMap[workUnitId]);

    // Find a free slot.
    for (size_t i=0; i<slots.size(); i++)
    {
        if (slots[i].status == Slot::FREE)
        {
            // Mark the slot as busy.
            slots[i].status = Slot::BUSY;

            // Track the association between the work unit and the slot.
            workUnitToSlotMap[workUnitId] = i;

            // Send the message to start the work unit.
            communicator->sendMessage(slots[i].workUnitRunnerEndpoint.process, slots[i].workUnitRunnerEndpoint.thread, runWorkUnitMsg);
            return;
        }
    }
    throw Exception("Could not find a free slot to run the work unit",workUnitId);
}

void SlotList::workUnitFinished(const lm::message::FinishedWorkUnit& msg)
{
    int64_t workUnitId = msg.work_unit_id();

    // Make sure we were processing this work unit.
    if (workUnitToSlotMap.count(workUnitId) == 0) throw new Exception("The completed work unit was not found",workUnitId);

    // Get the slot that was running the work unit.
    int slotId = workUnitToSlotMap[workUnitId];

    // Make sure the slot process and thread match with that on the message.
    if (slots[slotId].workUnitRunnerEndpoint.process != msg.process() || slots[slotId].workUnitRunnerEndpoint.thread != msg.thread()) throw Exception("Mismatch between slot endpoint and work unit runner endpoint");

    // Make sure the slot was correctly marked as busy.
    if (slots[slotId].status != Slot::BUSY) throw Exception("Work unit runner was not marked as busy while running work unit",slotId,workUnitId);

    // Erase the work unit from our map.
    workUnitToSlotMap.erase(workUnitId);

    // Mark the slot as free.
    slots[slotId].status = Slot::FREE;
}

}
}
