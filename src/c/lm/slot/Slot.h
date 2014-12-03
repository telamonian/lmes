/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the Software), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimers.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimers in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the names of the Roberts Group, Johns Hopkins University,
 * nor the names of its contributors may be used to endorse or
 * promote products derived from this Software without specific prior written
 * permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS WITH THE SOFTWARE.
 *
 * Author(s): Elijah Roberts, Max Klein
 */
#ifndef SLOT_H_
#define SLOT_H_

#include "lm/Types.h"
#include "lm/message/Endpoint.h"
#include "lm/resource/ComputeResources.h"

using std::string;
using std::vector;

namespace lm {
namespace slot {

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
    enum Status {NOT_STARTED, FREE, BUSY, DEAD};

public:
    Slot(int id, lm::resource::ComputeResources resources);
	~Slot();

public:
    int id;
    Status status;
    lm::resource::ComputeResources resources;
    lm::message::Endpoint workUnitRunnerEndpoint;
};

}
}

#endif /* SLOT_H_ */
