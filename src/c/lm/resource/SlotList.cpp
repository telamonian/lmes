/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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
 * Author(s): Elijah Roberts
 */

#include <climits>
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <string>
#include <vector>
#include "lm/Exceptions.h"
#include "lm/main/Main.h"
#include "lm/Math.h"
#include "lm/message/Communicator.h"
#include "lm/MPI.h"
#include "lm/Print.h"
#include "lm/resource/Slot.h"
#include "lm/resource/SlotList.h"
#include "lm/thread/Thread.h"
#include "lm/Types.h"

using lm::thread::PthreadException;
using lm::resource::Slot;
using std::string;
using std::vector;

namespace lm {
namespace resource {

SlotList::SlotList(lm::message::Communicator * supervisorComm): busySlots(), freeSlots(), xorShift(0,0), supervisorComm(supervisorComm), slotTemplateMsg() //the rng object xorShift uses the current time as a seed when given 0,0 as constructor arguments
{
}

SlotList::~SlotList()
{
    for (SlotMap::iterator m_it=busySlots.begin(); m_it!=busySlots.end(); ++m_it) delete m_it->second;
    for (SlotDeque::iterator d_it=freeSlots.begin(); d_it!=freeSlots.end(); ++d_it) delete *d_it;
}

void SlotList::addSlots(map<int,ResourceMap::ComputeResources> & allResources)
{
	for (map<int,ResourceMap::ComputeResources>::iterator it=allResources.begin(); it != allResources.end(); it++)
	{
		addSlots(it->second, cpuCoresPerReplicate, gpuDevicesPerReplicate);	//for now we'll just use the command line arguments for the cpu/gpu per slot arguments
	}
}

void SlotList::addSlots(ResourceMap::ComputeResources & resources, float cpusPerSlot, float gpusPerSlot)
{
	int slotsToStart;
	if (cpusPerSlot==0 && gpusPerSlot==0)
	{
		slotsToStart = 0;
		Print::printf(Print::INFO, "0 cpus and 0 gpus requested per trajectory. No compute resources were requested, so no slots will be started.");
	}
	else
	{
		int cpuSlots = cpusPerSlot > 0 ? resources.cpuCores.size()/cpusPerSlot: INT_MAX;
        int gpuSlots = gpusPerSlot > 0 ? resources.gpuDevices.size()/gpusPerSlot: INT_MAX;
		slotsToStart = cpuSlots > gpuSlots ? gpuSlots : cpuSlots;
	}
	Print::printf(Print::INFO, "Attempting to start %d slots with %.3f cpus and %.3f gpus each on process %d.", slotsToStart, cpusPerSlot, gpusPerSlot, resources.controller_process);
	for (int i=0; i<slotsToStart; i++)
	{
		addSlot(resources.controller_process, resources.controller_thread);
	}
	Print::printf(Print::INFO, "Start work unit runner(s) %d:%d start msg sent.", resources.controller_process, resources.controller_thread);
}

void SlotList::addSlot(int controller_process, int controller_thread)
{
	// Create temporary slot ID for use during slot registration process
	uint32_t uuid(xorShift.getRandom());
    Slot * addedSlot = new Slot(controller_process, controller_thread, uuid, supervisorComm, slotTemplateMsg);
    // the following static_cast<int> is used to get around the disallowment of 'narrowing' conversions in c++11
    int keys[] = {-1, static_cast<int>(uuid)};
    vector<int> slotKey(keys, keys+2);
    busySlots[slotKey] = addedSlot;
}

bool SlotList::workUnitRunnerStarted(const lm::message::StartedWorkUnitRunner & msg)
{
	// the following static_cast<int> is used to get around the disallowment of 'narrowing' conversions in c++11
	int keys[] = {-1, static_cast<int>(msg.uuid())};
	vector<int> slotKey(keys, keys+2);
	SlotMap::iterator m_it(busySlots.find(slotKey));
	if (m_it!=busySlots.end())
	{
		m_it->second->workUnitRunnerRemoteStarted(msg);
		freeSlots.push_front(m_it->second);
		busySlots.erase(m_it);
	}
	else
	{
		Print::printf(Print::ERROR, "Tried to register started slot %d:%d with uuid %d, but this slot does not exist on the master", msg.process(), msg.thread(), msg.uuid());
	}
	// If the StartedWorkUnitRunner messages have come back for all of the slots, notify the supervisor that the slots are ready to go
	return busySlots.empty();
}

void SlotList::delSlot(int process, int thread)
{
    SlotMap::iterator m_it(getBusySlotIt(process, thread));
    if (m_it!=busySlots.end()) {  //the slot we're trying to delete is currently busy
        //TODO: implement behavior for what is presumably the error state of trying to delete a currently busy slot. For now, pretend like it's fine and just delete the slot
        delete m_it->second;
        busySlots.erase(m_it);
    }
    else {
        SlotDeque::iterator d_it(getFreeSlotIt(process, thread));
        if (d_it!=freeSlots.end()) {    //the slot we're trying to delete is currently free
            delete *d_it;
            freeSlots.erase(d_it);
        }
        else {  //error state: we have tried to delete a slot that doesn't exist
            Print::printf(Print::ERROR, "Tried to delete slot %d:%d, but was not found in container of free or busy slots.", process, thread);
        }
    }
}

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

Slot * SlotList::alloc()
{
	if (freeSlots.size() > 0)
	{
		Slot * freeSlot(freeSlots.front());
		freeSlots.pop_front();
		busySlots[freeSlot->getSlotKey()] = freeSlot;
		return freeSlot;
	}
	else
	{
		return NULL;
	}
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

}
}
