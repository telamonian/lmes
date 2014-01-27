/*
 * SupervisorSlot.h
 *
 *  Created on: Jan 26, 2014
 *      Author: tel
 */

#ifndef SUPERVISORSLOT_H_
#define SUPERVISORSLOT_H_

#include "lm/resource/Slot.h"

using std::vector;

namespace lm {
namespace resource {

class SupervisorSlot: public Slot
{
public:
	SupervisorSlot(vector<int> ids): Slot(ids) {}
	~SupervisorSlot();
};

}
}

#endif /* SUPERVISORSLOT_H_ */
