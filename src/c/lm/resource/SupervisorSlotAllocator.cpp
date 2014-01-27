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

#include <iostream>
#include <pthread.h>
#include <sstream>
#include <vector>
#include "lm/Exceptions.h"
#include "lm/Math.h"
#include "lm/MPI.h"
#include "lm/resource/ResourceAllocator.h"
#include "lm/resource/SlotAllocatorSupervisor.h"
#include "lm/thread/Thread.h"
#include "lm/Types.h"
#include "lm/work/Result.pb.h"
#include "lm/work/Work.pb.h"

using lm::thread::PthreadException;
using std::vector;

namespace lm {
namespace resource {

SupervisorSlotAllocator::SupervisorSlotAllocator(int * maxSlotsTable): maxSlotsTable(maxSlotsTable)
{
    initialize();
}

void SupervisorSlotAllocator::initialize()
{
	int maxSlotsCounter = 0;
	for (int i=0; i<lm::MPI::worldSize; ++i)
	{
		for (int j=0; j<maxSlotsTable[i]; ++j)
		{
		vector<int> slotIds(2);
		slotIds.push_back(i);
		slotIds.push_back(j);
		slots[slotIds] = Slot(slotIds);
		++maxSlotsCounter;
		}
	}
	maxSlots = maxSlotsCounter;
	for(map<vector<int>, Slot>::iterator slot_it = slots.begin(); slot_it!=slots.end(); ++slot_it)
	{
		freeSlots.push_back(slot_it->second);
	}
}

int SupervisorSlotAllocator::getMaxSlots()
{
	return maxSlots;
}

int SupervisorSlotAllocator::getFreeSlotsSize()
{
	return freeSlots.size();
}

vector<int> SupervisorSlotAllocator::alloc()
{
	Slot * slot(freeSlots.back());
	freeSlots.pop_back();
	return slot->alloc();
}

void SupervisorSlotAllocator::free(vector<int> slotIds)
{
	Slot * slot = &(slots[slotIds]);
	slot->free();
	freeSlots.push_back(slot);
}

void SupervisorSlotAllocator::update(lm::work::Result result)
{
	vector<int> slotIds(2);
	slotIds.push_back(result.get_pid());
	slotIds.push_back(result.get_sid());
	free(slotIds);
}

}
}
