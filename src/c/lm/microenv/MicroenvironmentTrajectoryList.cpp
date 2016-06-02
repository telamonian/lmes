/*
 * Copyright 2016 Johns Hopkins University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
 *
 * Author(s): Elijah Roberts
 */

#include <limits>
#include <list>
#include <map>
#include <string>
#include "hrtime.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/input/Input.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/microenv/MicroenvironmentTrajectoryList.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/trajectory/TrajectoryList.h"

using std::map;
using std::string;

namespace lm {
namespace microenv {

MicroenvironmentTrajectoryList::MicroenvironmentTrajectoryList(const lm::input::Input& input, uint64_t replicate)
:replicate(replicate), stats_lastPrintTime(getHrTime())
{
    /*for (uint64_t i=firstTrajectory; i<=lastTrajectory; i++)
    {
        trajectories[i] = new lm::trajectory::Trajectory(i, getSimulationPhase(), input);
        waitingTrajectories[i] = trajectories[i];
    }*/
}

MicroenvironmentTrajectoryList::~MicroenvironmentTrajectoryList()
{
}

void MicroenvironmentTrajectoryList::workUnitFinished(const lm::message::FinishedWorkUnit& msg)
{
    //Call the base class method.
    TrajectoryList::workUnitFinished(msg);

    // Print out a message for any trajectories that finished.
    for (int i=0; i<msg.part_status_size(); i++)
    {
        uint64_t id = msg.part_status(i).final_state().trajectory_id();
        lm::trajectory::Trajectory* t = trajectories[id];
        if (t->getStatus() == lm::trajectory::Trajectory::FINISHED)
        {
            Print::printf(Print::INFO, "Replicate %lld completed with %8.2e of simulation time using %d work units.",
                          t->getID(), t->getState().cme_state().species_counts().time(0), t->getWorkUnitsPerformed());
        }
    }
}

uint64_t MicroenvironmentTrajectoryList::findNextTrajectoryToRun() const
{
    uint64_t minId=UINT64_MAX;
    double minTime=std::numeric_limits<double>::infinity();
    for (TrajectoryMap::const_iterator it=waitingTrajectories.begin(); it!=waitingTrajectories.end(); it++)
    {
        lm::trajectory::Trajectory* t = it->second;
        double time = t->getState().cme_state().species_counts().time(0);
        if (time < minTime)
        {
            minTime = time;
            minId = it->first;
        }
    }

    if (minId == UINT64_MAX)
    {
        for (TrajectoryMap::const_iterator it=waitingTrajectories.begin(); it!=waitingTrajectories.end(); it++)
        {
            it->second->getState().PrintDebugString();
        }
        throw Exception("Consistency error in MicroenvironmentTrajectoryList, no next trajectory found",minId,waitingTrajectories.size());
    }

    return minId;
}

void MicroenvironmentTrajectoryList::printTrajectoryStatistics() const
{
    // Print some performance statistics, if it has been a while.
    hrtime currentTime = getHrTime();
    if (convertHrToSeconds(currentTime-stats_lastPrintTime) > 700.0)        
    {
        const std::string statusStrings[] = {"ABORTED", "FINISHED", "NOT_STARTED", "RUNNING", "WAITING"};
        Print::printf(Print::INFO, "Trajectory status");
        Print::printf(Print::INFO, "        ID State       Time     Work_Units");
        Print::printf(Print::INFO, "------------------------------------------");
        for (TrajectoryMap::const_iterator it=trajectories.begin(); it!=trajectories.end(); it++)
        {
            uint64_t id = it->first;
            lm::trajectory::Trajectory* t = it->second;
            Print::printf(Print::INFO, "%10lld %-11s %8.2e %10d", id, statusStrings[(int)t->getStatus()].c_str(), t->getState().cme_state().species_counts().time(0), t->getWorkUnitsPerformed());
        }
        stats_lastPrintTime = getHrTime();
    }
}

}
}
