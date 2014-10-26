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
#include "lm/replicates/ReplicateTrajectory.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

namespace lm {
namespace replicates {

ReplicateTrajectory::ReplicateTrajectory(uint64_t id, lm::message::Message trajectoryTemplateMsg, lm::io::TrajectoryState* state, lm::tiling::Tilings& tilings, uint ffluxPhase): //TODO: make this signature less terrible
Trajectory(id)
{
    // Initialize the trajectory's Message msg, TrajectoryState state, and Tilings tilings fields
    setMsg(trajectoryTemplateMsg);

    // Make instance local copies of the supervisor's state
    setState(*state);

    // Set the trajectory id in the trajectory state.
    getState().set_trajectory_id(id);

    // Set the trajectory id in the CME state of the trajectory state (if applicable).
    if (getState().has_cme_state())
        getState().mutable_cme_state()->mutable_species_counts()->set_trajectory_id(id);

    // Set the trajectory id in the RDME state of the trajectory state (if applicable).
//        if (trajectories[id]->getState().has_rdme_state())
//            trajectories[id]->getState().mutable_rdme_state()->mutable_species_counts()->set_trajectory_id(id);

    // Limit setting code
    setLimits();
}

ReplicateTrajectory::~ReplicateTrajectory()
{
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
