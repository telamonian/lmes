/*
 * DistributorSlot.h
 *
 *  Created on: Jan 26, 2014
 *      Author: tel
 */

#ifndef DISTRIBUTORSLOT_H_
#define DISTRIBUTORSLOT_H_

#include <vector>
#include "lm/main/Runner.h"
#include "lm/resource/Slot.h"

using std::vector;
using lm::main::Runner;

namespace lm {
namespace resource {

class DistributorSlot: public Slot
{
public:
	DistributorSlot(vector<int> ids, ResourceAllocator::ComputeResources resource, Runner runner):
		Slot(ids), resource(resource), runner(runner) {}
	virtual ~DistributorSlot();
	virtual int getCore() {return resource.cpuCores[0];}

	ResourceAllocator::ComputeResources resource;
	Runner runner;
};

}
}

#endif /* DISTRIBUTORSLOT_H_ */
