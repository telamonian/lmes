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
#include "lm/runner/Runner.h"

using std::vector;
using lm::runner::Runner;

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
	void update(lm::work::Work & work);
};

}
}

#endif /* DISTRIBUTORSLOT_H_ */
