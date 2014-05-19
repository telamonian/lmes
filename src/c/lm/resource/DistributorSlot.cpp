/*
 * ResourceLinkedSlot.cpp
 *
 *  Created on: Jan 26, 2014
 *      Author: tel
 */

#include "lm/resource/DistributorSlot.h"

namespace lm {
namespace resource {

void DistributorSlot::alloc(lm::work::Work & work)
{
	runner->lock_mutex();
	runner->alloc(work);
	runner->cond_signal();
	runner->unlock_mutex();
}

}
}
