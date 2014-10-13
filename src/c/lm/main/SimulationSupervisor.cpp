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


#include "lm/Exceptions.h"
#include "lm/MPI.h"
#include "lm/Print.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/hdf5/HDF5.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Communicator.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ResourcesAvailable.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/message/StartedWorkUnit.pb.h"
#include "lm/message/StartedWorkUnitRunner.pb.h"
#include "lm/resource/ResourceMap.h"
#include "lm/resource/SlotList.h"

using lm::resource::ResourceMap;

namespace lm {
namespace main {

SimulationSupervisor::SimulationSupervisor()
    :workUnitCount(0),trajectories(NULL),communicator(lm::MPI::worldRank,THREAD_ID),resourceMap(NULL),simulationInputFilename(""),simulationOutputFilename(""),outputWriterClassName(""),solverClassName(""),useCPUAffinity(false),hasReactionModel(false),hasDiffusionModel(false),slots(&communicator)
{
}

SimulationSupervisor::~SimulationSupervisor()
{
}

void SimulationSupervisor::wake() throw(lm::thread::PthreadException)
{
    lm::message::Message msg;
    msg.mutable_ping_target()->set_id(0);
    communicator.sendMessage(communicator.getSourceProcess(), communicator.getSourceThread(), &msg);
}

void SimulationSupervisor::initialize()
{
    // Open the simulation file.
    lm::io::hdf5::Hdf5File * file = new lm::io::hdf5::Hdf5File(simulationInputFilename);

    // Get the simulation parameters.
    file->getParameters(&simulationParameters);

    // Map the simulation parameters.
    for (int i=0; i<simulationParameters.key_size() && i<simulationParameters.value_size(); i++)
        this->simulationParameterMap[simulationParameters.key(i)] = simulationParameters.value(i);

    // Get the reaction model.
    if (file->hasReactionModel())
    {
        hasReactionModel = true;
        file->getReactionModel(&reactionModel);
    }

    // Get the diffusion model.
    if (file->hasDiffusionModel())
    {
        hasDiffusionModel = true;
        file->getDiffusionModel(&diffusionModel);

        // See if we need to fill in the boundary conditions from the simulation parameters.
        if (simulationParameterMap.count("boundaryConditions") == 1 && !diffusionModel.has_boundary_conditions())
        {
            lm::io::BoundaryConditions* bc=diffusionModel.mutable_boundary_conditions();
            if (!parseBoundaryConditions(bc, simulationParameterMap["boundaryConditions"].c_str()))
            {
                throw Exception("Could not parse boundaryConditions parameter",simulationParameterMap["boundaryConditions"].c_str());
            }
            if (simulationParameterMap.count("boundarySite") == 1)
            {
                bc->set_boundary_site(atoi(simulationParameterMap["boundarySite"].c_str()));
            }
            if (simulationParameterMap.count("boundarySpecies") == 1)
            {
                bc->set_boundary_species(atoi(simulationParameterMap["boundarySpecies"].c_str()));
            }
            if (simulationParameterMap.count("boundaryConcentration") == 1)
            {
                bc->set_boundary_concentration(atof(simulationParameterMap["boundaryConcentration"].c_str()));
            }
            if (file->hasBoundaryGradient())
            {
                file->getBoundaryGradient(bc);
            }
        }
    }

//    // Get the forward flux parameters
//    if (file->hasFFluxParameters())
//    {
//        hasFFluxParameters = true;
//        file->getFFluxParameters(&ffluxParameters);
//    }

    if (file->hasOrderParameters())
    {
        hasOrderParameters = true;
        file->getOrderParameters(&orderParameters);
    }

    if (file->hasTilings())
    {
        hasTilings = true;
        file->getTilings(&tilingsBuf);
        tilings = new lm::tiling::Tilings(tilingsBuf);
    }

    // Close the file.
    delete file;
}

bool SimulationSupervisor::parseBoundaryConditions(lm::io::BoundaryConditions* bc, std::string arg)
{
    lm::io::BoundaryConditions_BoundaryConditionsType type;

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
            	workUnitRunnerStarted(message.started_work_unit_runner());
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
                outputWriterStarted(message.started_output_writer());
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

void SimulationSupervisor::workUnitRunnerStarted(const lm::message::StartedWorkUnitRunner & msg)
{
	if (slots.workUnitRunnerStarted(msg))
	{
		allWorkUnitRunnersStarted();
	}
}

void SimulationSupervisor::allResourcesRegistered()
{
    // Start the work unit runners.
    Print::printf(Print::INFO, "All resources registered with supervisor, starting work unit runners.");

    // Set up the template messages (which contain default values) for slots and trajectories
    lm::message::StartWorkUnitRunner * startSlotMsg = slots.addStartSlotMsg();
    //	s->set_use_cpu_affinity(useCPUAffinity);
    //	s->add_cpu(resources.cpuCores[i]);
    //	if (resources.gpusDevices.size() > 0)
    //		s->add_gpu(resources.gpusDevices[0]);
    startSlotMsg->set_solver(solverClassName);
	*startSlotMsg->mutable_simulation_parameters() = simulationParameters;
	if (hasReactionModel) *startSlotMsg->mutable_reaction_model() = reactionModel;
	if (hasDiffusionModel) *startSlotMsg->mutable_diffusion_model() = diffusionModel;
	if (hasOrderParameters) {
	    *startSlotMsg->mutable_order_parameters() = orderParameters;
//	    startSlotMsg->set_esample_type(lm::message::StartWorkUnitRunner::FFLUX);
	}

	map<int,ResourceMap::ComputeResources> allResources = resourceMap->getAvailableResources();
    slots.addSlots(allResources);
}

void SimulationSupervisor::allWorkUnitRunnersStarted()
{
    Print::printf(Print::INFO, "All work unit runners started, beginning simulation.");
    startSimulation();
}

void SimulationSupervisor::startSimulation()
{
    assignWork();
}

void SimulationSupervisor::finishSimulation()
{
    map<int,ResourceMap::ComputeResources> resources = resourceMap->getAvailableResources();
    for (map<int,ResourceMap::ComputeResources>::iterator it=resources.begin(); it != resources.end(); it++)
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
		// Allocate the next free slot, if there is one.
		lm::resource::Slot * workSlot = slots.alloc();
		if (workSlot==NULL) return false;	// Except for once (at the program's end), assignWork should return from here

		// Get the next trajectory to run, if there is one.
		lm::message::Message * nextWorkUnitMsg = trajectories->getNextWorkUnitMsg();
		if (nextWorkUnitMsg==NULL)
		{
			if (trajectories->isFinished())
			{
				return true;	// When there's no more trajectories to run and it's time for the program to shut down, assignWork should return from here
			}
			else
			{
				return false;	// Some trajectories are still running, there may still be more work units to come
			}
		}

		// If we got this far, put the next free slot together with the next trajectory
		workSlot->workUnitRemoteStart(nextWorkUnitMsg, workUnitCount++);
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
    if (trajectories->exists(msg.final_state().trajectory_id()))
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
    }

}

void SimulationSupervisor::initLimits()
{
	// See if we have a max time limit.
	if (simulationParameterMap.count("maxTime"))
		limits.set_max_time(atof(simulationParameterMap["maxTime"].c_str()));

	// Set the species lower limits from the parameters.
	if (simulationParameterMap.count("speciesLowerLimitList"))
	{
		for (int i=0; i<(int)reactionModel.number_species(); i++)
			limits.add_min_species_count(-1);

		string listString = simulationParameterMap["speciesLowerLimitList"];
		size_t start=0, end=0;
		while (end != string::npos)
		{
			end = listString.find(',', start);
			string speciesLowerLimit = listString.substr(start, (end == string::npos) ? string::npos : end - start);

			size_t equalsPos=0;
			equalsPos = speciesLowerLimit.find(':', 0);
			if (equalsPos > 0 && equalsPos < speciesLowerLimit.length()-1)
			{
				int parsedSpecies = atoi(speciesLowerLimit.substr(0, equalsPos).c_str());
				int parsedLimit = atoi(speciesLowerLimit.substr(equalsPos+1, string::npos).c_str());
				limits.set_min_species_count(parsedSpecies, parsedLimit);
				Print::printf(Print::DEBUG, "Parsed lower limit %s to: %d => %d", speciesLowerLimit.c_str(), parsedSpecies, parsedLimit);
			}
			start = end+1;
		}
	}

	// Set the species upper limits from the parameters.
	if (simulationParameterMap.count("speciesUpperLimitList"))
	{
		for (int i=0; i<(int)reactionModel.number_species(); i++)
			limits.add_max_species_count(-1);

		string listString = simulationParameterMap["speciesUpperLimitList"];
		size_t start=0, end=0;
		while (end != string::npos)
		{
			end = listString.find(',', start);
			string speciesUpperLimit = listString.substr(start, (end == string::npos) ? string::npos : end - start);

			size_t equalsPos=0;
			equalsPos = speciesUpperLimit.find(':', 0);
			if (equalsPos > 0 && equalsPos < speciesUpperLimit.length()-1)
			{
				uint parsedSpecies = atoi(speciesUpperLimit.substr(0, equalsPos).c_str());
				uint parsedLimit = atoi(speciesUpperLimit.substr(equalsPos+1, string::npos).c_str());
				limits.set_max_species_count(parsedSpecies, parsedLimit);
				Print::printf(Print::DEBUG, "Parsed upper limit %s to: %d <= %d", speciesUpperLimit.c_str(), parsedSpecies, parsedLimit);
			}
			start = end+1;
		}
	}
}

}
}
