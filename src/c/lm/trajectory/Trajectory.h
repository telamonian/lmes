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
#include "lm/io/TrajectoryState.pb.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

namespace lm {
namespace trajectory {

class Trajectory
{
public:
    enum status_t {NOT_STARTED, RUNNING, WAITING, FINISHED};

    Trajectory(uint64_t id, const lm::io::TrajectoryState& initialState);
    Trajectory(uint64_t id, const lm::input::Input& input, bool reversed=false);
    virtual ~Trajectory();

    // accessors
    virtual uint getFinalLimitID();
    virtual uint64_t getId();
    virtual double getOrderParameterValue(uint opID);
    virtual uint getSimSteps();
    virtual double getSimTime();
    virtual lm::io::SpeciesCounts* getSpeciesCounts();
    virtual const lm::io::TrajectoryState& getState();
    virtual status_t getStatus();
    virtual int64_t getWorkUnitsPerformed();
    virtual void printStatus();

    // mutators
    virtual int64_t getWorkUnitsPerformed();
    virtual void incrementWorkUnitsPerformed();
    virtual void resetSimTime();
    virtual void setFinalLimitID(int64_t finalLimitID);
    virtual void setState(const lm::io::TrajectoryState& newState);
    virtual void setStatus(status_t newStatus);

protected:
    virtual void inititializeHists();
    virtual void initializeState(const lm::input::Input& input, bool reversed=false);
    virtual void initializeState(const lm::io::TrajectoryState& initialState);

protected:
    uint64_t id;
    status_t status;
    lm::io::TrajectoryState state;
    int64_t numberWorkUnitsPerformed;
};

}
}

#endif
