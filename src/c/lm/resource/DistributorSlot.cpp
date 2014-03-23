/*
 * ResourceLinkedSlot.cpp
 *
 *  Created on: Jan 26, 2014
 *      Author: tel
 */

#include "lm/resource/DistributorSlot.h"

namespace lm {
namespace resource {

void DistributorSlot::update(lm::work::Work & work)
{
	//// CRITICAL SECTION
	runner.lock_mutex();
	runner.update(work);
	runner.cond_signal();
	runner.unlock_mutex();
	//// CRITICAL SECTION
}

}
}
