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
#include "lm/resource/Slot.h"

using std::string;

namespace lm {
namespace resource {

Slot::Slot(int controller_process, int controller_thread, uint32_t uuid, lm::message::Communicator * supervisorComm, lm::message::Message & msg)
		   :process(), thread(), controller_process(controller_process), controller_thread(controller_thread), uuid(uuid), supervisorComm(supervisorComm), status(FREE)
{
	// Send a message to the controller to start a work unit runner.
	startRemote(controller_process, controller_thread, msg);
	// receive the handshake from the slave node signalling that the runner associated with this slot has been started
	startedRemote();
}

Slot::~Slot()
{
}

void Slot::startRemote(int controller_process, int controller_thread, lm::message::Message & msg)
{
	lm::message::StartWorkUnitRunner* s = msg.mutable_start_work_unit_runner(0);
	s->set_uuid(uuid);
	supervisorComm->sendMessage(controller_process, controller_thread, &msg);
}

void Slot::startedRemote()
{
	lm::message::Message msg;
	supervisorComm->receiveMessage(&msg);
	if (msg.has_started_work_unit_runner())
	{
		process = msg.started_work_unit_runner().process();
		thread = msg.started_work_unit_runner().thread();
	}
	else
	{
		Print::printf(Print::ERROR, "Supervisor received an unknown message during slot startup handshake: {\n%s}",msg.DebugString().c_str());
	}
	Print::printf(Print::INFO, "Work unit runner for slot %d:%d started, %d simultaneous work unit runners.", msg.started_work_unit_runner().process(), msg.started_work_unit_runner().thread(), msg.started_work_unit_runner().simultaneous_work_units());
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


}
}
