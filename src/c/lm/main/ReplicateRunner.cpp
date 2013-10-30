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
#include "lm/rdme/RDMESolver.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using std::string;
using std::map;
using lm::me::MESolverFactory;

namespace lm {
namespace main {

ReplicateRunner::ReplicateRunner(int replicate, MESolverFactory solverFactory, map<string,string> * parameters, lm::io::ReactionModel * reactionModel, lm::io::DiffusionModel * diffusionModel, uint8_t * lattice, size_t latticeSize, uint8_t * latticeSites, size_t latticeSitesSize, ResourceAllocator::ComputeResources resources) throw(PthreadException)
:replicate(replicate),solverFactory(solverFactory),parameters(parameters),reactionModel(reactionModel),diffusionModel(diffusionModel),lattice(lattice),latticeSize(latticeSize),latticeSites(latticeSites),latticeSitesSize(latticeSitesSize),resources(resources),replicateFinished(false),replicateExitCode(-1)
{
}

ReplicateRunner::~ReplicateRunner() throw(PthreadException)
{
}

void ReplicateRunner::wake() throw(PthreadException)
{
}

int ReplicateRunner::run()
{
	// Set the processor affinity.
	//setAffinity(resources.cpuCores[0]);

	#if defined(OPT_CUDA)
	// Set the GPU affinity.
	if (resources.cudaDevices.size() > 0)
		lm::CUDA::setCurrentDevice(resources.cudaDevices[0]);
	#endif

	// Print a message detailing where this replciate is running.
	if (resources.cudaDevices.size() > 0)
		Print::printf(Print::INFO, "Running replicate %d in process %d on CPU core %d and GPU %d", replicate, resources.processNumber, resources.cpuCores[0], resources.cudaDevices[0]);
	else
		Print::printf(Print::INFO, "Running replicate %d in process %d on CPU core %d", replicate, resources.processNumber, resources.cpuCores[0]);


    PROF_SET_THREAD(resources.cpuCores[0]+PROF_THREAD_VARIABLE_START);
    PROF_BEGIN(PROF_REPLICATE_EXECUTE);

    MESolver * solver = NULL;
    int status = -1;
    try
    {
        // Run the simulation using the specified solver.
        solver = solverFactory.instantiate();
        solver->initialize(replicate, parameters, &resources);
        if (solver->needsReactionModel())
        {
            ((lm::cme::CMESolver *)solver)->setReactionModel(reactionModel);
        }
        if (solver->needsDiffusionModel())
        {
            ((lm::rdme::RDMESolver *)solver)->setDiffusionModel(diffusionModel, lattice, latticeSize, latticeSites, latticeSitesSize);
        }
        solver->generateTrajectory();
        status = 0;
    }
    catch (lm::Exception e)
    {
        Print::printf(Print::ERROR, "Exception during execution of replicate: %s.", e.what());
    }
    catch (std::exception& e)
    {
        Print::printf(Print::ERROR, "Std exception during execution of replicate: %s.", e.what());
    }
    catch (...)
    {
        Print::printf(Print::ERROR, "Unknown exception during execution of replicate.");
    }

    // Free any resources.
    if (solver != NULL) delete solver; solver = NULL;

    //// BEGIN CRITICAL SECTION: controlMutex
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&controlMutex));

    // Mark the replicate status as finished.
    replicateFinished = true;
    replicateExitCode = status;

    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&controlMutex));
    //// END CRITICAL SECTION: controlMutex

    PROF_END(PROF_REPLICATE_EXECUTE);

    return 0;
}

bool ReplicateRunner::hasReplicateFinished()
{
    bool ret;

    //// BEGIN CRITICAL SECTION: controlMutex
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&controlMutex));

    ret = replicateFinished;

    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&controlMutex));
    //// END CRITICAL SECTION: controlMutex

    return ret;
}

int ReplicateRunner::getReplicateExitCode()
{
    int ret;

    //// BEGIN CRITICAL SECTION: controlMutex
    PTHREAD_EXCEPTION_CHECK(pthread_mutex_lock(&controlMutex));

    ret = replicateExitCode;

    PTHREAD_EXCEPTION_CHECK(pthread_mutex_unlock(&controlMutex));
    //// END CRITICAL SECTION: controlMutex

    return ret;
}


}
}
