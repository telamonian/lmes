/*
 * DistributorSlot.h
 *
 *  Created on: Jan 26, 2014
 *      Author: tel
 */

#ifndef DISTRIBUTORSLOT_H_
#define DISTRIBUTORSLOT_H_

#include <vector>
#include "lm/resource/Slot.h"
#include "lm/runner/Runner.h"

using std::vector;
using lm::runner::Runner;

namespace lm {
namespace resource {

class DistributorSlot: public Slot
{
public:
	DistributorSlot(vector<int> ids, ResourceAllocator::ComputeResources resource, Runner * runner):
		Slot(ids), resource(resource), runner(runner) {}
	virtual ~DistributorSlot() {delete runner;}
	virtual int getCore() {return resource.cpuCores[0];}
	virtual void alloc(lm::work::Work & work);

	ResourceAllocator::ComputeResources resource;
	Runner * runner;
};

}
}

#endif /* DISTRIBUTORSLOT_H_ */
