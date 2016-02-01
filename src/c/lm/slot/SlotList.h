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

#ifndef LM_RESOURCE_SLOTALLOCATOR_H_
#define LM_RESOURCE_SLOTALLOCATOR_H_

#include <map>
#include <string>
#include <vector>
#include "lm/Types.h"
#include "lm/input/Input.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.pb.h"
#include "lm/message/Communicator.h"
#include "lm/message/Message.pb.h"
#include "lm/resource/ComputeResources.h"
#include "lm/rng/XORShift.h"
#include "lm/slot/Slot.h"
#include "lm/thread/Thread.h"

using lm::resource::ComputeResources;
using lm::slot::Slot;
using lm::thread::PthreadException;
using std::map;
using std::string;
using std::vector;

namespace lm {
namespace slot {

class SlotList
{
public:
    SlotList(lm::message::Communicator * communicator);
    ~SlotList();

    //create slot methods
    void createAllSlots(map<int,ComputeResources> & allResources, double cpusPerSlot, double gpusPerSlot, bool useCPUAffinity, string solver, lm::input::Input* input);
    int createProcessSlots(int startingSlotId, int process, ComputeResources resources, double cpusPerSlot, double gpusPerSlot, bool useCPUAffinity, string solver, lm::input::Input* input);
    void createSlot(int slotId, ComputeResources resources, bool useCPUAffinity, lm::message::Message* msg, string solver, lm::input::Input* input);

    int getNumberSlots() {return slots.size();}
    void markSlotStarted(const lm::message::StartedWorkUnitRunner & msg);
    bool hasUnstartedSlots();
    bool hasFreeSlots();
    bool hasBusySlots();
    void runWorkUnit(lm::message::Message* runWorkUnitMsg);
    void workUnitFinished(const lm::message::FinishedWorkUnit& msg);



private:
    vector<Slot> slots;
    map<int64_t,int> workUnitToSlotMap;
    lm::message::Communicator* communicator;
};

}
}

#endif
