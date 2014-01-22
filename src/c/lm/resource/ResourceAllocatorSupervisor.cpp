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
#include "lm/Exceptions.h"
#include "lm/Math.h"
#include "lm/MPI.h"
#include "lm/resource/ResourceAllocator.h"
#include "lm/resource/ResourceAllocatorSupervisor.h"
#include "lm/thread/Thread.h"
#include "lm/Types.h"
#include "lm/work/Result.pb.h"
#include "lm/work/Work.pb.h"

using lm::thread::PthreadException;

namespace lm {
namespace resource {

ResourceAllocatorSupervisor::ResourceAllocatorSupervisor(int * maxSlotsTable) throw(Exception,PthreadException)
{
    initialize(maxSlotsTable);
}

void ResourceAllocatorSupervisor::initialize(int * maxSlotsTable) throw(Exception,PthreadException)
{
	int maxSlotsCounter = 0;
	for (int i=0; i<lm::MPI::worldSize; ++i)
	{
		for (int j=0; j<maxSlotsTable[i]; ++j)
		{
		vector<int> slotId(2);
		slotId.push_back(i);
		slotId.push_back(j);
		slots[slotId] = Slot();
		++maxSlotsCounter;
		}
	}
	maxSlots = maxSlotsCounter;
}

int ResourceAllocatorSupervisor::getMaxSimultaneousSlots()
{
	return maxSlots;
}

void ResourceAllocatorSuperivsor::assignWorkUnit(lm::work::Work & workUnit) throw(Exception,PthreadException)
{
    for(map<vector<int>, Slot>::iterator it = slots.begin(); it!=slots.end(); ++it)
    {
    	if (it->second.status==FREE)
    	{
    		workUnit.set_pid(it->first[0]);
    		workUnit.set_sid(it->first[1]);
    		it->second.status = BUSY;
    		break;
    	}
    }
}

void ResourceAllocatorSupervisor::freeWorkUnit(lm::work::Result & resultUnit) throw(Exception,PthreadException)
{
	vector<int> slotId(2);
	slotId.push_back(resultUnit.get_pid());
	slotId.push_back(resultUnit.get_sid());
    slots[slotId] = FREE;
}


}
}
