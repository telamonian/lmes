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

#ifndef LM_REPLICATES_TRAJECTORYLIST_H_
#define LM_REPLICATES_TRAJECTORYLIST_H_

#include <map>
#include <string>

#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/TrajectoryState.pb.h"

using std::map;

namespace lm {
namespace replicates {

class TrajectoryList
{
public:
    enum status_t {NOT_STARTED, RUNNING, WAITING, FINISHED};

protected:
    class TrajectoryStatus
    {
    public:
        TrajectoryStatus(int trajectoryNumber) : trajectoryNumber(trajectoryNumber),status(NOT_STARTED) {}
        int trajectoryNumber;
        status_t status;
        lm::io::TrajectoryState state;
    };

public:
    TrajectoryList(int firstTrajectory, int lastTrajectory, map<std::string,std::string>& simulationParameters, const lm::io::ReactionModel& reactionModel, const lm::io::DiffusionModel& diffusionModel);
    virtual ~TrajectoryList();
    virtual int nextTrajectoryToRun();
    virtual status_t getTrajectoryStatus(int trajectory);
    virtual void updateTrajectoryStatus(int trajectory, status_t status);
    virtual const lm::io::TrajectoryState& getTrajectoryState(int trajectory);
    virtual void updateTrajectoryState(int trajectory, const lm::io::TrajectoryState& state);

protected:
    map<int,TrajectoryStatus*> trajectories;
};

}
}

#endif
