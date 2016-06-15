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
#include <vector>

#include "lm/input/Input.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

namespace lm {
namespace trajectory {

class Trajectory
{
public:
    // NB: any changes made to the enum Status *must* be made also to the array status_strings
    enum Status {ABORTED,
                 FINISHED,
                 NOT_STARTED,
                 RUNNING,
                 WAITING};
    static const std::string status_strings[];

    Trajectory(uint64_t id, uint64_t phase, const lm::io::TrajectoryState& initialState);
    Trajectory(uint64_t id, uint64_t phase, const lm::input::Input& input, bool reversed=false);
    virtual ~Trajectory();

    // accessors
    virtual uint64_t getID() const;
    virtual std::vector<double> getLastOrderParameterValues() const;
    virtual std::vector<int32_t> getLastSpeciesCounts() const;
    virtual const lm::input::TrajectoryLimit& getLimitReached() const;
    virtual const lm::io::OrderParametersValues& getOrderParameterValues() const;
    virtual uint64_t getSimulationPhase() const;
    virtual int32_t getSimSteps() const;
    virtual double getSimTime() const;
    virtual const lm::io::SpeciesCounts& getSpeciesCounts() const;
    virtual const lm::io::TrajectoryState& getState() const;
    virtual Status getStatus() const;
    virtual int64_t getWorkUnitsPerformed() const;
    virtual void printStatus() const;

    // mutators
    virtual void clearLimitReached();
    virtual double* getLastOrderParameterValuesMutable();
    virtual int32_t* getLastSpeciesCountsMutable();
    virtual lm::io::TrajectoryState* getStateMutable();
    virtual void incrementWorkUnitsPerformed();
    virtual void resetSimTime();
    virtual void setID(uint64_t trajectoryID);
    virtual void setLimitReached(const lm::input::TrajectoryLimit& limitBuf);
    virtual void setState(const lm::io::TrajectoryState& newState);
    virtual void setStatus(Status newStatus);

protected:
    virtual void initializeDegreeAdvancements(const lm::input::Input& input);
    virtual void inititializeHists(const lm::input::Input& input);
    virtual void initializeOrderParameters(const lm::input::Input& input);
    virtual void initializeOrderParameterFirstPassageTimes(const lm::input::Input& input);
    virtual void initializeState(const lm::input::Input& input, bool reversed=false);

protected:
    uint64_t id;
    int64_t numberWorkUnitsPerformed;
    uint64_t simulationPhase;
    lm::io::TrajectoryState state;
    Status status;
};

}
}

#endif
