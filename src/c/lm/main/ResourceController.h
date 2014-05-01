/*/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
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

#ifndef LM_MAIN_RESOURCECONTROLLER
#define LM_MAIN_RESOURCECONTROLLER

#include <pthread.h>
#include <list>
#include <map>
#include <string>
#include "lm/Print.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/main/Main.h"
#include "lm/resource/DistributorSlotAllocator.h"
#include "lm/resource/ResourceAllocator.h"
#include "lm/main/ReplicateRunner.h"
#include "lm/me/MESolverFactory.h"
#include "lm/message/Communicator.h"
#include "lm/thread/Worker.h"
#include "lm/thread/Thread.h"
#include "lm/work/Work.pb.h"

namespace lm {
namespace main {

using lm::me::MESolverFactory;
using lm::main::ReplicateRunner;
using lm::resource::ResourceAllocator;
using lm::resource::DistributorSlotAllocator;
using std::list;
using std::map;
using std::string;
using std::vector;

class ResourceController : public lm::thread::Worker
{

public:
    ResourceController();
    virtual ~ResourceController();

    virtual void wake() throw(PthreadException);

protected:
    virtual int run();

protected:
    lm::message::Communicator communicator;

    /*
    DistributorSlotAllocator distributorSlotAllocator;

private:
    //variables relating to Worker behavior
    void * staticDataBuffer;
    lm::work::Work work;
    bool shouldCheckpoint;
    bool shouldAbort;

    //resourceAllocator and solverFactory are member variables since they're needed when starting replicates in run()
    ResourceAllocator & resourceAllocator;
    MESolverFactory & solverFactory;
    list<ReplicateRunner *> runningReplicates;
    */
};

}
}

#endif
