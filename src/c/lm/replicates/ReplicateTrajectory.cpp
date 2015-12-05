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
#include <csignal>
#include <list>
#include <map>
#include <cstdio>
#include <string>
#include "lm/io/Tilings.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/option/SimulationParameters.h"
#include "lm/Print.h"
#include "lm/replicates/ReplicateTrajectory.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

using lm::io::DiffusionModel;
using lm::io::ReactionModel;
using lm::io::TrajectoryState;
using std::map;
using std::string;

namespace lm {
namespace replicates {

//ReplicateTrajectory::ReplicateTrajectory(uint64_t id,const ReactionModel& reactionModel,const DiffusionModel& diffusionModel,map<string,string>& simulationParameters):
//Trajectory(id,reactionModel,diffusionModel,simulationParameters)
//{
//    // Limit setting code
//    initLimits(reactionModel,simulationParameters);
//}
//ReplicateTrajectory::ReplicateTrajectory(uint64_t id,const ReactionModel& reactionModel,const DiffusionModel& diffusionModel,map<string,string>& simulationParameters,TrajectoryState* zerothState):
//Trajectory(id,reactionModel,diffusionModel,simulationParameters,zerothState)
//{
//    // Limit setting code
//    initLimits(reactionModel,simulationParameters);
//}

ReplicateTrajectory::ReplicateTrajectory(uint64_t id, lm::input::Input& input):
Trajectory(id, input)
{
    // Limit setting code
    initLimits(input.reactionModelBuf,input.simulationParameters);
}

ReplicateTrajectory::ReplicateTrajectory(uint64_t id,lm::input::Input& input, TrajectoryState* zerothState):
Trajectory(id,input,zerothState)
{
    // Limit setting code
    initLimits(input.reactionModelBuf,input.simulationParameters);
}

ReplicateTrajectory::~ReplicateTrajectory()
{
}

void ReplicateTrajectory::initLimits(const ReactionModel& reactionModel, lm::option::SimulationParameters& simulationParameters)
{
    // See if we have a max time limit.
    if (simulationParameters.count("maxTime"))
        getLimits()->set_max_time(atof(simulationParameters["maxTime"].c_str()));

    // Set the species lower limits from the parameters.
    if (simulationParameters.count("speciesLowerLimitList"))
    {
        for (int i=0; i<(int)reactionModel.number_species(); i++)
            getLimits()->add_min_species_count(-1);

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
                int parsedSpecies = atoi(speciesLowerLimit.substr(0, equalsPos).c_str());
                int parsedLimit = atoi(speciesLowerLimit.substr(equalsPos+1, string::npos).c_str());
                getLimits()->set_min_species_count(parsedSpecies, parsedLimit);
                Print::printf(Print::DEBUG, "Parsed lower limit %s to: %d => %d", speciesLowerLimit.c_str(), parsedSpecies, parsedLimit);
            }
            start = end+1;
        }
    }

    // Set the species upper limits from the parameters.
    if (simulationParameters.count("speciesUpperLimitList"))
    {
        for (int i=0; i<(int)reactionModel.number_species(); i++)
            getLimits()->add_max_species_count(-1);

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
                getLimits()->set_max_species_count(parsedSpecies, parsedLimit);
                Print::printf(Print::DEBUG, "Parsed upper limit %s to: %d <= %d", speciesUpperLimit.c_str(), parsedSpecies, parsedLimit);
            }
            start = end+1;
        }
    }

    // Set the order parameter lower limits from the parameters.
    if (simulationParameters.count("orderParameterLowerLimitList"))
    {
        string listString = simulationParameters["orderParameterLowerLimitList"];
        size_t start=0, end=0;
        while (end != string::npos)
        {
            end = listString.find(',', start);
            string speciesLowerLimit = listString.substr(start, (end == string::npos) ? string::npos : end - start);

            size_t equalsPos=0;
            equalsPos = speciesLowerLimit.find(':', 0);
            if (equalsPos > 0 && equalsPos < speciesLowerLimit.length()-1)
            {
                int parsedOParamID = atoi(speciesLowerLimit.substr(0, equalsPos).c_str());
                int parsedLimit = atoi(speciesLowerLimit.substr(equalsPos+1, string::npos).c_str());
                lm::io::TrajectoryLimits::DecreasingOrderParameterLimit* dopl = getLimits()->add_decreasing_order_parameter_limit();
                dopl->set_arrangement(lm::io::TrajectoryLimits::DESCENDING);
                dopl->set_limit_id(0);
                dopl->set_order_parameter_id(parsedOParamID);
                dopl->add_value(parsedLimit);
                Print::printf(Print::INFO, "Parsed order parameter lower limit %s to: %d => %d", speciesLowerLimit.c_str(), parsedOParamID, parsedLimit);
            }
            start = end+1;
        }
    }

    // Set the order parameter upper limits from the parameters.
    if (simulationParameters.count("orderParameterUpperLimitList"))
    {
        string listString = simulationParameters["orderParameterUpperLimitList"];
        size_t start=0, end=0;
        while (end != string::npos)
        {
            end = listString.find(',', start);
            string speciesLowerLimit = listString.substr(start, (end == string::npos) ? string::npos : end - start);

            size_t equalsPos=0;
            equalsPos = speciesLowerLimit.find(':', 0);
            if (equalsPos > 0 && equalsPos < speciesLowerLimit.length()-1)
            {
                int parsedOParamID = atoi(speciesLowerLimit.substr(0, equalsPos).c_str());
                int parsedLimit = atoi(speciesLowerLimit.substr(equalsPos+1, string::npos).c_str());
                lm::io::TrajectoryLimits::IncreasingOrderParameterLimit* iopl = getLimits()->add_increasing_order_parameter_limit();
                iopl->set_arrangement(lm::io::TrajectoryLimits::ASCENDING);
                iopl->set_limit_id(0);
                iopl->set_order_parameter_id(parsedOParamID);
                iopl->add_value(parsedLimit);
                Print::printf(Print::INFO, "Parsed order parameter upper limit %s to: %d => %d", speciesLowerLimit.c_str(), parsedOParamID, parsedLimit);
            }
            start = end+1;
        }
    }
}

}
}
