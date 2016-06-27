/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
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
#include <limits>
#include <list>
#include <map>
#include <string>
#include "hrtime.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/input/Input.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/replicates/ReplicateTrajectoryList.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/trajectory/TrajectoryList.h"

#ifndef UINT64_MAX
#define UINT64_MAX        18446744073709551615ULL
#endif

using lm::trajectory::TrajectoryMap;
using std::map;
using std::string;

namespace lm {
namespace replicates {

ReplicateTrajectoryList::ReplicateTrajectoryList(const lm::input::Input& input, uint64_t firstTrajectory, uint64_t lastTrajectory)
:firstTrajectory(firstTrajectory), lastTrajectory(lastTrajectory), stats_lastPrintTime(getHrTime())
{
    for (uint64_t i=firstTrajectory; i<=lastTrajectory; i++)
    {
        trajectories[i] = new lm::trajectory::Trajectory(input, i, getSimulationPhaseIndex());
        waitingTrajectories[i] = trajectories[i];
    }
}

ReplicateTrajectoryList::~ReplicateTrajectoryList()
{
}

void ReplicateTrajectoryList::workUnitFinished(const lm::message::FinishedWorkUnit& msg)
{
    // Call the base class method.
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

uint64_t ReplicateTrajectoryList::findNextTrajectoryToRun() const
{
    uint64_t minId = UINT64_MAX;
    double minTime = std::numeric_limits<double>::infinity();
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
        throw Exception("Consistency error in ReplicateTrajectoryList, no next trajectory found",minId,waitingTrajectories.size());
    }

    return minId;
}

void ReplicateTrajectoryList::printTrajectoryStatistics() const
{
    // Print some performance statistics, if it has been a while.
    hrtime currentTime = getHrTime();
    if (convertHrToSeconds(currentTime-stats_lastPrintTime) > 700.0)
    {
        Print::printf(Print::INFO, "Trajectory status");
        Print::printf(Print::INFO, "        ID State       Time     Work_Units");
        Print::printf(Print::INFO, "------------------------------------------");
        for (TrajectoryMap::const_iterator it=trajectories.begin(); it!=trajectories.end(); it++)
        {
            uint64_t id = it->first;
            lm::trajectory::Trajectory* t = it->second;
            Print::printf(Print::INFO, "%10lld %-11s %8.2e %10d", id, lm::trajectory::Trajectory::status_strings[(int)t->getStatus()].c_str(), t->getState().cme_state().species_counts().time(0), t->getWorkUnitsPerformed());
        }
        stats_lastPrintTime = getHrTime();
    }
}

}
}
