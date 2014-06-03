/*
 * Slot.h
 *
 *  Created on: Jan 26, 2014
 *      Author: tel
 */

#ifndef SLOT_H_
#define SLOT_H_

#include "lm/message/Communicator.h"
#include <vector>

using std::vector;

namespace lm {
namespace resource {

/*
 * The Slot class is the glue between a Trajectory object on the master node and a set of computational resources on a slave node
 * There are four methods that a Slot on the master uses to communicate with the resource set on the slave
 * alloc - takes the data required to run a work unit from a Trajectory object, packages it into a RunWorkUnit message, and send it the appropriate WorkUnitRunner
 * free - takes the state and status information from a FinishedWorkUnit message and passes it on to the appropriate Trajectory
 * startRunner - tells the slave to start a WorkUnitRunner object
 * stopRunner - tells the slave to stop the appropriate WorkUnitRunner object
 */
class Slot
{
public:
	enum slotStatus {FREE, BUSY, DEAD};

	Slot(int controller_process,
		 int controller_thread,
	     lm::message::Communicator * supervisorComm,
	     lm::io::SimulationParameters & simulationParameters,
	     bool hasReactionModel,
	     lm::io::ReactionModel & reactionModel,
	     bool hasDiffusionModel,
	     lm::io::DiffusionModel & diffusionModel);
	virtual ~Slot();

	virtual vector<int> alloc();
	virtual void free();

	virtual bool startRemote();
	virtual bool registerRemote();
	virtual bool stopRemote();

	virtual void setStatus(slotStatus newStatus) {status = newStatus;}
	virtual slotStatus getStatus() {return status;}
	virtual vector<int> getSlotKey() {vector<int> slotKey; slotKey.push_back(process); slotKey.push_back(thread); return slotKey;}

	int process;
	int thread;

protected:
	lm::message::Communicator * supervisorComm;
	slotStatus status;
};

}
}

#endif /* SLOT_H_ */
