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

#ifndef LM_RESOURCE_TRAJECTORYLIST_H_
#define LM_RESOURCE_TRAJECTORYLIST_H_

#include <map>
#include <string>
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/resource/Trajectory.h"

using std::map;
using std::string;

typedef map<int, lm::resource::Trajectory *> TrajectoryMap;

namespace lm {
namespace resource {

class TrajectoryList
{
public:
//    virtual int nextTrajectoryToRun()=0;
    TrajectoryList(map<string,string>& simulationParameters, const lm::io::ReactionModel& reactionModel);
    virtual ~TrajectoryList();
    virtual Trajectory::status_t getTrajectoryStatus(int trajectory);

    virtual lm::message::Message * getNextWorkUnitMsg();
    virtual bool workUnitStarted(const lm::message::StartedWorkUnit & msg);
    virtual void workUnitFinished(const lm::message::FinishedWorkUnit & msg);
    virtual const lm::io::TrajectoryState& getTrajectoryState(int trajectory);

    // dealing with the internal template Message methods
	virtual lm::message::RunWorkUnit * getRunWorkUnitMsg() {return trajectoryTemplateMsg.mutable_run_work_unit();}

protected:
    virtual void updateTrajectoryStatus(int trajectory, Trajectory::status_t status);
    virtual void updateTrajectoryState(int trajectory, const lm::io::TrajectoryState& state);
    long long trajectoryCount;
    long long workUnitCount;
    map<string,string>& simulationParameters;
    const lm::io::ReactionModel& reactionModel;
    lm::message::Message trajectoryTemplateMsg;
    TrajectoryMap trajectories;

};

}
}

#endif
