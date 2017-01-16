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

#include "lm/EnumHelper.h"
#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/input/Input.h"
#include "lm/input/SimulationParametersWrap.h"
#include "lm/input/OutputOptions.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/sfile/LocalSFile.h"
#include "lm/io/sfile/SFile.h"
#include "lm/io/sfile/SFileRecord.h"
#include "lm/trajectory/TrajectoryLimits.h"
#include "lm/Types.h"
#include "robertslab/pbuf/NDArray.pb.h"
#include "robertslab/pbuf/NDArraySerializer.h"

using lm::input::OutputOptions;
using lm::trajectory::LimitValueT;
using std::map;
using std::string;

namespace lm {
namespace input {

Input::Input(vector<string> inputFilenames)
:reactionModelPresent(false),diffusionModelPresent(false),orderParametersPresent(false),tilingsPresent(false),trajectoryLimitsPresent(false),
 outputOptionsPresent(false),simulationParameters(),partsPerWorkUnit(1),stepsPerWorkUnit(10000000)
{
    for (int i=0; i<inputFilenames.size(); i++)
    {
        // See if the file is an HDF5 file.
        if (lm::io::hdf5::Hdf5File::isValidFile(inputFilenames[i]))
        {
            lm::io::hdf5::Hdf5File file = lm::io::hdf5::Hdf5File(inputFilenames[i]);
            readHDF5Input(file);
        }

        // See if the file is an SFile.
        lm::io::sfile::LocalSFile sfile(inputFilenames[i]);
        if(sfile.exists() && sfile.isFile() && sfile.isSFile())
        {
            // Read the input from the sfile.
            sfile.openRead();
            readSFileInput(sfile);
            sfile.close();
        }
    }
}

Input::~Input()
{
}

void Input::readHDF5Input(lm::io::hdf5::Hdf5File& file)
{
    // Get any generic simulation parameters.
    {
        lm::input::SimulationParameters p;
        file.getParameters(&p);
        simulationParameters.set(p);
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
            lm::types::BoundaryConditions* bc=diffusionModel.mutable_boundary_conditions();
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
            trajectoryLimits.addLimitBuf<EH::TIME>(0, simulationParameters.parse<double>("maxTime"), EH::MAX);
            trajectoryLimitsPresent |= true;
        }

        // set the other limits, if present in the simulation parameters
        trajectoryLimitsPresent |= parseLimits<EH::DEGREE_ADVANCEMENT>("degreeAdvancementLowerLimitList", "degree advancement lower limit", EH::MIN);
        trajectoryLimitsPresent |= parseLimits<EH::DEGREE_ADVANCEMENT>("degreeAdvancementUpperLimitList", "degree advancement upper limit", EH::MAX);
        trajectoryLimitsPresent |= parseLimits<EH::ORDER_PARAMETER>("orderParameterLowerLimitList", "order parameter lower limit", EH::MIN);
        trajectoryLimitsPresent |= parseLimits<EH::ORDER_PARAMETER>("orderParameterUpperLimitList", "order parameter upper limit", EH::MAX);
        trajectoryLimitsPresent |= parseLimits<EH::SPECIES>("speciesLowerLimitList", "species lower limit", EH::MIN);
        trajectoryLimitsPresent |= parseLimits<EH::SPECIES>("speciesUpperLimitList", "species upper limit", EH::MAX);
    }

    // Get the output options.
    {
        if (simulationParameters.count("degreeAdvancementWriteInterval"))
        {
            outputOptionsPresent = true;
            outputOptions.set_degree_advancement_write_interval(atof(simulationParameters["degreeAdvancementWriteInterval"].c_str()));
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

        if (simulationParameters.count("latticeWriteInterval"))
        {
            outputOptionsPresent = true;
            outputOptions.set_lattice_write_interval(atof(simulationParameters["latticeWriteInterval"].c_str()));
        }

        if (simulationParameters.count("orderParameterWriteInterval"))
        {
            outputOptionsPresent = true;
            outputOptions.set_order_parameter_write_interval(simulationParameters.parse<double>("orderParameterWriteInterval"));
        }
        
        if (simulationParameters.count("writeInterval"))
        {
            outputOptionsPresent = true;
            outputOptions.set_species_write_interval(atof(simulationParameters["writeInterval"].c_str()));
            outputOptions.set_concentrations_write_interval(atof(simulationParameters["writeInterval"].c_str()));
        }
    }

    // Get some generic input options.
    if (simulationParameters.count("partsPerWorkUnit"))
        partsPerWorkUnit = simulationParameters.parse<uint64_t>("partsPerWorkUnit");

    if (simulationParameters.count("maxWorkUnitSteps"))
        stepsPerWorkUnit = atoll(simulationParameters["maxWorkUnitSteps"].c_str());
}

void Input::readSFileInput(lm::io::sfile::SFile& file)
{
    // Read all of the records.
    while (!file.isEof())
    {
        lm::io::sfile::SFileRecord r = file.readNextSFileRecord();

        // See if this is an input record.
        if (r.type == "protobuf:lm.input.SimulationInput")
        {
            // Allocate a buffer.
            char* buffer = new char[r.dataSize];

            // Read the record.
            file.readFully(buffer, r.dataSize);

            // Parse the record.
            lm::input::SimulationInput newInput;
            if (!newInput.ParseFromArray(buffer, r.dataSize)) throw RuntimeException("unable to deserialize simulation input");

            // Merge this record into the global input record.
            input.MergeFrom(newInput);

            // Release the buffer.
            delete[] buffer;
        }
        else
        {
            // Skip the record.
            file.skip(r.dataSize);
        }
    }
}

bool Input::parseBoundaryConditions(lm::types::BoundaryConditions* bc, string arg)
{
    lm::types::BoundaryConditions::BoundaryConditionsType type;

    // See if it is a global boundary condition.
    if (lm::types::BoundaryConditions_BoundaryConditionsType_Parse(arg, &type))
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
            if (!lm::types::BoundaryConditions_BoundaryConditionsType_Parse(std::string(pch+2), &type))
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
            if (!lm::types::BoundaryConditions_BoundaryConditionsType_Parse(std::string(pch+3), &type))
            {
                delete[] argbuf;
                return false;
            }

            // Set the axis value.
            pch[2] = '\0';
            std::string axis=pch;
            if (axis == "+x" && type != lm::types::BoundaryConditions::PERIODIC)
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_x_plus(type);
            }
            else if (axis == "-x" && type != lm::types::BoundaryConditions::PERIODIC)
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_x_minus(type);
            }
            else if (axis == "+y" && type != lm::types::BoundaryConditions::PERIODIC)
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_y_plus(type);
            }
            else if (axis == "-y" && type != lm::types::BoundaryConditions::PERIODIC)
            {
                bc->set_axis_specific_boundaries(true);
                bc->set_y_minus(type);
            }
            else if (axis == "+z" && type != lm::types::BoundaryConditions::PERIODIC)
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

template <EH::LimitType LT> bool Input::parseLimits(string key, string debugString, EH::StoppingCondition sc, bool includeEndpoint)
{
    if (simulationParameters.count(key))
    {
        typename pairVector<uint, typename LimitValueT<LT>::type>::type idLimitVec(simulationParameters.parsePairVector<uint, typename LimitValueT<LT>::type>(key, debugString));
        for (typename pairVector<uint, typename LimitValueT<LT>::type>::iterator it(idLimitVec.begin());
             it != idLimitVec.end(); it++)
        {
            trajectoryLimits.addLimitBuf<LT>(it->first, it->second, sc, includeEndpoint);
        }
        return idLimitVec.size() > 0;
    }
    else
    {
        return false;
    }
}

}
}
