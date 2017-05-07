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
 * Author(s): Max Klein
 */
#ifndef LM_FFLUX_FFLUXTRAJECTORY_H_
#define LM_FFLUX_FFLUXTRAJECTORY_H_

#include <algorithm>

#include "lm/EnumHelper.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/input/Input.h"
#include "lm/limit/LimitCheckFunctions.h"
#include "lm/Stats.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

namespace lm {
namespace fflux {

class FFluxTrajectory : public lm::trajectory::Trajectory
{
public:
    FFluxTrajectory(const lm::input::Input& input, uint64_t phase, uint64_t id)
    :Trajectory(input, phase, id)
    {
    }

    template <typename InputIterator> FFluxTrajectory(const lm::input::Input& input, InputIterator speciesStart, InputIterator speciesEnd, double startTime, uint64_t phase, uint64_t id)
    :Trajectory(input, speciesStart, speciesEnd, startTime, phase, id)
    {
    }

    FFluxTrajectory(const lm::io::TrajectoryState& initialState, uint64_t phase, uint64_t id)
    :Trajectory(initialState, phase, id)
    {
    }

    virtual ~FFluxTrajectory()
    {
    }

    virtual void processState(const lm::io::TrajectoryState& trajectoryState)
    {
        // set wrapper on the limit_trackings field
        limitTrackingsWrap.setWrappedField(trajectoryState.limit_tracking_list().limit_trackings());

        // consistency checks
//        if (limitTrackingsWrap.size()!=2) throw ConsistencyException("Finished Forward Flux phase n>0 trajectories should have 2 tracked limits in their outputs; trajectory id %llu has %d", trajectoryState.trajectory_id(), limitTrackingsWrap.size());
//        for (int i=0;i<2;i++) {if (limitTrackingsWrap.Get(i).limit_id()!=i) throw ConsistencyException("Finished Forward Flux phase n>0 trajectories should have 2 tracked limits in their outputs with limit_ids {0, 1}; trajectory id %llu has limit tracking index %d with limit_id %d", trajectoryState.trajectory_id(), i, limitTrackingsWrap.Get(i).limit_id());}

        // fetch forth some time data from limit 0 (ie backward flux) and limit 1 (ie forward flux) tracking
        timeWrapBasinEntry.setWrappedMsg(limitTrackingsWrap.Get(0).times());
        timeWrapForwardFlux.setWrappedMsg(limitTrackingsWrap.Get(1).times());

        // check if this trajectory fluxed backwards or forwards (and make sure it didn't somehow do both)
        if (timeWrapBasinEntry.size()==1 and timeWrapForwardFlux.size()==0)       // branch for "failed" trajectories (ie ones that fluxed backward)
        {
            phaseWeightSV.push_back(0);
        }
        else if (timeWrapBasinEntry.size()==0 and timeWrapForwardFlux.size()==1)  // branch for "successful" trajectories (ie ones that fluxed forward)
        {
            phaseWeightSV.push_back(1);
        }
        else throw ConsistencyException("Finished Forward Flux phase n>0 trajectory %llu has recorded %d backward flux events and %d forward flux events; it should have either 1 forward or 1 backward flux event, and not both", trajectoryState.trajectory_id(), timeWrapBasinEntry.size(), timeWrapForwardFlux.size());
    }

public:
    // streaming variance of the waiting time in between interface 0 forward crossing events
    StreamingVariance phaseWeightSV;

protected:
    lm::protowrap::Repeated<lm::io::LimitTracking> limitTrackingsWrap;
    lm::protowrap::NDArray<double> timeWrapForwardFlux, timeWrapBasinEntry;
};

}
}

#endif // LM_FFLUX_FFLUXTRAJECTORY_H_
