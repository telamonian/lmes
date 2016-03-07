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

    Trajectory(uint64_t id, uint64_t phase, const lm::io::TrajectoryState& initialState);
    Trajectory(uint64_t id, uint64_t phase, const lm::input::Input& input, bool reversed=false);
    virtual ~Trajectory();

    // accessors
    virtual uint64_t getID() const;
    virtual const lm::io::TrajectoryLimits::TrajectoryLimit& getLimitReached() const;
    virtual const lm::io::OrderParametersValues& getOrderParameterValues() const;
    virtual uint64_t getPhase() const;
    virtual int32_t getSimSteps() const;
    virtual double getSimTime() const;
    virtual const lm::io::SpeciesCounts& getSpeciesCounts() const;
    virtual const lm::io::TrajectoryState& getState() const;
    virtual status_t getStatus() const;
    virtual int64_t getWorkUnitsPerformed() const;
    virtual void printStatus() const;

    // mutators
    virtual void incrementWorkUnitsPerformed();
    virtual void resetSimTime();
    virtual void setID(uint64_t trajectoryID);
    virtual void setLimitReached(const lm::io::TrajectoryLimits::TrajectoryLimit& limitBuf);
    virtual void setPhase(uint64_t newPhase);
    virtual void setState(const lm::io::TrajectoryState& newState);
    virtual void setStatus(status_t newStatus);

protected:
    virtual void inititializeHists(const lm::input::Input& input);
    virtual void initializeState(const lm::input::Input& input, bool reversed=false);

protected:
    uint64_t id;
    int64_t numberWorkUnitsPerformed;
    uint64_t phase;
    lm::io::TrajectoryState state;
    status_t status;
};

}
}

#endif
