/*
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

#if defined(MACOSX)
#include <sys/sysctl.h>
#elif defined(LINUX)
#include <sys/sysinfo.h>
#endif

#include "lm/Exceptions.h"
#include "lm/ClassFactory.h"
#ifdef OPT_CUDA
#include "lm/Cuda.h"
#endif
#include "lm/MPI.h"
#include "lm/io/OutputWriter.h"
#include "lm/main/ResourceController.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Communicator.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ResourcesAvailable.pb.h"
#include "lm/message/StartOutputWriter.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/message/StartedWorkUnitRunner.pb.h"

namespace lm {
namespace main {

ResourceController::ResourceController()
:communicator(lm::MPI::worldRank, threadNumber)
{
}

ResourceController::~ResourceController()
{
}

void ResourceController::wake() throw(PthreadException)
{
//    MPI_EXCEPTION_CHECK(MPI_Send(NULL, 0, MPI_INT, lm::MPI::worldRank, lm::MPI::MSG_WAKE_REPLICATE_DISTRIBUTOR, MPI_COMM_WORLD));
}

/**
 * Gets the number of physical cpu cores on the system.
 */
std::vector<int> ResourceController::getPhysicalCPUCores()
{
    std::vector<int> cpus;

    // Get the number of processors.
    int numberCPUs=0;
    #if defined(MACOSX)
    uint physicalCpuCores;
    size_t  physicalCpuCoresSize=sizeof(physicalCpuCores);
    sysctlbyname("hw.activecpu",&physicalCpuCores,&physicalCpuCoresSize,NULL,0);
    numberCPUs=(int)physicalCpuCores;
    #elif defined(LINUX)
    numberCPUs=get_nprocs();
    #else
    #error "Unsupported architecture."
    #endif

    // Create a pid entry for each cpu.
    for (int i=0; i<numberCPUs; i++)
        cpus.push_back(i);

    return cpus;
}

std::vector<int> ResourceController::getPhysicalGPUs()
{
    std::vector<int> gpus;

    #ifdef OPT_CUDA
    for (int i=0; i<lm::CUDA::getNumberDevices(); i++)
        gpus.push_back(i);
    #endif

    return gpus;
}

int ResourceController::run()
{
    try
    {
        Print::printf(Print::INFO, "Resource controller %d:%d started.", lm::MPI::worldRank, threadNumber);

        // Register our info with the supervisor.
        lm::message::Message msg;
        msg.mutable_resources_available()->set_hostname(communicator.getHostname());
        msg.mutable_resources_available()->set_controller_process(lm::MPI::worldRank);
        msg.mutable_resources_available()->set_controller_thread(threadNumber);
        std::vector<int> cpus=getPhysicalCPUCores();
        for (std::vector<int>::iterator it = cpus.begin() ; it != cpus.end(); ++it)
            msg.mutable_resources_available()->add_cpu(*it);
        std::vector<int> gpus=getPhysicalGPUs();
        for (std::vector<int>::iterator it = gpus.begin() ; it != gpus.end(); ++it)
            msg.mutable_resources_available()->add_gpu(*it);
        communicator.sendMessage(lm::MPI::MASTER, lm::main::SimulationSupervisor::THREAD_ID, &msg);

        // Loop reading messages.
        lm::message::Message message;
        while (true)
        {
            // Read the next message.
            communicator.receiveMessage(&message);

            // Do something with the message.
            if (message.start_work_unit_runner_size() > 0)
            {
                for (int i=0; i<message.start_work_unit_runner_size(); i++)
                    startWorkUnitRunner(message.start_work_unit_runner(i));
            }
            else if (message.has_start_output_writer())
            {
                startOutputWriter(message.start_output_writer());
            }
            else
            {
                Print::printf(Print::ERROR, "Resource controller received an unknown message: {\n%s}",message.DebugString().c_str());
            }

            // Clear the message object so it can be used again.
            message.Clear();
        }

        Print::printf(Print::INFO, "Resource controller %d:%d finished.", lm::MPI::worldRank, threadNumber);
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
    return -1;
}

void ResourceController::startWorkUnitRunner(const lm::message::StartWorkUnitRunner& msg)
{
    // Start the work unit runner.
    WorkUnitRunner* runner = new WorkUnitRunner(properties);
    runners[runner->getThreadNumber()] = runner;
    runner->start();
}

void ResourceController::stopWorkUnitRunner(const lm::message::StopWorkUnitRunner & properties)
{
	runners[properties.thread()].stop();
	runners.remove(properties.thread());
}

void ResourceController::startOutputWriter(const lm::message::StartOutputWriter& msg)
{
    lm::io::OutputWriter* writer = static_cast<lm::io::OutputWriter*>(lm::ClassFactory::getInstance().allocateObjectOfClass("lm::io::OutputWriter",msg.output_writer_class()));
    if (msg.use_cpu_affinity()) writer->setAffinity(msg.cpu());
    writer->start();
}

}
}
