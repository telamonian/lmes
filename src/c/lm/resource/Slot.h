/*
 * Slot.h
 *
 *  Created on: Jan 26, 2014
 *      Author: tel
 */

#ifndef SLOT_H_
#define SLOT_H_

#include <deque>
#include <vector>

using std::vector;
using std::deque;

typedef deque<lm::resource::Slot *> SlotDeque;

namespace lm {
namespace resource {

class Slot
{
public:
	enum slotStatus {FREE, BUSY, DEAD};

	Slot(vector<int> ids): process(ids[0]), thread(ids[1]), status(FREE), dequeIter(NULL) {}
	~Slot() {}
	void setStatus(slotStatus newStatus) {status = newStatus;}
    vector<int> alloc() {setStatus(BUSY); return getSlotIds();}
	void free() {setStatus(FREE);}
	vector<int> getSlotKey() {vector<int> slotKey; slotKey.push_back(process); slotKey.push_back(thread); return slotKey;}

	const int slot_id;
	const int process;
	const int thread;
	slotStatus status;

private:
	SlotDeque::iterator dequeIter;
};

}
}

#endif /* SLOT_H_ */
