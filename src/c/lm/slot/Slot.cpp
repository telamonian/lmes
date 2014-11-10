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

// Slot is responsible for StartWorkUnitRunner messages
Slot::Slot(int controller_process, int controller_thread, uint32_t uuid, lm::message::Communicator * supervisorComm, lm::message::Message & msg)
		   :process(), thread(), controller_process(controller_process), controller_thread(controller_thread), output_process(0), output_thread(3), uuid(uuid), supervisorComm(supervisorComm), status(FREE) // TODO: still ned to deshitify the whole output workers thing
{
	// Send a message to the controller to start a work unit runner.
	workUnitRunnerRemoteStart(controller_process, controller_thread, msg);
}

Slot::~Slot()
{
}

void Slot::workUnitRunnerRemoteStart(int controller_process, int controller_thread, lm::message::Message & msg)
{
	lm::message::StartWorkUnitRunner* s = msg.mutable_start_work_unit_runner(0);
	s->set_uuid(uuid);
	supervisorComm->sendMessage(controller_process, controller_thread, &msg);
}

void Slot::workUnitRemoteStart(lm::message::Message* msg, long long workUnitID)
{
    // Send the start work unit message.
    lm::message::RunWorkUnit& run = *(msg->mutable_run_work_unit());
    run.set_work_unit_id(workUnitID);

    Print::printf(Print::DEBUG, "Sending message to start work unit %d with trajectory %d on slot %d:%d.", workUnitID, msg->run_work_unit().initial_state().trajectory_id(), getSlotKey()[0], getSlotKey()[1]);
    supervisorComm->sendMessage(getSlotKey()[0], getSlotKey()[1], msg);
}

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

}
}
