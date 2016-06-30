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
#ifndef LM_REPLICATES_REPLICATETRAJECTORYLIST_H_
#define LM_REPLICATES_REPLICATETRAJECTORYLIST_H_

#include <map>
#include <string>

#include "hrtime.h"
#include "lm/input/Input.h"
#include "lm/input/DiffusionModel.pb.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/trajectory/TrajectoryList.h"
#include "lm/Types.h"

using std::map;
using lm::trajectory::TrajectoryList;

namespace lm {
namespace replicates {

class ReplicateTrajectoryList : public lm::trajectory::TrajectoryList
{

public:
    ReplicateTrajectoryList(const lm::input::Input& input, uint64_t firstTrajectory, uint64_t lastTrajectory);
    virtual ~ReplicateTrajectoryList();
    virtual void workUnitFinished(const lm::message::FinishedWorkUnit& msg);

protected:
    virtual void printTrajectoryStatistics() const;
    virtual uint64_t findNextTrajectoryToRun() const;

protected:
    uint64_t firstTrajectory;
    uint64_t lastTrajectory;

private:
    mutable hrtime stats_lastPrintTime;
};

}
}

#endif
