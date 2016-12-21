/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
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
#include <map>
#include <string>
#include <vector>

#include "lm/ClassFactory.h"
#include "lm/EnumHelper.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/Input.h"
#include "lm/input/OutputOptions.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/main/Globals.h"
#include "lm/option/SimulationParameters.h"
#include "lm/Print.h"
#include "lm/limit/TrajectoryLimits.h"
#include "lm/Types.h"

using lm::input::OutputOptions;
using std::map;
using std::string;
using std::vector;

namespace lm {
namespace input {

bool Input::registered=Input::registerClass();

bool Input::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::input::Input","lm::input::Input",(ClassAllocator)&Input::allocateObject);
    return true;
}

void* Input::allocateObject(const lm::io::hdf5::Hdf5File& file)
{
    return new Input(file);
}

Input::Input()
:degreeAdvancementPresent(false),diffusionModelPresent(false),reactionModelPresent(false),orderParametersPresent(false),
 outputOptionsPresent(false),tilingsPresent(false),trajectoryLimitsPresent(false),limitTrackingListWrap(&limitTrackingListMsg),
 includeEndpointInLimits(true),partsPerWorkUnit(1),stepsPerWorkUnit((uint64_t)1e8)
{
}

Input::Input(const lm::io::hdf5::Hdf5File& file)
:degreeAdvancementPresent(false),diffusionModelPresent(false),reactionModelPresent(false),orderParametersPresent(false),
 outputOptionsPresent(false),tilingsPresent(false),trajectoryLimitsPresent(false),limitTrackingListWrap(&limitTrackingListMsg),
 includeEndpointInLimits(true),partsPerWorkUnit(1),stepsPerWorkUnit((uint64_t)1e8)
{
    init(file);
}

Input::~Input()
{
}

void Input::init(const lm::io::hdf5::Hdf5File& file)
{
    simulationParameters.rFF(file);

    initReactionModel(file);
    initDiffusionModel(file);
    initOrderParameters(file);
    initTilings(file);
    initTrajectoryLimits(file);
    initOutputOptions(file);
    initWorkUnitParameters(file);

    // warn the user about any unrecognized/unparsed simulation parameters
    initSanityCheck();
}

// Get the reaction model.
void Input::initReactionModel(const lm::io::hdf5::Hdf5File& file)
{
    if (file.hasReactionModel())
    {
        file.getReactionModel(&reactionModelMsg);
        reactionModelPresent = true;
    }
}

// Get the diffusion model.
void Input::initDiffusionModel(const lm::io::hdf5::Hdf5File& file)
{
    if (file.hasDiffusionModel())
    {
        file.getDiffusionModel(&diffusionModel);
        diffusionModelPresent = true;

        // See if we need to fill in the boundary conditions from the simulation parameters.
        if (simulationParameters.count("boundaryConditions") == 1 && !diffusionModel.has_boundary_conditions())
        {
            lm::input::BoundaryConditions* bc = diffusionModel.mutable_boundary_conditions();
            if (!parseBoundaryConditions(bc, simulationParameters["boundaryConditions"].c_str()))
            {
                throw Exception("Could not parse boundaryConditions parameter", simulationParameters["boundaryConditions"].c_str());
            }
            if (simulationParameters.count("boundarySite") == 1)
            {
                bc->set_boundary_site(atoi(simulationParameters["boundarySite"].c_str()));
            }
            if (simulationParameters.count("boundarySpecies") == 1)
            {
                bc->set_boundary_species(atoi(simulationParameters["boundarySpecies"].c_str()));
            }
            if (simulationParameters.count("boundaryConcentration") == 1)
            {
                bc->set_boundary_concentration(atof(simulationParameters["boundaryConcentration"].c_str()));
            }
            if (file.hasBoundaryGradient())
            {
                file.getBoundaryGradient(bc);
            }
        }
    }
}

// Get the order parameters.
void Input::initOrderParameters(const lm::io::hdf5::Hdf5File& file)
{
    if (file.hasOrderParameters())
    {
        file.getOrderParameters(&orderParametersMsg);
        orderParameters.init(&file);
        orderParametersPresent = true;
    }
}

// Get the tilings.
void Input::initTilings(const lm::io::hdf5::Hdf5File& file)
{
    if (file.hasTilings())
    {
        file.getTilings(&tilingsMsg);
        tilings.init(&file, orderParameters);
        // run a consistency check on the basins in the tiling (if any)
        tilings.testBasinsPosition();
        tilings.testBasinsSize(reactionModelMsg);
        tilingsPresent = true;
    }
}

// Get the limits.
void Input::initTrajectoryLimits(const lm::io::hdf5::Hdf5File& file)
{
    // - By default, we include endpoints when checking limits 
    //     - eg if limitType==MAX and limitVal==2, then the limit will be triggered when currentVal >= 2, as opposed to being triggered only when currentVal > 2
    // - The user can override this behavior with the following (advanced) option
    parseAndSet("includeEnpointInLimits", &this->includeEndpointInLimits);

    // See if we have a max time limit.
    if (simulationParameters.count("maxTime"))
    {
        trajectoryLimits.addLimitMsg<TrajLimEnums::TIME>(0, simulationParameters.parse<double>("maxTime"), TrajLimEnums::MAX, includeEndpointInLimits);
    }

    // set the other limits, if present in the simulation parameters
    degreeAdvancementPresent = parseLimits<TrajLimEnums::DEGREE_ADVANCEMENT>("degreeAdvancementLowerLimitList", "degree advancement lower limit", TrajLimEnums::MIN, includeEndpointInLimits);
    degreeAdvancementPresent = parseLimits<TrajLimEnums::DEGREE_ADVANCEMENT>("degreeAdvancementUpperLimitList", "degree advancement upper limit", TrajLimEnums::MAX, includeEndpointInLimits);

    parseLimits<TrajLimEnums::ORDER_PARAMETER>("orderParameterLowerLimitList", "order parameter lower limit", TrajLimEnums::MIN, includeEndpointInLimits);
    parseLimits<TrajLimEnums::ORDER_PARAMETER>("orderParameterUpperLimitList", "order parameter upper limit", TrajLimEnums::MAX, includeEndpointInLimits);

    parseLimits<TrajLimEnums::SPECIES>("speciesLowerLimitList", "species lower limit", TrajLimEnums::MIN, includeEndpointInLimits);
    parseLimits<TrajLimEnums::SPECIES>("speciesUpperLimitList", "species upper limit", TrajLimEnums::MAX, includeEndpointInLimits);
}

// Get the output options.
void Input::initOutputOptions(const lm::io::hdf5::Hdf5File& file)
{
    // This flag changes the organization of the output such that the total number of groups and datasets is minimized. Currently only implemented (partially) for HDF5, no effect otherwise
    parseAndSet("condenseOutput", &OutputOptions::set_condense_output, outputOptionsMsg);

    // Flags that control whether output is recorded for the initial and/or the final state of every trajectory.
    parseAndSet("writeInitialTrajectoryState", &OutputOptions::set_write_initial_trajectory_state, outputOptionsMsg);
    parseAndSet("writeFinalTrajectoryState", &OutputOptions::set_write_final_trajectory_state, outputOptionsMsg);

    // Flag that globally controls whether any limit tracking data collected during a trajectory is written out directly to disk.
    parseAndSet("writeLimitTracking", &OutputOptions::set_write_limit_tracking, outputOptionsMsg);

    // Specify the period at which various outputs should be written out. Leave a WriteInterval unset to suppress its related output
    degreeAdvancementPresent = parseAndSet("degreeAdvancementWriteInterval", &OutputOptions::set_degree_advancement_write_interval, outputOptionsMsg);
    parseAndSet("latticeWriteInterval", &OutputOptions::set_lattice_write_interval, outputOptionsMsg);
    parseAndSet("orderParameterWriteInterval", &OutputOptions::set_order_parameter_write_interval, outputOptionsMsg);
    parseAndSet("writeInterval", &OutputOptions::set_species_write_interval, outputOptionsMsg);

    // Initialize the species counts first passage times in the output options
    parseAndSetList("fptTrackingList", &OutputOptions::add_fpt_species_to_track, outputOptionsMsg);

    // Initialize the order parameter values first passage times in the output options
    parseAndSetList("fptOrderParameterTrackingList", &OutputOptions::add_fpt_order_parameter_to_track, outputOptionsMsg);

}

// Get some parameters that tweak how work units are run
void Input::initWorkUnitParameters(const lm::io::hdf5::Hdf5File& file)
{
    parseAndSet("maxWorkUnitSteps", &this->stepsPerWorkUnit);
    parseAndSet("partsPerWorkUnit", &this->partsPerWorkUnit);
}

void Input::initSanityCheck()
{
    if (not simulationParameters.checkAllParsed())
    {
        simulationParameters.printUnparsed();
    }
}

void Input::copyLimitsTo(lm::message::RunWorkUnit* rwuMsg)
{
    rwuMsg->mutable_trajectory_limits()->CopyFrom(getTrajectoryLimitsMsg());
}

void Input::copyLimitTrackingsTo(lm::message::RunWorkUnit* rwuMsg)
{
    if (limitTrackingListWrap.limit_trackings_size() > 1)
    {
        for (lm::protowrap::Repeated<lm::message::WorkUnit>::iterator it=rwuMsg->mutable_part()->begin();it!=rwuMsg->mutable_part()->end();it++)
        {
            if (not it->initial_state().trajectory_started())
            {
                limitTrackingListWrap.set_all_trajectory_id(it->initial_state().trajectory_id());
                it->mutable_initial_state()->mutable_limit_tracking_list()->CopyFrom(limitTrackingListWrap.wrappedMsg());
            }
        }
    }
}

bool Input::parseBoundaryConditions(lm::input::BoundaryConditions* bc, string arg)
{
    lm::input::BoundaryConditions::BoundaryConditionsType type;

    // See if it is a global boundary condition.
    if (lm::input::BoundaryConditions_BoundaryConditionsType_Parse(arg, &type))
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
            if (!lm::input::BoundaryConditions_BoundaryConditionsType_Parse(std::string(pch+2), &type))
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
            if (!lm::input::BoundaryConditions_BoundaryConditionsType_Parse(std::string(pch+3), &type))
            {
                delete[] argbuf;
                return false;
            }

            // Set the axis value.
            pch[2] = '\0';
            std::string axis=pch;
            if (axis == "+x" && type != lm::input::BoundaryConditions::PERIODIC)
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_x_plus(type);
            }
            else if (axis == "-x" && type != lm::input::BoundaryConditions::PERIODIC)
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_x_minus(type);
            }
            else if (axis == "+y" && type != lm::input::BoundaryConditions::PERIODIC)
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_y_plus(type);
            }
            else if (axis == "-y" && type != lm::input::BoundaryConditions::PERIODIC)
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_y_minus(type);
            }
            else if (axis == "+z" && type != lm::input::BoundaryConditions::PERIODIC)
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

}
}
