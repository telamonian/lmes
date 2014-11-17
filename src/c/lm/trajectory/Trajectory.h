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
#ifndef LM_TRAJECTORY_TRAJECTORY_H_
#define LM_TRAJECTORY_TRAJECTORY_H_

#include <map>
#include <string>

#include "lm/input/Input.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

namespace lm {
namespace trajectory {

// Trajectory is responsible for RunWorkUnit messages
class Trajectory
{
public:
    enum status_t {NOT_STARTED, RUNNING, WAITING, FINISHED};

//    Trajectory(uint64_t id,const lm::io::ReactionModel& reactionModel,const lm::io::DiffusionModel& diffusionModel,std::map<std::string,std::string>& simulationParameters,lm::tiling::Tilings* tilings,bool reversed=false);
//    Trajectory(uint64_t id,const lm::io::ReactionModel& reactionModel,const lm::io::DiffusionModel& diffusionModel,std::map<std::string,std::string>& simulationParameters,lm::tiling::Tilings* tilings,lm::io::TrajectoryState* zerothState);
    Trajectory(uint64_t id,lm::input::Input& input,bool reversed=false);
    Trajectory(uint64_t id,lm::input::Input& input,lm::io::TrajectoryState* zerothState);
    virtual ~Trajectory();
    virtual void initHists();
    virtual void initMsg(std::map<std::string,std::string>& simulationParameters);
    virtual void initMsg(const lm::message::Message& newMsg);
    virtual void initState(const lm::io::ReactionModel& reactionModel,bool reversed);
    virtual void initState(lm::io::TrajectoryState* zerothState);
//    virtual void initLimits() = 0;

    // accessors
    virtual uint64_t getID();
    virtual lm::io::TrajectoryLimits* getLimits();
    virtual lm::message::Message* getMsg();
    virtual lm::message::Message* getNextWorkUnitMsg(uint64_t nextWorkUnitID);
    virtual lm::message::RunWorkUnit* getRunMsg();
    virtual lm::io::TrajectoryState* getState();
    virtual status_t getStatus();

    // mutators
    virtual void setID(uint64_t id);
    virtual void setLimits(const lm::io::TrajectoryLimits* newLimits);
    virtual void setMsg(const lm::message::Message& newMsg);
    virtual void setStarted(bool trajectoryStarted);
    virtual void setState(const lm::io::TrajectoryState* newState);
    virtual void setStatus(status_t newStatus);
    virtual void setWorkUnitId(uint64_t id);

protected:
    uint64_t id;
    lm::input::Input& input;
    lm::message::Message msg;
//    lm::io::ReactionModel& reactionModel;
//    lm::io::DiffusionModel& diffusionModel;
//    map<string,string> simulationParameters;
//    lm::io::TrajectoryLimits limits;
//    lm::io::TrajectoryState state;  // state is supposed to be synced at all (or at least most) times with the msg.run_work_unit.initial_state field
    status_t status;
};

}
}

typedef std::map<uint64_t, lm::trajectory::Trajectory*> TrajectoryMap;

#endif
