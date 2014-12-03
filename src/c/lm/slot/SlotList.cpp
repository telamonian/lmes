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

void SlotList::createAllSlots(map<int,ComputeResources> & allResources, double cpusPerSlot, double gpusPerSlot, bool useCPUAffinity, string solver, lm::input::Input* input)
{
    int nextSlotId = 0;
    for (map<int,ComputeResources>::iterator it=allResources.begin(); it != allResources.end(); it++)
	{
        nextSlotId += createProcessSlots(nextSlotId, it->first, it->second, cpusPerSlot, gpusPerSlot, useCPUAffinity, solver, input);
	}
}

int SlotList::createProcessSlots(int startingSlotId, int process, ComputeResources resources, double cpusPerSlot, double gpusPerSlot, bool useCPUAffinity, string solver, lm::input::Input* input)
{
    // Make sure we are using the correct process.
    if (process != resources.controller_process)
        throw Exception("Mismatch between slot process and controller process.",process,resources.controller_process);

    // Make sure that a least one compute resource was being requested, otherwise we can create infinite slots.
    if (cpusPerSlot == 0.0 && gpusPerSlot == 0.0)
        throw Exception("No compute resources were requested for each work unit runner.");

    // Create the message to start the workers.
    lm::message::Message msg;

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

        // Create the slot.
        createSlot(startingSlotId+i, slotResources, useCPUAffinity, &msg, solver, input);
    }

    // Send the message to create all of the work units runners for this process.
    communicator->sendMessage(resources.controller_process, resources.controller_thread, &msg);

    // Return the number of slots that were created.
    return i;
}

void SlotList::createSlot(int slotId, ComputeResources resources, bool useCPUAffinity, lm::message::Message* msg, string solver, lm::input::Input* input)
{
    Print::printf(Print::DEBUG, "Creating slot %d on process (%d:%d) using resources: %s.", slotId, resources.controller_process, resources.controller_thread, resources.toString().c_str());

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
    (*s->mutable_simulation_parameters()) = input->simulationParametersBuf;
    if (input->hasReactionModel)
        (*s->mutable_reaction_model()) = input->reactionModelBuf;
    if (input->hasDiffusionModel)
        (*s->mutable_diffusion_model()) = input->diffusionModelBuf;
    if (input->hasOrderParameters)
        (*s->mutable_order_parameters()) = input->orderParametersBuf;
    if (input->hasTilings)
        (*s->mutable_tilings()) = input->tilingsBuf;
}

void SlotList::markSlotStarted(const lm::message::StartedWorkUnitRunner & msg)
{
    // Mark the work unit runner as started.
    if (msg.work_unit_runner_id() < 0 || msg.work_unit_runner_id() >= (int)slots.size()) throw Exception("Invalid work unit runner id received in started work unit runner message",msg.work_unit_runner_id());
    if (slots[msg.work_unit_runner_id()].status != Slot::NOT_STARTED) throw Exception("Work unit runner was previosuly started",msg.work_unit_runner_id());
    slots[msg.work_unit_runner_id()].status = Slot::FREE;
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

void SlotList::runWorkUnit(lm::message::Message* runWorkUnitMsg)
{
    uint64_t workUnitId = runWorkUnitMsg->run_work_unit().work_unit_id();

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

/*
Slot * SlotList::getSlot(int process, int thread)
{
    SlotMap::iterator m_it(getBusySlotIt(process, thread));
    if (m_it!=busySlots.end()) {  //the slot we're trying to get is currently busy
        return m_it->second;
    }
    else {
        SlotDeque::iterator d_it(getFreeSlotIt(process, thread));
        if (d_it!=freeSlots.end()) {    //the slot we're trying to get is currently free
            return *d_it;
        }
        else {  //possible error state: the slot that we're trying to get doesn't exist
            Print::printf(Print::ERROR, "Tried to get slot %d:%d, but was not found in either container of free or busy slots.", process, thread);
        }
    }
    return NULL;
}

Slot * SlotList::getSlotByUUID(uint32_t uuid)
{
	SlotMap::iterator m_it(getBusySlotItByUUID(uuid));
	if (m_it!=busySlots.end()) {  //the slot we're trying to get is currently busy
		return m_it->second;
	}
	else {
		SlotDeque::iterator d_it(getFreeSlotItByUUID(uuid));
		if (d_it!=freeSlots.end()) {    //the slot we're trying to get is currently free
			return *d_it;
		}
		else {  //possible error state: the slot that we're trying to get doesn't exist
			Print::printf(Print::ERROR, "Tried to get slot uuid: %d, but was not found in either container of free or busy slots.", uuid);
		}
	}
	return NULL;
}


void SlotList::free(int process, int thread)
{
    SlotMap::iterator m_it(getBusySlotIt(process, thread));
    if (m_it!=busySlots.end()) {
        Slot * freedSlot(m_it->second);
        busySlots.erase(m_it);
        freeSlots.push_back(freedSlot);
    }
    else {  //it is an error state if this branch is reached
        if (getFreeSlotIt(process, thread)!=freeSlots.end()) {
            Print::printf(Print::ERROR, "Tried to double free slot %d:%d.", process, thread);
        }
        else {
        Print::printf(Print::ERROR, "Tried to free non-existent slot %d:%d.", process, thread);
        }
    }
}

//for getBusySlotIt and getFreeSlotIt, it is the responsibility of the calling function to check whether the returned iterator is equal to container.end()
SlotMap::iterator SlotList::getBusySlotIt(int process, int thread)
{
	int keys[] = {process, thread};
    vector<int> slotKey(keys, keys+2);
    return busySlots.find(slotKey);
}

SlotDeque::iterator SlotList::getFreeSlotIt(int process, int thread)
{
	int keys[] = {process, thread};
	vector<int> slotKey(keys, keys+2);
    SlotDeque::iterator d_it=freeSlots.begin();
    for (; d_it!=freeSlots.end(); ++d_it) {
        if (slotKey==((*d_it)->getSlotKey())) {
            return d_it;
        }
    }
    return d_it;
}

//for getBusySlotItByUUID and getFreeSlotItByUUID, it is the responsibility of the calling function to check whether the returned iterator is equal to container.end()
SlotMap::iterator SlotList::getBusySlotItByUUID(uint32_t uuid)
{
	SlotMap::iterator m_it=busySlots.begin();
	for (; m_it!=busySlots.end(); ++m_it) {
		if (uuid==(m_it->second->getUUID())) {
			return m_it;
		}
	}
	return m_it;
}

SlotDeque::iterator SlotList::getFreeSlotItByUUID(uint32_t uuid)
{
    SlotDeque::iterator d_it=freeSlots.begin();
    for (; d_it!=freeSlots.end(); ++d_it) {
        if (uuid==((*d_it)->getUUID())) {
            return d_it;
        }
    }
    return d_it;
}
*/

}
}
