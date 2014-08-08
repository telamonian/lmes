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
	~Trajectory() {};

	//getters
	lm::message::Message* getMsg() {return &msg;}
	lm::message::RunWorkUnit* getRunMsg();
	status_t getStatus() {return status;}
	lm::io::TrajectoryState& getState() {return state;}

	//setters
	void setMsg(const lm::message::Message& newMsg) {msg = newMsg;}
	void setWorkUnitId(int64_t id) {getRunMsg()->set_work_unit_id(id);}
	void setStarted(bool trajectoryStarted) {state.set_trajectory_started(trajectoryStarted);}
	void setStatus(status_t newStatus) {status = newStatus;}
	void setState(const lm::io::TrajectoryState& newState) {state = newState;}

	//other?
	void updateInitialRunState();

	uint64_t trajectoryID;

protected:
	status_t status;
	lm::io::TrajectoryState state;
	lm::message::Message msg;
};

}
}

typedef map<uint64_t, lm::resource::Trajectory*> TrajectoryMap;

#endif
