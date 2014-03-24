/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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
 * Author(s): Elijah Roberts
 */

#ifndef LM_RUNNER_RUNNER_H_
#define LM_RUNNER_RUNNER_H_

#include <map>
#include <string>
#include "lm/resource/ResourceAllocator.h"
#include "lm/me/MESolverFactory.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/runner/Runner.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lm/work/Result.pb.h"
#include "lm/work/Work.pb.h"

using lm::me::MESolverFactory;
using lm::resource::ResourceAllocator;
using lm::thread::PthreadException;
using lm::thread::Worker;
using std::map;
using std::string;

namespace lm {
namespace runner {

class Runner : public Worker
{
public:
    Runner(MESolverFactory solverFactory, ResourceAllocator::ComputeResources resources) throw(PthreadException);
    virtual ~Runner() throw(PthreadException);
    virtual void wake() throw(PthreadException);
    virtual int run() = 0;

    pthread_cond_t runnerCv;

    virtual void alloc(lm::work::Work & work);
    virtual void go() = 0;
    virtual void emit_result();

    virtual void lock_mutex();
    virtual void unlock_mutex();

    virtual void cond_signal();

protected:
    lm::work::Result result;
    MESolverFactory solverFactory;
    ResourceAllocator::ComputeResources resources;
    volatile bool replicateFinished;
    volatile int replicateExitCode;

private:
    void * staticDataBuffer;
};

}
}

#endif
