/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *                  Johns Hopkins University
 *                  http://biophysics.jhu.edu/roberts/
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
#ifndef LM_RESOURCE_TRAJECTORY_H_
#define LM_RESOURCE_TRAJECTORY_H_

#include <map>
#include <string>
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/Types.h"

using std::map;
using std::string;


namespace lm {
namespace resource {

class Trajectory
{
public:
    enum status_t {NOT_STARTED, RUNNING, WAITING, FINISHED};

    Trajectory(uint64_t trajectoryID);
    virtual ~Trajectory();
    virtual void initMsg(int supervisorProcess, int supervisorThread, int outputProcess, int outputThread);
    virtual void initMsg(const lm::message::Message& newMsg);
    virtual void initState(const lm::io::ReactionModel& reactionModel);
    virtual void initState(const lm::io::TrajectoryState& initState);

    //getters
    virtual lm::message::Message* getMsg();
    virtual lm::message::RunWorkUnit* getRunMsg();
    virtual status_t getStatus();
    virtual lm::io::TrajectoryState& getState();

    //setters
    virtual void setMsg(const lm::message::Message& newMsg);
    virtual void setWorkUnitId(int64_t id);
    virtual void setStarted(bool trajectoryStarted);
    virtual void setStatus(status_t newStatus);
    virtual void setState(const lm::io::TrajectoryState& newState);

    //other?
    virtual void updateInitialRunState();

    uint64_t id;

protected:
    status_t status;
    lm::io::TrajectoryState state;  // state is supposed to be synced at all (or at least most) times with the msg.run_work_unit.initial_state field
    lm::message::Message msg;
};

}
}

typedef map<uint64_t, lm::resource::Trajectory*> TrajectoryMap;

#endif
