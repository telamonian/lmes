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
 * Author(s): Elijah Roberts, Max Klein
 */

#include <string>
#include <map>
#include <pthread.h>
#include <vector>

#include "hrtime.h"
#include "lm/ClassFactory.h"
#include "lm/cme/CMESolver.h"
#include "lm/Print.h"
#if defined(OPT_CUDA)
#include "lm/Cuda.h"
#endif
#include "lm/main/SimulationSupervisor.h"
#include "lm/main/WorkUnitRunner.h"
#include "lm/me/MESolver.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/message/WorkUnit.pb.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using std::map;
using std::string;
using std::vector;
using lm::message::Communicator;
using lm::message::Endpoint;

namespace lm {
namespace main {

WorkUnitRunner::WorkUnitRunner(const lm::message::StartWorkUnitRunner& msg)
:id(-1),communicator(NULL),properties(msg),solver(NULL)
{
    id = msg.work_unit_runner_id();

    // Create the communicator.
    communicator = lm::message::Communicator::createObjectOfDefaultSubclass(false);
}

WorkUnitRunner::~WorkUnitRunner()
{
    if (solver != NULL) delete solver; solver = NULL;
    if (communicator != NULL) delete communicator; communicator = NULL;
}

void WorkUnitRunner::wake() throw(PthreadException)
{
    lm::message::Message msg;
    msg.mutable_ping_target()->set_id(0);
    communicator->sendMessage(communicator->getSourceAddress(), &msg);
}

int WorkUnitRunner::run()
{
    try
    {
        Print::printf(Print::INFO, "Work Unit runner %s started with %d cpu cores (affinity=%d) and %d gpus.", Communicator::printableAddress(communicator->getSourceAddress()).c_str(), properties.cpu_size(), properties.use_cpu_affinity(), properties.gpu_size());

        // Set the processor affinity.
        if (properties.use_cpu_affinity() && properties.cpu_size() > 0)
        {
            setAffinity(properties.cpu(0));
        }

        // Set the GPU affinity.
        #if defined(OPT_CUDA)
        if (properties.gpu_size() > 0)
        {
            Print::printf(Print::INFO, "Work Unit runner %s using gpu device %d.", Communicator::printableAddress(communicator->getSourceAddress()).c_str(), properties.gpu(0));
            lm::CUDA::setCurrentDevice(properties.gpu(0));
        }
        #endif

        if (lm::ClassFactory::getInstance().getBaseClass(properties.solver()) == "lm::me::MESolver")
            solver = createMESolver();
        else if (lm::ClassFactory::getInstance().getBaseClass(properties.solver()) == "lm::pde::DiffusionPDESolver")
            solver = createDiffusionPDESolver();
        else
            throw Exception("Unknown solver type:", properties.solver().c_str(), lm::ClassFactory::getInstance().getBaseClass(properties.solver()).c_str());

        // Set the solver resources.
        vector<int> cpus;
        vector<int> gpus;
        for (int i=0; i<properties.cpu_size(); i++) cpus.push_back(properties.cpu(i));
        for (int i=0; i<properties.gpu_size(); i++) gpus.push_back(properties.gpu(i));
        solver->setComputeResources(cpus, gpus);

        // Tell the supervisor the runner has started.
        lm::message::Message msgp;
        lm::message::StartedWorkUnitRunner* msg = msgp.mutable_started_work_unit_runner();
        msg->set_work_unit_runner_id(id);
        msg->mutable_address()->CopyFrom(communicator->getSourceAddress());
        msg->set_simultaneous_work_units(solver->getSimultaneousTrajectories());
        communicator->sendMessage(communicator->getSupervisorAddress(), &msgp);

        // Loop reading messages.
        lm::message::Message message;
        while (true)
        {
            // Read the next message.
            communicator->receiveMessage(&message);

            // Do something with the message.
            if (message.has_run_work_unit())
            {
                runWorkUnits(message.run_work_unit());
            }
            else if (message.has_ping_target())
            {
                // If we are done running, stop the loop.
                if (!running) break;
            }
            else
            {
                Print::printf(Print::ERROR, "Work unit runner received an unknown message: {\n%s}",message.DebugString().c_str());
            }

            // Clear the message object so it can be used again.
            message.Clear();
        }

        //Delete the solver.
        if (solver != NULL) delete solver; solver = NULL;

        Print::printf(Print::INFO, "Work unit runner %s finished.", Communicator::printableAddress(communicator->getSourceAddress()).c_str());
        return 0;
    }
    catch (lm::Exception e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (std::exception& e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (...)
    {
        Print::printf(Print::FATAL, "Unknown Exception during execution (%s:%d)", __FILE__, __LINE__);
    }
    exit(-1);
    return -1;
}

Solver* WorkUnitRunner::createMESolver()
{
    // Instantiate the solver.
    lm::me::MESolver* solver = static_cast<lm::me::MESolver*>(lm::ClassFactory::getInstance().allocateObjectOfClass("lm::me::MESolver",properties.solver()));

    // Set the model for the solver.
    if (solver->needsReactionModel())
    {
        if (properties.has_reaction_model())
            solver->setReactionModel(properties.reaction_model());
        else
            throw Exception("Work Unit runner terminating, solver requires a reaction model but none was specified", properties.solver().c_str());
    }
    if (solver->needsDiffusionModel())
    {
        if (properties.has_diffusion_model())
            solver->setDiffusionModel(properties.diffusion_model());
        else
            throw Exception("Work Unit runner terminating, solver requires a diffusion model but none was specified", properties.solver().c_str());
    }

    // Set the order parameters for the solver
    if (properties.has_order_parameters())
    {
        solver->setOrderParameters(properties.order_parameters());
    }

    return solver;
}

Solver* WorkUnitRunner::createDiffusionPDESolver()
{
    // Instantiate the solver.
    lm::pde::DiffusionPDESolver* solver = static_cast<lm::pde::DiffusionPDESolver*>(lm::ClassFactory::getInstance().allocateObjectOfClass("lm::pde::DiffusionPDESolver", properties.solver()));

    if (properties.has_microenv_model())
        solver->setMicroenvironmentModel(properties.microenv_model());
    else
        throw Exception("Work Unit runner terminating, solver requires a microenvironment model but none was specified", properties.solver().c_str());

    return solver;
}

void WorkUnitRunner::runWorkUnits(const lm::message::RunWorkUnit& rwuMsg)
{
    PROF_BEGIN(PROF_WORK_UNIT_RUN);

    // Tell the supervisor the work unit is started.
    lm::message::Message handshakeMsg;
    lm::message::StartedWorkUnit* swuMsg = handshakeMsg.mutable_started_work_unit();
    swuMsg->set_work_unit_id(rwuMsg.work_unit_id());
    communicator->sendMessage(communicator->getSupervisorAddress(), &handshakeMsg);

    // Set the limits.
    if (rwuMsg.has_trajectory_limits())
        solver->setLimits(rwuMsg.trajectory_limits());

    // Set the output options.
    if (rwuMsg.has_output_options())
        solver->setOutputOptions(rwuMsg.output_options());

    // Create the finished work unit message.
    lm::message::Message finalStateMsg;
    lm::message::FinishedWorkUnit* fwuMsg = finalStateMsg.mutable_finished_work_unit();
    fwuMsg->set_work_unit_id(rwuMsg.work_unit_id());

    // Create the output message.
    bool hasOutput = false;
    lm::message::Message outputParent;
    lm::message::ProcessWorkUnitOutput* output = outputParent.mutable_process_work_unit_output();
    output->set_work_unit_id(rwuMsg.work_unit_id());
    google::protobuf::RepeatedPtrField<lm::message::WorkUnitOutput>* outputParts = output->mutable_part_output();

    uint64_t totalSteps=0;
    hrtime totalTime=0;
    for (int i=0; i<rwuMsg.part_size(); i+=solver->getSimultaneousTrajectories())
    {
        // Reset the solver.
        solver->reset();

        // Configure the solver state for each simultaneous trajectory.
        for (int j=0; j<(int)solver->getSimultaneousTrajectories() && (i+j)<rwuMsg.part_size(); j++)
        {
            solver->setState(rwuMsg.part(i+j).initial_state(), j);
        }

        PROF_BEGIN(PROF_WORK_UNIT_RUN_PART);

        // Run the work unit.
        hrtime t1=getHrTime();
        totalSteps += solver->generateTrajectory(rwuMsg.max_steps());
        totalTime += getHrTime()-t1;

        PROF_END(PROF_WORK_UNIT_RUN_PART);

        PROF_BEGIN(PROF_WORK_UNIT_SAVE_PART);

        // Save the status and the state.
        for (int j=0; j<(int)solver->getSimultaneousTrajectories() && (i+j)<rwuMsg.part_size(); j++)
        {
            lm::message::WorkUnitStatus* status = fwuMsg->add_part_status();
            status->set_status(solver->getStatus(j));
            lm::io::TrajectoryState* finalState = status->mutable_final_state();
            finalState->set_trajectory_id(rwuMsg.part(i+j).initial_state().trajectory_id());
            finalState->set_trajectory_started(true);
            solver->getState(finalState, j);
        }

        // Save the output.
        for (int j=0; j<(int)solver->getSimultaneousTrajectories() && (i+j)<rwuMsg.part_size(); j++)
        {
            lm::message::WorkUnitOutput* output = solver->getOutput(j);
            if (output->has_output())
            {
                output->set_condense_output(rwuMsg.output_options().condense_output());
                output->set_record_name_prefix(rwuMsg.output_options().record_name_prefix());

                outputParts->AddAllocated(output);
                hasOutput = true;
            }
            else
            {
                delete output;
            }
        }

        PROF_END(PROF_WORK_UNIT_SAVE_PART);
    }

    // Send the output.
    if (hasOutput) communicator->sendMessage(rwuMsg.output_address(), &outputParent);

    // Tell the supervisor the work unit has finished.
    fwuMsg->set_steps(totalSteps);
    fwuMsg->set_run_time(convertHrToSeconds(totalTime));
    communicator->sendMessage(communicator->getSupervisorAddress(), &finalStateMsg);

    PROF_END(PROF_WORK_UNIT_RUN);
}

}
}
