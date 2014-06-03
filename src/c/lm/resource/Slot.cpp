/*
 * Slotl.cpp
 *
 *  Created on: Jan 26, 2014
 *      Author: tel
 */

#include "lm/message/Communicator.h"
#include "lm/message/Message.h"
#include "lm/resource/Slot.h"
#include <vector>

namespace lm {
namespace resource {

Slot::Slot(int controller_process,
		   int controller_thread,
		   lm::message::Communicator * supervisorComm,
		   lm::io::SimulationParameters & simulationParameters,
		   bool hasReactionModel,
		   lm::io::ReactionModel & reactionModel,
		   bool hasDiffusionModel,
		   lm::io::DiffusionModel & diffusionModel): process(), thread(), supervisorComm(supervisorComm), status(FREE)
{
	// Send a message to the controller to start a work unit runner.
	lm::message::Message msg;
	lm::message::StartWorkUnitRunner* s = msg.add_start_work_unit_runner();
//	s->set_use_cpu_affinity(useCPUAffinity);
//	s->add_cpu(resources.cpuCores[i]);
//	if (resources.gpusDevices.size() > 0)
//		s->add_gpu(resources.gpusDevices[0]);
	s->set_solver(solverClassName);
	*s->mutable_simulation_parameters() = simulationParameters;
	if (hasReactionModel) *s->mutable_reaction_model() = reactionModel;
	if (hasDiffusionModel) *s->mutable_diffusion_model() = diffusionModel;
	supervisorComm->sendMessage(controller_process, controller_thread, &msg);

	// receive the handshake from the slave node signalling that the runner associated with this slot has been started
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

Slot::~Slot()
{
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
