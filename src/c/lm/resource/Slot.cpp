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

void Slot::startedRemote(const lm::message::StartedWorkUnitRunner & msg)
{
	process = msg.process();
	thread = msg.thread();
	Print::printf(Print::INFO, "Work unit runner for slot %d:%d started, %d simultaneous work unit runners.", msg.process(), msg.thread(), msg.simultaneous_work_units());
}

void Slot::startWorkUnit(lm::message::Message & msg)
{
	// Send the start work unit message.
	lm::message::Message msg;
	lm::message::RunWorkUnit& run = *msg.mutable_run_work_unit();
	run.set_work_unit_id(workUnitCount++);
	run.set_supervisor_process(communicator.getSourceProcess());
	run.set_supervisor_thread(communicator.getSourceThread());
	run.set_output_process(outputWriterProcess);
	run.set_output_thread(outputWriterThread);
	run.set_max_steps(100);
	*run.mutable_initial_state() = trajectories->getTrajectoryState(nextTrajectory);
	*run.mutable_limits() = limits;
	Print::printf(Print::INFO, "Sending message to start work unit %d with trajectory %d on slot %d:%d.", run.work_unit_id(), nextTrajectory, workSlot->getSlotKey()[0], workSlot->getSlotKey()[1]);
	communicator.sendMessage(workSlot->getSlotKey()[0], workSlot->getSlotKey()[1], &msg);
	trajectories->updateTrajectoryStatus(nextTrajectory, FFluxTrajectoryList::RUNNING);
}

void Slot::startedWorkUnit()
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
