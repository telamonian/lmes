/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
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

#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/BoundaryConditions.pb.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.pb.h"
#include "lm/io/SpatialModel.pb.h"
#include "lm/input/InputHelper.h"
#include "lm/message/Message.pb.h"
#include "lm/oparam/OParams.h"
#include "lm/tiling/Tilings.h"

namespace lm {
namespace input {

// this should be called after rFFDiffusionModelBuf
bool InputHelper::rFFBoundaryConditionsBuf(lm::io::hdf5::Hdf5File* file, lm::io::DiffusionModel* diffusionModelBuf, lm::main::SimulationParametersMap* simulationParametersMap)
{
    // See if we need to fill in the boundary conditions from the simulation parameters.
    if (simulationParametersMap->count("boundaryConditions") == 1 && !diffusionModelBuf->has_boundary_conditions())
    {
        lm::io::BoundaryConditions* bc=diffusionModelBuf->mutable_boundary_conditions();
        if (!parseBoundaryConditions(bc, (*simulationParametersMap)["boundaryConditions"].c_str()))
        {
            throw Exception("Could not parse boundaryConditions parameter",(*simulationParametersMap)["boundaryConditions"].c_str());
        }
        if (simulationParametersMap->count("boundarySite") == 1)
        {
            bc->set_boundary_site(atoi((*simulationParametersMap)["boundarySite"].c_str()));
        }
        if (simulationParametersMap->count("boundarySpecies") == 1)
        {
            bc->set_boundary_species(atoi((*simulationParametersMap)["boundarySpecies"].c_str()));
        }
        if (simulationParametersMap->count("boundaryConcentration") == 1)
        {
            bc->set_boundary_concentration(atof((*simulationParametersMap)["boundaryConcentration"].c_str()));
        }
        if (file->hasBoundaryGradient())
        {
            file->getBoundaryGradient(bc);
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool InputHelper::rFFDiffusionModelBuf(lm::io::hdf5::Hdf5File* file, lm::io::DiffusionModel* diffusionModelBuf, lm::main::SimulationParametersMap* simulationParametersMap)
{
    if (file->hasDiffusionModel())
    {
        file->getDiffusionModel(diffusionModelBuf);
        rFFBoundaryConditionsBuf(file,diffusionModelBuf,simulationParametersMap);
        return true;
    }
    else
    {
        return false;
    }
}

bool InputHelper::rFFOrderParameters(lm::io::hdf5::Hdf5File* file, lm::oparam::OParams* oparams, lm::main::SimulationParametersMap* simulationParametersMap)
{
    if (file->hasOrderParameters())
    {
        oparams->init(file);
        return true;
    }
    else
    {
        return false;
    }
}

bool InputHelper::rFFOrderParametersBuf(lm::io::hdf5::Hdf5File* file, lm::io::OrderParameters* orderParametersBuf, lm::main::SimulationParametersMap* simulationParametersMap)
{
    if (file->hasOrderParameters())
    {
        file->getOrderParameters(orderParametersBuf);
        return true;
    }
    else
    {
        return false;
    }
}

bool InputHelper::rFFReactionModelBuf(lm::io::hdf5::Hdf5File* file, lm::io::ReactionModel* reactionModelBuf, lm::main::SimulationParametersMap* simulationParametersMap)
{
    if (file->hasReactionModel())
    {
        file->getReactionModel(reactionModelBuf);
        return true;
    }
    else
    {
        return false;
    }
}

bool InputHelper::rFFSimulationParametersBuf(lm::io::hdf5::Hdf5File* file, lm::io::SimulationParameters* simulationParametersBuf)
{
    file->getParameters(simulationParametersBuf);
    return true;
}

bool InputHelper::rFFSimulationParametersMap(lm::io::hdf5::Hdf5File* file, lm::main::SimulationParametersMap* simulationParametersMap)
{
    lm::io::SimulationParameters simulationParametersBuf;
    InputHelper::rFFSimulationParametersBuf(file, &simulationParametersBuf);
    for (int i=0; i<simulationParametersBuf.key_size() && i<simulationParametersBuf.value_size(); i++)
    {
        (*simulationParametersMap)[simulationParametersBuf.key(i)] = simulationParametersBuf.value(i);
    }
    return true;
}

bool InputHelper::rFFTilings(lm::io::hdf5::Hdf5File* file, lm::tiling::Tilings* tilings, lm::main::SimulationParametersMap* simulationParametersMap)
{
    if (file->hasTilings())
    {
        tilings->init(file);
        return true;
    }
    else
    {
        return false;
    }
}

bool InputHelper::rFFTilingsBuf(lm::io::hdf5::Hdf5File* file, lm::io::Tilings* tilingsBuf, lm::main::SimulationParametersMap* simulationParametersMap)
{
    if (file->hasTilings())
    {
        file->getTilings(tilingsBuf);
        return true;
    }
    else
    {
        return false;
    }
}

lm::input::Input* InputHelper::rFFInput(lm::io::hdf5::Hdf5File* file, lm::io::DiffusionModel* dMB, lm::oparam::OParams* ops, lm::io::ReactionModel* rMB, lm::main::SimulationParametersMap* sPM, lm::tiling::Tilings* tngs)
{
    bool hasDMB,hasOPs,hasRMB,hasTngs;
    rFFSimulationParametersMap(file, sPM);
    hasDMB = rFFDiffusionModelBuf(file, dMB, sPM);
    hasOPs = rFFOrderParameters(file, ops, sPM);
    hasRMB = rFFReactionModelBuf(file, rMB, sPM);
    hasTngs = rFFTilings(file, tngs, sPM);
    return new lm::input::Input(hasDMB,hasOPs,hasRMB,hasTngs,*dMB,*ops,*rMB,*sPM,*tngs);
}

bool InputHelper::parseBoundaryConditions(lm::io::BoundaryConditions* bc, std::string arg)
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

}
}
