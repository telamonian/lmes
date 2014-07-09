/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * Copyright 2012 Roberts Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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
 * 			  Max Klein
 */

#ifndef LM_RESOURCE_SLOTALLOCATOR_H_
#define LM_RESOURCE_SLOTALLOCATOR_H_

#include <deque>
#include <map>
#include <string>
#include <vector>
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.pb.h"
#include "lm/message/Communicator.h"
#include "lm/message/Message.pb.h"
#include "lm/resource/ResourceMap.h"
#include "lm/resource/Slot.h"
#include "lm/rng/XORShift.h"
#include "lm/thread/Thread.h"
#include "lm/Types.h"

using std::deque;
using std::map;
using std::string;
using std::vector;
using lm::resource::Slot;
using lm::thread::PthreadException;

typedef map<vector<int>, Slot *> SlotMap;
typedef deque<Slot *> SlotDeque;

namespace lm {
namespace resource {

/*
 * follows similar reasoning to the Resource Manager design pattern (http://www.eventhelix.com/realtimemantra/PatternCatalog/resource_manager_pattern.htm) with fewer typos
 * slots are allocated using new, and their deallocation is the responsibility of SlotList's destructor
 * pointers to the free slots are kept in a member deque named freeSlots
 * pointers to the busy slots are kept in a member map named slots
 * creation, deletion, and getter methods are implemented such that no objects external to a SlotList need to care about SlotList's internal structure (maps, deques, etc.)
 */
class SlotList
{
public:
    SlotList(lm::message::Communicator * supervisorComm);
    virtual ~SlotList();

    //create slot methods
    virtual void addSlots(map<int,ResourceMap::ComputeResources> & allResources);
    virtual void addSlots(ResourceMap::ComputeResources & resources, float cpusPerSlot=1.0, float gpusPerSlot=1.0);
    virtual void addSlot(int controller_process, int controller_thread);

    //delete slot methods
    virtual void delSlot(int process, int thread);

    //getter methods
    virtual Slot * getSlot(int process, int thread);
    virtual Slot * getSlotByUUID(uint32_t uuid);
    virtual int getSlotsSize() {return getBusySlotsSize() + getFreeSlotsSize();}
    virtual int getBusySlotsSize() {return busySlots.size();}
    virtual int getFreeSlotsSize() {return freeSlots.size();}

    //allocate and free methods
    virtual Slot * alloc();
    virtual void workUnitFinished(const lm::message::FinishedWorkUnit& msg) {free(msg.process(), msg.thread());}
    virtual void free(int process, int thread);

    //dealing with the internal Message methods
    virtual lm::message::StartWorkUnitRunner * addStartSlotMsg() {return slotTemplateMsg.add_start_work_unit_runner();}
    virtual bool workUnitRunnerStarted(const lm::message::StartedWorkUnitRunner & msg);

private:
    virtual SlotMap::iterator getBusySlotIt(int process, int thread);
    virtual SlotDeque::iterator getFreeSlotIt(int process, int thread);
    virtual SlotMap::iterator getBusySlotItByUUID(uint32_t uuid);
    virtual SlotDeque::iterator getFreeSlotItByUUID(uint32_t uuid);

	lm::message::Communicator * supervisorComm;
	lm::message::Message slotTemplateMsg;
    SlotMap busySlots;
    SlotDeque freeSlots;
    lm::rng::XORShift xorShift;
};

}
}

#endif
