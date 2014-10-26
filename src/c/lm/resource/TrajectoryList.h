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
#ifndef LM_RESOURCE_TRAJECTORYLIST_H_
#define LM_RESOURCE_TRAJECTORYLIST_H_

#include <map>
#include <string>

#include "lm/io/ReactionModel.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/resource/Trajectory.h"
#include "lm/Types.h"

using std::map;
using std::string;

namespace lm {
namespace resource {

class TrajectoryList
{
public:
    TrajectoryList(const lm::io::ReactionModel& reactionModel,const lm::io::DiffusionModel& diffusionModel, map<string,string>& simulationParameters);
    virtual ~TrajectoryList();
    virtual void init()=0;

    // getter
    virtual lm::resource::Trajectory* getTrajectory(uint64_t trajectoryID);
    virtual lm::resource::Trajectory::status_t getTrajectoryStatus(uint64_t trajectoryID);
    virtual const lm::io::TrajectoryState& getTrajectoryState(uint64_t trajectoryID);
    virtual bool exists(uint64_t trajectoryID) {if (trajectories.find(trajectoryID)!=trajectories.end()) return true; else return false;}

    // setter
    virtual void setTrajectoryStarted(uint64_t trajectoryID, bool trajectoryStarted);
    virtual void setTrajectoryStatus(uint64_t trajectoryID, lm::resource::Trajectory::status_t status);
    virtual void setTrajectoryState(uint64_t trajectoryID, const lm::io::TrajectoryState& state);

    // destroyer
    virtual void deleteTrajectory(uint64_t trajectoryID);
    virtual void deleteAllTrajectories();

    virtual lm::message::Message* getNextWorkUnitMsg();
    virtual bool isFinished();
    virtual lm::resource::Trajectory* workUnitFinished(const lm::message::FinishedWorkUnit & msg);

    // dealing with the internal template Message methods
//    virtual lm::message::RunWorkUnit* getRunMsg() {return trajectoryTemplateMsg.mutable_run_work_unit();}
//    virtual lm::io::TrajectoryLimits* getLimitsMsg() {return getRunMsg()->mutable_limits();}

protected:
    const lm::io::ReactionModel& reactionModel;
    const lm::io::DiffusionModel& diffusionModel;
    map<string,string>& simulationParameters;
    uint64_t trajectoryCount;
    int64_t workUnitCount;
    // this template message is used when initializing new Trajectory instances
    //lm::message::Message trajectoryTemplateMsg;
    TrajectoryMap trajectories;
};

}
}

#endif
