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

#include "lm/Print.h"
#include "lm/input/Input.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/BoundaryConditions.pb.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.pb.h"
#include "lm/oparam/OParams.h"
#include "lm/tiling/Tilings.h"

using std::map;
using std::string;

namespace lm {
namespace input {

Input::Input(const lm::io::hdf5::Hdf5File& file)
:reactionModelPresent(false),diffusionModelPresent(false),orderParametersPresent(false),tilingsPresent(false),trajectoryLimitsPresent(false),outputOptionsPresent(false),
 stepsPerWorkUnit(10000000)
{
    // Get the simulation parameters.
    file.getParameters(&simulationParametersMsg);
    for (int i=0; i<simulationParametersMsg.key_size() && i<simulationParametersMsg.value_size(); i++)
    {
        simulationParameters[simulationParametersMsg.key(i)] = simulationParametersMsg.value(i);
    }

    // Get the reaction model.
    if (file.hasReactionModel())
    {
        file.getReactionModel(&reactionModel);
        reactionModelPresent = true;
    }

    // Get the diffusion model.
    if (file.hasDiffusionModel())
    {
        file.getDiffusionModel(&diffusionModel);
        diffusionModelPresent = true;

        // See if we need to fill in the boundary conditions from the simulation parameters.
        if (simulationParameters.count("boundaryConditions") == 1 && !diffusionModel.has_boundary_conditions())
        {
            lm::io::BoundaryConditions* bc=diffusionModel.mutable_boundary_conditions();
            if (!parseBoundaryConditions(bc, simulationParameters["boundaryConditions"].c_str()))
            {
                throw Exception("Could not parse boundaryConditions parameter",simulationParameters["boundaryConditions"].c_str());
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

    // Get the order parameters.
    if (file.hasOrderParameters())
    {
        file.getOrderParameters(&orderParametersMsg);
        orderParameters.init(&file);
        orderParametersPresent = true;
    }

    // Get the tilings.
    if (file.hasTilings())
    {
        file.getTilings(&tilingsMsg);
        tilings.init(&file);
        tilingsPresent = true;
    }

    // Get the limits.
    {
        // See if we have a max time limit.
        if (simulationParameters.count("maxTime"))
        {
            trajectoryLimits.set_max_time_limit(atof(simulationParameters["maxTime"].c_str()));
            trajectoryLimitsPresent = true;
        }

        // Set the species lower limits from the parameters.
        if (simulationParameters.count("speciesLowerLimitList"))
        {
            string listString = simulationParameters["speciesLowerLimitList"];
            size_t start=0, end=0;
            while (end != string::npos)
            {
                end = listString.find(',', start);
                string speciesLowerLimit = listString.substr(start, (end == string::npos) ? string::npos : end - start);

                size_t equalsPos=0;
                equalsPos = speciesLowerLimit.find(':', 0);
                if (equalsPos > 0 && equalsPos < speciesLowerLimit.length()-1)
                {
                    uint parsedSpecies = (uint)atoi(speciesLowerLimit.substr(0, equalsPos).c_str());
                    int parsedLimit = atoi(speciesLowerLimit.substr(equalsPos+1, string::npos).c_str());
                    lm::io::TrajectoryLimits::SpeciesCountLimit* limit = trajectoryLimits.add_min_species_count_limit();
                    limit->set_species_id(parsedSpecies);
                    limit->set_value(parsedLimit);
                    Print::printf(Print::DEBUG, "Parsed lower limit %s to: %d => %d", speciesLowerLimit.c_str(), parsedSpecies, parsedLimit);
                }
                start = end+1;
            }
            trajectoryLimitsPresent = true;
        }

        // Set the species upper limits from the parameters.
        if (simulationParameters.count("speciesUpperLimitList"))
        {
            string listString = simulationParameters["speciesUpperLimitList"];
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
                    lm::io::TrajectoryLimits::SpeciesCountLimit* limit = trajectoryLimits.add_max_species_count_limit();
                    limit->set_species_id(parsedSpecies);
                    limit->set_value(parsedLimit);
                    Print::printf(Print::DEBUG, "Parsed upper limit %s to: %d <= %d", speciesUpperLimit.c_str(), parsedSpecies, parsedLimit);
                }
                start = end+1;
            }
            trajectoryLimitsPresent = true;
        }

        // Set the order parameter upper limits from an order parameter.
        if (simulationParameters.count("orderParameterUpperLimitList"))
        {
            string listString = simulationParameters["orderParameterUpperLimitList"];
            size_t start=0, end=0;
            while (end != string::npos)
            {
                end = listString.find(',', start);
                string orderParameterUpperLimit = listString.substr(start, (end == string::npos) ? string::npos : end - start);

                size_t equalsPos=0;
                equalsPos = orderParameterUpperLimit.find(':', 0);
                if (equalsPos > 0 && equalsPos < orderParameterUpperLimit.length()-1)
                {
                    uint parsedOrderParameter = atoi(orderParameterUpperLimit.substr(0, equalsPos).c_str());
                    double parsedLimit = atof(orderParameterUpperLimit.substr(equalsPos+1, string::npos).c_str());
                    lm::io::TrajectoryLimits::IncreasingOrderParameterLimit* limit = trajectoryLimits.add_increasing_order_parameter_limit();
                    limit->set_order_parameter_id(parsedOrderParameter);
                    limit->add_value(parsedLimit);
                    limit->set_arrangement(lm::io::TrajectoryLimits::ASCENDING);
                    Print::printf(Print::DEBUG, "Parsed op upper limit %s to: %d <= %e", orderParameterUpperLimit.c_str(), parsedOrderParameter, parsedLimit);
                }
                start = end+1;
            }
            trajectoryLimitsPresent = true;
        }

        // Set the order parameter lower limits from an order parameter.
        if (simulationParameters.count("orderParameterLowerLimitList"))
        {
            string listString = simulationParameters["orderParameterLowerLimitList"];
            size_t start=0, end=0;
            while (end != string::npos)
            {
                end = listString.find(',', start);
                string orderParameterLowerLimit = listString.substr(start, (end == string::npos) ? string::npos : end - start);

                size_t equalsPos=0;
                equalsPos = orderParameterLowerLimit.find(':', 0);
                if (equalsPos > 0 && equalsPos < orderParameterLowerLimit.length()-1)
                {
                    uint parsedOrderParameter = atoi(orderParameterLowerLimit.substr(0, equalsPos).c_str());
                    double parsedLimit = atof(orderParameterLowerLimit.substr(equalsPos+1, string::npos).c_str());
                    lm::io::TrajectoryLimits::DecreasingOrderParameterLimit* limit = trajectoryLimits.add_decreasing_order_parameter_limit();
                    limit->set_order_parameter_id(parsedOrderParameter);
                    limit->add_value(parsedLimit);
                    limit->set_arrangement(lm::io::TrajectoryLimits::DESCENDING);
                    Print::printf(Print::DEBUG, "Parsed op lower limit %s to: %d <= %e", orderParameterLowerLimit.c_str(), parsedOrderParameter, parsedLimit);
                }
                start = end+1;
            }
            trajectoryLimitsPresent = true;
        }
    }

    // Get the output options.
    {
        if (simulationParameters.count("writeInterval"))
        {
            outputOptions.set_species_write_interval(atof(simulationParameters["writeInterval"].c_str()));
            outputOptionsPresent = true;
        }

        if (simulationParameters.count("latticeWriteInterval"))
        {
            outputOptions.set_lattice_write_interval(atof(simulationParameters["latticeWriteInterval"].c_str()));
            outputOptionsPresent = true;
        }

        // Get the first passage times.
        if (simulationParameters.count("fptTrackingList"))
        {
            // Initialize the first passage times in the cme state.
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
    }

    // Get some generic input options.
    if (simulationParameters.count("maxWorkUnitSteps"))
        stepsPerWorkUnit = atoll(simulationParameters["maxWorkUnitSteps"].c_str());

}

Input::~Input()
{
}

bool Input::parseBoundaryConditions(lm::io::BoundaryConditions* bc, string arg)
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


//
//    // has methods
//    virtual bool hasBoundaryGradient();
//    virtual bool hasDiffusionModel();
//    virtual bool hasOrderParameters();
//    virtual bool hasReactionModel();
//    virtual bool hasTilings();
//
//    // get protobuf methods
//    virtual lm::io::BoundaryConditions* getBoundaryGradient();
//    virtual lm::io::DiffusionModel* getDiffusionModel();
//    virtual lm::io::SimulationParameters* getParameters();
//    virtual lm::io::OrderParameters* getOrderParameters();
//    virtual lm::io::ReactionModel* getReactionModel();
//    virtual lm::io::SpatialModel* getSpatialModel();
//    virtual lm::io::Tilings* getTilings();
//
//    // get protobuf methods (load-into-pointer style)
//    virtual void getBoundaryGradient(lm::io::BoundaryConditions* bc);
//    virtual void getDiffusionModel(lm::io::DiffusionModel* diffusionModel);
//    virtual void getParameters(lm::io::SimulationParameters* parameters);
//    virtual void getOrderParameters(lm::io::OrderParameters* orderParameters);
//    virtual void getReactionModel(lm::io::ReactionModel* reactionModel);
//    virtual void getSpatialModel(lm::io::SpatialModel* model);
//    virtual void getTilings(lm::io::Tilings* tilings);
//
//    // get wrapper methods
//    virtual map<string,string> getParameters();
//    virtual string getParameter(string key, string defaultValue="");
//    virtual lm::oparam::OParam* getOrderParameter();
//
//    // set methods
//    virtual void setDiffusionModel(lm::io::DiffusionModel* diffusionModel);
//    virtual void setOrderParameters(lm::io::OrderParameters* orderParameters);
//    virtual void setParameter(string key, string value);
//    virtual void setReactionModel(lm::io::ReactionModel* reactionModel);
//    virtual void setSpatialModel(lm::io::SpatialModel* model);
//    virtual void setTilings(lm::io::Tilings* tilings);
//
//protected:
//    // load from file methods
//    virtual void _loadBoundaryGradient(lm::io::BoundaryConditions* bc);
//    virtual void _loadDiffusionModel(lm::io::DiffusionModel* diffusionModel);
//    virtual void _loadParameters(lm::io::SimulationParameters* parameters);
//    virtual void _loadOrderParameters(lm::io::OrderParameters* orderParameters);
//    virtual void _loadReactionModel(lm::io::ReactionModel* reactionModel);
//    virtual void _loadSpatialModel(lm::io::SpatialModel* model);
//    virtual void _loadTilings(lm::io::Tilings* tilings);
//
//private:
//    lm::io::hdf5::Hdf5File& file;
//    lm::message::Message msg;
//};

}
}
