/*
 * Slot.h
 *
 *  Created on: Jan 26, 2014
 *      Author: tel
 */

#ifndef SLOT_H_
#define SLOT_H_

#include <vector>

using std::vector;

namespace lm {
namespace resource {

class Slot
{
public:
	static enum slotStatus {FREE, BUSY, DEAD};

	Slot(vector<int> ids): pid(ids[0]), sid(ids[1]), status(FREE) {}
	~Slot();
	void setStatus(slotStatus newStatus) {status = newStatus;}
	vector<int> alloc() {setStatus(BUSY); return getSlotIds();}
	void free() {setStatus(FREE);}
	vector<int> getSlotIds() {vector<int> slotIds; slotIds.push_back(pid); slotIds.push_back(sid); return slotIds;}

	const int pid;
	const int sid;
	slotStatus status;
};

}
}

#endif /* SLOT_H_ */
