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
#include <string>

#include "lm/Exceptions.h"
#include "lm/MPI.h"
#include "lm/Print.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/hdf5/HDF5.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/main/Main.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Communicator.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ResourcesAvailable.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/message/StartedWorkUnitRunner.pb.h"
#include "lm/resource/ComputeResources.h"
#include "lm/resource/ResourceMap.h"
#include "lm/slot/SlotList.h"

using lm::resource::ComputeResources;
using lm::resource::ResourceMap;
using std::string;

namespace lm {
namespace main {

SimulationSupervisor::SimulationSupervisor()
    :communicator(lm::MPI::worldRank,THREAD_ID),resourceMap(NULL),simulationInputFilename(""),simulationOutputFilename(""),outputWriterClassName(""),hasOutputWriterStarted(false),outputWriterProcess(-1),outputWriterThread(-1),solverClassName(""),useCPUAffinity(false),input(NULL),hasReactionModel(false),hasDiffusionModel(false),hasOrderParameters(false),hasTilings(false),tilings(),trajectories(NULL),slots(&communicator),haveAllWorkUnitRunnersStarted(false),workUnitCount(0)
{
}

SimulationSupervisor::~SimulationSupervisor()
{
    if (input != NULL) delete input; input = NULL;
    if (trajectories != NULL) delete trajectories; trajectories = NULL; // since Supervisors call new to allocate their TrajectoryLists, this needs to be here
}

void SimulationSupervisor::wake() throw(lm::thread::PthreadException)
{
    lm::message::Message msg;
    msg.mutable_ping_target()->set_id(0);
    communicator.sendMessage(communicator.getSourceProcess(), communicator.getSourceThread(), &msg);
}

void SimulationSupervisor::init()
{
    // Open the simulation file.
    lm::io::hdf5::Hdf5File * file = new lm::io::hdf5::Hdf5File(simulationInputFilename);

    // Get the simulation parameters and read them into a map.
    file->getParameters(&simulationParametersBuf);
    for (int i=0; i<simulationParametersBuf.key_size() && i<simulationParametersBuf.value_size(); i++)
    {
        simulationParametersMap[simulationParametersBuf.key(i)] = simulationParametersBuf.value(i);
    }

    // Get the reaction model.
    if (file->hasReactionModel())
    {
        hasReactionModel = true;
        file->getReactionModel(&reactionModelBuf);
    }

    // Get the diffusion model.
    if (file->hasDiffusionModel())
    {
        hasDiffusionModel = true;
        file->getDiffusionModel(&diffusionModelBuf);

        // See if we need to fill in the boundary conditions from the simulation parameters.
        if (simulationParametersMap.count("boundaryConditions") == 1 && !diffusionModelBuf.has_boundary_conditions())
        {
            lm::io::BoundaryConditions* bc=diffusionModelBuf.mutable_boundary_conditions();
            if (!parseBoundaryConditions(bc, simulationParametersMap["boundaryConditions"].c_str()))
            {
                throw Exception("Could not parse boundaryConditions parameter",simulationParametersMap["boundaryConditions"].c_str());
            }
            if (simulationParametersMap.count("boundarySite") == 1)
            {
                bc->set_boundary_site(atoi(simulationParametersMap["boundarySite"].c_str()));
            }
            if (simulationParametersMap.count("boundarySpecies") == 1)
            {
                bc->set_boundary_species(atoi(simulationParametersMap["boundarySpecies"].c_str()));
            }
            if (simulationParametersMap.count("boundaryConcentration") == 1)
            {
                bc->set_boundary_concentration(atof(simulationParametersMap["boundaryConcentration"].c_str()));
            }
            if (file->hasBoundaryGradient())
            {
                file->getBoundaryGradient(bc);
            }
        }
    }

    if (file->hasOrderParameters())
    {
        hasOrderParameters = true;
        file->getOrderParameters(&orderParametersBuf);
        ops.init(orderParametersBuf);
    }

    if (file->hasTilings())
    {
        hasTilings = true;
        file->getTilings(&tilingsBuf);
        tilings.init(tilingsBuf);
    }

//    // initialize input struct (used for setting up trajectories)
    input = new lm::input::Input(hasDiffusionModel,hasOrderParameters,hasReactionModel,hasTilings,diffusionModelBuf,orderParametersBuf,ops,reactionModelBuf,simulationParametersBuf,simulationParametersMap,tilingsBuf,tilings);

    // close the file
    delete file;
}

bool SimulationSupervisor::parseBoundaryConditions(lm::io::BoundaryConditions* bc, std::string arg)
{
    lm::io::BoundaryConditions::BoundaryConditionsType type;

    // See if it is a global boundary condition.
    if (lm::io::BoundaryConditions_BoundaryConditionsType_Parse(arg, &type))
    {
        bc->set_global(type);
        return true;
    }

    // See if there are axis specific boundary conditions.
    char * argbuf = new char[arg.size()+1];
    memset(argbuf,0,arg.size()+1);
    strcpy(argbuf,arg.c_str());
    char * pch = strtok(argbuf,",");
    while (pch != NULL)
    {
        if (strlen(pch) >= 3 && (pch[0] == 'x' || pch[0] == 'y' || pch[0] == 'z') && pch[1] == ':')
        {
            // Parse the axis-specific type.
            if (!lm::io::BoundaryConditions_BoundaryConditionsType_Parse(std::string(pch+2), &type))
            {
                delete[] argbuf;
                return false;
            }

            // Set the axis value.
            pch[1] = '\0';
            std::string axis=pch;
            if (axis == "x")
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_x_plus(type);
                bc->set_x_minus(type);
            }
            else if (axis == "y")
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_y_plus(type);
                bc->set_y_minus(type);
            }
            else if (axis == "z")
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_z_plus(type);
                bc->set_z_minus(type);
            }
            else
            {
                delete[] argbuf;
                return false;
            }
        }
        else if (strlen(pch) >= 4 && ((pch[0] == '+' || pch[0] == '-') && (pch[1] == 'x' || pch[1] == 'y' || pch[1] == 'z')) && pch[2] == ':')
        {
            // Parse the axis-specific type.
            if (!lm::io::BoundaryConditions_BoundaryConditionsType_Parse(std::string(pch+3), &type))
            {
                delete[] argbuf;
                return false;
            }

            // Set the axis value.
            pch[2] = '\0';
            std::string axis=pch;
            if (axis == "+x" && type != lm::io::BoundaryConditions::PERIODIC)
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_x_plus(type);
            }
            else if (axis == "-x" && type != lm::io::BoundaryConditions::PERIODIC)
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_x_minus(type);
            }
            else if (axis == "+y" && type != lm::io::BoundaryConditions::PERIODIC)
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_y_plus(type);
            }
            else if (axis == "-y" && type != lm::io::BoundaryConditions::PERIODIC)
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_y_minus(type);
            }
            else if (axis == "+z" && type != lm::io::BoundaryConditions::PERIODIC)
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_z_plus(type);
            }
            else if (axis == "-z")
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_z_minus(type);
            }
            else
            {
                delete[] argbuf;
                return false;
            }
        }
        else
        {
            delete[] argbuf;
            return false;
        }
        pch = strtok(NULL,",");
    }
    delete[] argbuf;
    return bc->axis_specific_boundaries();
}


int SimulationSupervisor::run()
{
    try
    {
        Print::printf(Print::INFO, "Supervisor %d:%d started.", lm::MPI::worldRank, threadNumber);

        // Loop reading messages.
        lm::message::Message message;
        while (true)
        {
            // Read the next message.
            communicator.receiveMessage(&message);

            // Do something with the message.
            if (message.has_resources_available())
            {
                resourceAvailable(message.resources_available());
            }
            else if (message.has_started_work_unit_runner())
			{
                receivedStartedWorkUnitRunner(message.started_work_unit_runner());
			}
            else if (message.has_started_work_unit())
            {
                workUnitStarted(message.started_work_unit());
            }
            else if (message.has_finished_work_unit())
            {
                workUnitFinished(message.finished_work_unit());
            }
            else if (message.has_started_output_writer())
            {
                receivedStartedOutputWriter(message.started_output_writer());
            }
            else if (message.has_ping_target())
            {
                // If we are done running, stop the loop.
                if (!running) break;
            }
            else
            {
                Print::printf(Print::ERROR, "Supervisor received an unknown message: {\n%s}",message.DebugString().c_str());
            }

            // Clear the message object so it can be used again.
            message.Clear();
        }

        Print::printf(Print::INFO, "Supervisor %d:%d finished.", lm::MPI::worldRank, threadNumber);

        return 0;
    }
    catch (lm::Exception e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e.what(), __FILE__, __LINE__);
    }
    catch (lm::Exception* e)
	{
		Print::printf(Print::FATAL, "Exception during execution: %s (%s:%d)", e->what(), __FILE__, __LINE__);
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

void SimulationSupervisor::resourceAvailable(const lm::message::ResourcesAvailable& msg)
{
    Print::printf(Print::INFO, "Resource controller %d:%d on %s registered with %d cpu core(s) and %d gpu device(s).", msg.controller_process(), msg.controller_thread(), msg.hostname().c_str(), msg.cpu_size(), msg.gpu_size());
    if (resourceMap->registerResources(msg))
    {
        allResourcesRegistered();
    }
}

void SimulationSupervisor::allResourcesRegistered()
{
    // Start the work unit runners.
    Print::printf(Print::INFO, "All resources registered with supervisor, starting workers.");

    startOutputWriter();
    startWorkUnitRunners();
}

void SimulationSupervisor::startOutputWriter()
{
    // Reserve a core for the output writer.
    ComputeResources resources = resourceMap->reserveCPUCores(communicator.getSourceProcess(),1);
    Print::printf(Print::INFO, "Reserved core %d on %d:%d for the output writer.", resources.cpuCores[0], resources.controller_process, resources.controller_thread);

    // Start the output writer.
    lm::message::Message msg;
    msg.mutable_start_output_writer()->set_use_cpu_affinity(useCPUAffinity);
    msg.mutable_start_output_writer()->set_cpu(resources.cpuCores[0]);
    msg.mutable_start_output_writer()->set_output_filename(simulationOutputFilename);
    msg.mutable_start_output_writer()->set_output_writer_class(outputWriterClassName);
    communicator.sendMessage(resources.controller_process, resources.controller_thread, &msg);
}

void SimulationSupervisor::receivedStartedOutputWriter(const lm::message::StartedOutputWriter& msg)
{
    Print::printf(Print::INFO, "Output writer started: %d:%d.",msg.process(),msg.thread());
    hasOutputWriterStarted = true;
    outputWriterProcess = msg.process();
    outputWriterThread = msg.thread();
    startSimulationIfAllWorkersStarted();
}

void SimulationSupervisor::startWorkUnitRunners()
{
    map<int,ComputeResources> allResources = resourceMap->getAvailableResources();
    slots.createAllSlots(allResources, cpuCoresPerRunner, gpuDevicesPerRunner, useCPUAffinity, solverClassName, input);
}

void SimulationSupervisor::receivedStartedWorkUnitRunner(const lm::message::StartedWorkUnitRunner & msg)
{
    Print::printf(Print::INFO, "Work unit runner started: %d:%d.",msg.process(),msg.thread());

    slots.markSlotStarted(msg);
    if (!slots.hasUnstartedSlots())
    {
        haveAllWorkUnitRunnersStarted = true;
        startSimulationIfAllWorkersStarted();
    }
}

void SimulationSupervisor::startSimulationIfAllWorkersStarted()
{
    if (haveAllWorkUnitRunnersStarted && hasOutputWriterStarted)
    {
        Print::printf(Print::INFO, "All supervisor workers have started, beginning simulation.");
        startSimulation();
    }
}

void SimulationSupervisor::startSimulation()
{
    if (assignWork())
    {
        // If assign work returned true, there was nothing to be done.
        running = false;
        Print::printf(Print::INFO, "Simulation finished, no work to be performed.");
        finishSimulation();
    }
}

void SimulationSupervisor::finishSimulation()
{
    map<int,ComputeResources> resources = resourceMap->getAvailableResources();
    for (map<int,ComputeResources>::iterator it=resources.begin(); it != resources.end(); it++)
    {
        // Send a message for the resource controller to stop.
        lm::message::Message msg;
        msg.mutable_stop_resource_controller()->set_abort(false);
        communicator.sendMessage(it->second.controller_process, it->second.controller_thread, &msg);
    }
    running = false;
}

bool SimulationSupervisor::assignWork()
{
	// Go though the available slots and fill them with work units.
    while (true)
	{
        printf("1\n");
        // Allocate the next free slot, if there is one. Except for once (at the program's end), assignWork should return from here.
        if (!slots.hasFreeSlots()) return false;

        printf("2\n");

		// Get the next trajectory to run, if there is one.
		lm::message::Message * nextWorkUnitMsg = trajectories->getNextWorkUnitMsg();
        if (nextWorkUnitMsg != NULL)
        {
            printf("3\n");

            // Set the source process/thread
            nextWorkUnitMsg->mutable_run_work_unit()->set_supervisor_process(communicator.getSourceProcess());
            nextWorkUnitMsg->mutable_run_work_unit()->set_supervisor_thread(communicator.getSourceThread());

            // Set the writer process/thread
            nextWorkUnitMsg->mutable_run_work_unit()->set_output_process(outputWriterProcess);
            nextWorkUnitMsg->mutable_run_work_unit()->set_output_thread(outputWriterThread);

            // Run the work unit.
            slots.runWorkUnit(nextWorkUnitMsg);

            printf("4\n");
            return false;
        }
        else
		{
            printf("2.5\n");
			if (trajectories->isFinished())
			{
                printf("2.6\n");
                return true;	// When there's no more trajectories to run and it's time for the program to shut down, assignWork should return from here
			}
			else
			{
                printf("2.7\n");
                return false;	// Some trajectories are still running, there may still be more work units to come
			}
		}
	}
}

void SimulationSupervisor::workUnitStarted(const lm::message::StartedWorkUnit& msg)
{
    Print::printf(Print::DEBUG, "Work unit %d started.",msg.work_unit_id());
}

void SimulationSupervisor::workUnitFinished(const lm::message::FinishedWorkUnit& msg)
{
    Print::printf(Print::DEBUG, "Work unit %d finished in %0.3f s.",msg.work_unit_id(),msg.run_time());
    // If the trajectory associated with the finished work unit exists...
    /*if (trajectories->exists(msg.final_state().trajectory_id()))
    {
		// ...update the trajectory based on the results of the work unit
		trajectories->workUnitFinished(msg);
    }
    // Otherwise, assume that the associated trajectory has already been deleted and so skip reading in this result
    // The exists() check ensures that hangover results from older fflux phases aren't recorded as belonging to a newer phase

    // Free the slot that the returning work unit just ran on
    slots.workUnitFinished(msg);

    // Fill the newly freed slot with a work unit. If there are more trajectories than slots, this is guaranteed to use the slot we just freed. Otherwise it will be the "coldest" (longest unoccupied) slot
    if (assignWork())
    {
    	if (running)
    	{
    		running = false;
    		Print::printf(Print::INFO, "finish simulation hit");
			finishSimulation();
    	}
    }*/
}

}
}
