/*
 * Slotl.cpp
 *
 *  Created on: Jan 26, 2014
 *      Author: tel
 */

#include <string>
#include <vector>
#include "lm/message/Communicator.h"
#include "lm/message/Message.pb.h"
#include "lm/Print.h"
#include "lm/slot/Slot.h"

using std::string;

namespace lm {
namespace slot {

Slot::Slot(int32_t id, lm::resource::ComputeResources resources)
:id(id), status(NOT_STARTED), resources(resources)
{
}

Slot::~Slot()
{
}

/*
void Slot::markWorkUnitRunnerRemoteStarted(const lm::message::StartedWorkUnitRunner & msg)
{
	// This is where the slot process and thread numbers are actually set
	process = msg.process();
	thread = msg.thread();
	Print::printf(Print::INFO, "Work unit runner for slot %d:%d started, %d simultaneous work unit runners.", msg.process(), msg.thread(), msg.simultaneous_work_units());
}

void startedWorkUnitRemote(const lm::message::StartedWorkUnit & msg)
{
}

void Slot::stop()
{
	// send message to associated resource controller to stop the associated runner
	stopRemote();
	// receive message from associated resource controller confirming that it has stopped the associated runner
	stoppedRemote();
}

void Slot::stopRemote()
{
	lm::message::Message msg;
	lm::message::StopWorkUnitRunner* s = msg.add_stop_work_unit_runner();
	s->set_process(process);
	s->set_thread(thread);
	supervisorComm->sendMessage(controller_process, controller_thread, &msg);
}

void Slot::stoppedRemote()
{
	lm::message::Message msg;
	supervisorComm->receiveMessage(&msg);
	Print::printf(Print::INFO, "Work unit runner for slot %d:%d stopped", msg.stopped_work_unit_runner().process(), msg.stopped_work_unit_runner().thread());
}

vector<int> Slot::alloc()
{
    setStatus(BUSY); return getSlotKey();
}

void Slot::free()
{
    setStatus(FREE);
}
*/

}
}
