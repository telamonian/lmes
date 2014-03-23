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

#include <string>
#include <map>
#include <mpi.h>
#include <pthread.h>
#include "lm/Print.h"
#include "lm/cme/CMESolver.h"
#include "lm/cme/GillespieDSolver.h"
#include "lm/cme/HillSwitch.h"
#include "lm/cme/SelfRegulatingGeneSwitch.h"
#include "lm/cme/TwoStateExpression.h"
#include "lm/cme/TwoStateHillSwitch.h"
#include "lm/cme/TwoStateHillLoopSwitch.h"
#include "lm/cme/GillespieDSolver.h"
#if defined(OPT_CUDA)
#include "lm/Cuda.h"
#endif
#include "lm/main/Main.h"
#include "lm/main/ReplicateRunner.h"
#include "lm/me/MESolverFactory.h"
#include "lm/MPI.h"
#include "lm/rdme/RDMESolver.h"
#include "lm/runner/Runner.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lm/work/Work.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using std::string;
using std::map;
using lm::me::MESolverFactory;

namespace lm {
namespace runner {

Runner::Runner(MESolverFactory solverFactory, ResourceAllocator::ComputeResources resources) throw(PthreadException):
solverFactory(solverFactory),
resources(resources)
{
	pthread_cond_init(&runnerCv, NULL);
}

Runner::~Runner() throw(PthreadException)
{
}

void Runner::lock_mutex()
{
	//// BEGIN CRITICAL SECTION: controlMutex
	PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&controlMutex));
	//// END CRITICAL SECTION: controlMutex
}

void Runner::unlock_mutex()
{
	//// BEGIN CRITICAL SECTION: controlMutex
	PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&controlMutex));
	//// END CRITICAL SECTION: controlMutex
}

void Runner::cond_signal()
{
	PTHREAD_EXCEPTION_CHECK(pthread_cond_signal(&runnerCv));
}

void Runner::update(lm::work::Work & work)
{

}

void Runner::go()
{

}

}
}
