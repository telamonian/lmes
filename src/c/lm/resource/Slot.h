/*
 * Slot.h
 *
 *  Created on: Jan 26, 2014
 *      Author: tel
 */

#ifndef SLOT_H_
#define SLOT_H_

#include <string>
#include <vector>
#include "lm/message/Communicator.h"
#include "lm/Types.h"

using std::string;
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

	Slot(int controller_process, int controller_thread, uint32_t uuid, lm::message::Communicator * supervisorComm, lm::message::Message & msg);
	virtual ~Slot();

	virtual vector<int> alloc();
	virtual void free();

	virtual void startRemote(int controller_process, int controller_thread, lm::message::Message & msg);
	virtual void startedRemote(const lm::message::StartedWorkUnitRunner & msg);
	virtual void startWorkUnitRemote(lm::message::Message * msg, long long workUnitID);
	virtual void startedWorkUnitRemote(const lm::message::StartedWorkUnit & msg);
	virtual void stop();
	virtual void stopRemote();
	virtual void stoppedRemote();

	virtual void setStatus(slotStatus newStatus) {status = newStatus;}
	virtual slotStatus getStatus() {return status;}
	virtual vector<int> getSlotKey() {int keys[] = {process, thread}; vector<int> slotKey(keys, keys+2); return slotKey;}
	virtual uint32_t getUUID() {return uuid;}

	int process;
	int thread;
	int controller_process;	// in theory this should always be the same as process
	int controller_thread;

protected:
	lm::message::Communicator * supervisorComm;
	uint32_t uuid;
	slotStatus status;
};

}
}

#endif /* SLOT_H_ */
