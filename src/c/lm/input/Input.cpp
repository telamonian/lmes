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

#include "lm/EnumHelper.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/Input.h"
#include "lm/input/OutputOptions.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/option/SimulationParameters.h"
#include "lm/Print.h"
#include "lm/limit/TrajectoryLimits.h"
#include "lm/Types.h"

using lm::input::OutputOptions;
using lm::limit::LimitValueT;
using std::map;
using std::string;
using std::vector;

namespace lm {
namespace input {

Input::Input(const lm::io::hdf5::Hdf5File& file)
:reactionModelPresent(false),diffusionModelPresent(false),orderParametersPresent(false),tilingsPresent(false),trajectoryLimitsPresent(false),
 outputOptionsPresent(false),simulationParameters(file),partsPerWorkUnit(1),stepsPerWorkUnit(10000000)
{
    init(file);
}

Input::~Input()
{
}

void Input::init(const lm::io::hdf5::Hdf5File& file)
{
    initReactionModel(file);
    initDiffusionModel(file);
    initOrderParameters(file);
    initTilings(file);
    initTrajectoryLimits(file);
    initOutputOptions(file);
    initWorkUnitParameters(file);
}

// Get the reaction model.
void Input::initReactionModel(const lm::io::hdf5::Hdf5File& file)
{
    if (file.hasReactionModel())
    {
        file.getReactionModel(&reactionModel);
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
        tilings.testBasinsSize(reactionModel);
        tilingsPresent = true;
    }
}

// Get the limits.
void Input::initTrajectoryLimits(const lm::io::hdf5::Hdf5File& file)
{
    // See if we have a max time limit.
    if (simulationParameters.count("maxTime"))
    {
        trajectoryLimits.addLimitMsg<TrajLimEnums::TIME>(0, simulationParameters.parse<double>("maxTime"), TrajLimEnums::MAX);
        trajectoryLimitsPresent = true;
    }

    // set the other limits, if present in the simulation parameters
    if (parseLimits<TrajLimEnums::DEGREE_ADVANCEMENT>("degreeAdvancementLowerLimitList", "degree advancement lower limit", TrajLimEnums::MIN) ||
        parseLimits<TrajLimEnums::DEGREE_ADVANCEMENT>("degreeAdvancementUpperLimitList", "degree advancement upper limit", TrajLimEnums::MAX))
    {
        trajectoryLimitsPresent = true;
        degreeAdvancementPresent = true;
    }
    if (parseLimits<TrajLimEnums::ORDER_PARAMETER>("orderParameterLowerLimitList", "order parameter lower limit", TrajLimEnums::MIN) ||
        parseLimits<TrajLimEnums::ORDER_PARAMETER>("orderParameterUpperLimitList", "order parameter upper limit", TrajLimEnums::MAX) ||
        parseLimits<TrajLimEnums::SPECIES>("speciesLowerLimitList", "species lower limit", TrajLimEnums::MIN) ||
        parseLimits<TrajLimEnums::SPECIES>("speciesUpperLimitList", "species upper limit", TrajLimEnums::MAX))
    {
        trajectoryLimitsPresent = true;
    }
}

// Get the output options.
void Input::initOutputOptions(const lm::io::hdf5::Hdf5File& file)
{
    if (parseAndSet("degreeAdvancementWriteInterval", &OutputOptions::set_degree_advancement_write_interval, outputOptions))
    {
        degreeAdvancementPresent = true;
        outputOptionsPresent = true;
    }

    // Initialize the species counts first passage times in the output options
    if (simulationParameters.count("fptTrackingList"))
    {
        const string listString = simulationParameters["fptTrackingList"];
        std::list<int> fptList;
        size_t start=0, end=0;
        while (end != string::npos)
        {
            end = listString.find(',', start);
            string trackedSpecies = listString.substr(start, (end == string::npos) ? string::npos : end - start);
            if (trackedSpecies.length() > 0)
            {
                outputOptions.add_fpt_species_to_track((uint)atoi(trackedSpecies.c_str()));
            }
            start = end+1;
        }
        outputOptionsPresent = true;
    }

    // Initialize the order parameter values first passage times in the output options
    if (parseAndSetList("fptOrderParameterTrackingList", &OutputOptions::add_fpt_order_parameter_to_track, outputOptions))
    {
        outputOptionsPresent = true;
    }

    if (simulationParameters.count("latticeWriteInterval"))
    {
        outputOptions.set_lattice_write_interval(atof(simulationParameters["latticeWriteInterval"].c_str()));
        outputOptionsPresent = true;
    }

    if (parseAndSet("orderParameterWriteInterval", &OutputOptions::set_order_parameter_write_interval, outputOptions))
    {
        outputOptionsPresent = true;
    }

    if (simulationParameters.count("writeInterval"))
    {
        outputOptions.set_species_write_interval(atof(simulationParameters["writeInterval"].c_str()));
        outputOptionsPresent = true;
    }
}

// Get some parameters that tweak how work units are run
void Input::initWorkUnitParameters(const lm::io::hdf5::Hdf5File& file)
{
    parseAndSet("partsPerWorkUnit", &this->partsPerWorkUnit);
    parseAndSet("maxWorkUnitSteps", &this->stepsPerWorkUnit);
}

void Input::copyLimitsTo(lm::message::RunWorkUnit* rwuMsg)
{
    rwuMsg->mutable_trajectory_limits()->CopyFrom(getTrajectoryLimitsMsg());
}

void Input::copyLimitTrackingsTo(lm::message::RunWorkUnit* rwuMsg)
{
    for (lm::protowrap::Repeated<lm::message::WorkUnit>::iterator it=rwuMsg->mutable_part()->begin();it!=rwuMsg->mutable_part()->end();it++)
    {
        trajectoryLimits.setTrackingTrajectoryID(it->initial_state().trajectory_id());
        it->mutable_initial_state()->mutable_limit_trackings()->CopyFrom(trajectoryLimits.getTrackingRepeated());
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

template <TrajLimEnums::LimitType LT> bool Input::parseLimits(const string key, const string debugString, TrajLimEnums::StoppingCondition sc, bool includeEndpoint)
{
    if (simulationParameters.count(key))
    {
        typename pairVector<uint, typename LimitValueT<LT>::type>::type idLimitVec(simulationParameters.parsePairVector<uint, typename LimitValueT<LT>::type>(key, debugString));
        for (typename pairVector<uint, typename LimitValueT<LT>::type>::iterator it(idLimitVec.begin()); it!=idLimitVec.end(); it++)
        {
            trajectoryLimits.addLimitMsg<LT>(it->first, it->second, sc, includeEndpoint);
        }
        return idLimitVec.size() > 0;
    }
    else
    {
        return false;
    }
}

// Version of parseAndSet for fields that can be passed in as mutable pointers
// By using template parameter inference on the pointer, this template automatically figures out what type to parse from simulationParameters
template <typename ValT> bool Input::parseAndSet(const string key, ValT* fieldPtr)
{
    if (simulationParameters.count(key))
    {
        *fieldPtr = simulationParameters.parse<ValT>(key);
        return true;
    }
    else
    {
        return false;
    }
}

// Version of parseAndSet for fields that have setters
// By using template parameter inference on the setter (passed as a function pointer), this template automatically figures out what type to parse from simulationParameters
template <typename T, typename SetterReturnT, typename ValT> bool Input::parseAndSet(const string key, SetterReturnT (T::*setterFunc)(ValT), T& obj)
{
    if (simulationParameters.count(key))
    {
        (obj.*setterFunc)(simulationParameters.parse<ValT>(key));
        return true;
    }
    else
    {
        return false;
    }
}

// Same as parseAndSet, but for options specified as lists
template <typename T, typename AdderReturnT, typename ValT> bool Input::parseAndSetList(const string key, AdderReturnT (T::*adderFunc)(ValT), T& obj)
{
    if (simulationParameters.count(key))
    {
        std::vector<ValT> parsedVector(simulationParameters.parseVector<ValT>(key));
        for (typename vector<ValT>::const_iterator it=parsedVector.begin(); it!=parsedVector.end(); it++)
        {
            (obj.*adderFunc)(*it);
        }
        return parsedVector.size() > 0;
    }
    else
    {
        return false;
    }
};

};
}
