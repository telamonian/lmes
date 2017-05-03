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
#ifndef LM_FFLUX_FFLUXPHASEZEROTRAJECTORY_H_
#define LM_FFLUX_FFLUXPHASEZEROTRAJECTORY_H_

#include <algorithm>

#include "lm/EnumHelper.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/input/Input.h"
#include "lm/limit/LimitCheckFunctions.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

namespace lm {
namespace fflux {

class FFluxPhaseZeroTrajectory : public lm::trajectory::Trajectory
{
public:
    FFluxPhaseZeroTrajectory(const lm::input::Input& input, uint64_t phase, uint64_t id)
    :Trajectory(input, phase, id),hInitialBasin(true),timeInOtherBasinsLast(0.0),timeInOtherBasins(0.0)
    {
    }

    template <typename InputIterator> FFluxPhaseZeroTrajectory(const lm::input::Input& input, InputIterator speciesStart, InputIterator speciesEnd, double startTime, uint64_t phase, uint64_t id)
    :Trajectory(input, speciesStart, speciesEnd, startTime, phase, id),hInitialBasin(true),timeInOtherBasinsLast(0.0),timeInOtherBasins(0.0)
    {
    }

    FFluxPhaseZeroTrajectory(const lm::io::TrajectoryState& initialState, uint64_t phase, uint64_t id)
    :Trajectory(initialState, phase, id),hInitialBasin(true),timeInOtherBasinsLast(0.0),timeInOtherBasins(0.0)
    {
    }

    virtual ~FFluxPhaseZeroTrajectory()
    {
    }

    void accumulateTimeInOtherBasins(const lm::io::TrajectoryState& trajectoryState)
    {
        // set wrapper on the limit_trackings field
        limitTrackingsWrap.setWrappedField(trajectoryState.limit_tracking_list().limit_trackings());

        // set wrappers on the ndarrays with the limit-triggering times
        timeWrapForwardFlux.setWrappedMsg(limitTrackingsWrap.Get(0).times());
        timeWrapBackwardFlux.setWrappedMsg(limitTrackingsWrap.Get(1).times());
        timeWrapOtherBasinEntry.setWrappedMsg(limitTrackingsWrap.Get(2).times());

        timeInOtherBasinsLast = 0.0;
        // If the trajectory was previously in a non-initial basin, or if it passed into a non-initial basin during this work unit, accumulate the time the trajectory spent in a non-initial basin during its most recent work unit
        if ((not hInitialBasin) or timeWrapOtherBasinEntry.size() > 0)
        {
            double startTime = getSimTime();
            double endTime = trajectoryState.cme_state().species_counts().time(trajectoryState.cme_state().species_counts().time_size() - 1);

            double *timeDataBackwardFlux, *timeDataBackwardFluxEnd, *timeDataOtherBasinEntry, *timeDataOtherBasinEntryEnd;
            timeDataOtherBasinEntry = timeWrapOtherBasinEntry.get_data(true);
            timeDataOtherBasinEntryEnd = timeDataOtherBasinEntry +  timeWrapOtherBasinEntry.size();
            timeDataBackwardFlux = timeWrapBackwardFlux.get_data(true);
            timeDataBackwardFluxEnd = timeDataBackwardFlux +  timeWrapBackwardFlux.size();

            timeInOtherBasinsLast = sumTimeIntervals(timeDataOtherBasinEntry, timeDataOtherBasinEntryEnd, timeDataBackwardFlux, timeDataBackwardFluxEnd, startTime, endTime, &hInitialBasin);
            timeInOtherBasins += timeInOtherBasinsLast;

            if (timeWrapBackwardFlux.compressed_deflate()) delete[] timeDataBackwardFlux;
            if (timeWrapOtherBasinEntry.compressed_deflate()) delete[] timeDataOtherBasinEntry;
        }
    }

    // TODO: handle startTime and endTime in a more robust way and/or checked way
    static double sumTimeIntervals(double* entryTimes, double* entryTimesEnd, double* exitTimes, double* exitTimesEnd, double startTime, double endTime, bool* notInInterval=NULL)
    {
//        bool inverted = false;
        double sumTime = 0.0;
        checkLimitCurry<TrajLimEnums::MAX, false, double> greaterThanCurry(0.0);

        // special handling if notInInterval is set to false at the start of the function call (implying that an interval had already started at startTime)
        if (notInInterval!=NULL and (not *notInInterval))
        {
//            inverted = true;
//            std::swap(entryTimes, entryTimes);
//            std::swap(entryTimesEnd, exitTimesEnd);

            // find the first interval exit time after the start time
            exitTimes = std::find_if(exitTimes, exitTimesEnd, greaterThanCurry.setLimitVal(startTime));

            // if we didn't find an appropriate exit time, just return the length of the entire interval
            if (exitTimes==exitTimesEnd)
            {
                return endTime - startTime;
            }
            // otherwise, add the difference between the first exitTime and the startTime
            else
            {
                sumTime += (*exitTimes - startTime);

                // set that we are now in an interval
                if (notInInterval!=NULL) *notInInterval = true;

                // change the start time to coincide with the first exitTime
                startTime = *exitTimes;
            }
        }

        // find the first interval entry time after the start time
        entryTimes = std::find_if(entryTimes, entryTimesEnd, greaterThanCurry.setLimitVal(startTime));

        // if we didn't find an appropriate entry time, return now
        if (entryTimes==entryTimesEnd) return sumTime;

        while (true)
        {
            // try to find the next interval exit time
            exitTimes = std::find_if(exitTimes, exitTimesEnd, greaterThanCurry.setLimitVal(*entryTimes));
            if (exitTimes==exitTimesEnd)               // If have an entryTime with no exitTime, add the difference between the endtime and the last entryTime, and then break
            {
                sumTime += (endTime - *entryTimes);
                if (notInInterval!=NULL) *notInInterval = false;
                return sumTime;
            }
            else                                       // Otherwise, we have found the next exit time. Add the length of this interval to the sumTime
            {
                sumTime += (*exitTimes - *entryTimes);
            }

            // try to find the next interval entry time
            entryTimes = std::find_if(entryTimes, entryTimesEnd, greaterThanCurry.setLimitVal(*exitTimes));
            if (entryTimes==entryTimesEnd)            // If we can't find another entryTime, there are no more intervals so break
            {
                if (notInInterval!=NULL) *notInInterval = true;
                return sumTime;
            }
        }

//        if (inverted)
//        {
//            if (notInInterval!=NULL) *notInInterval = !(*notInInterval);
//            return (endTime - startTime) - sumTime;
//        }
//        else
//        {
//            return sumTime;
//        }
    }

public:
    // hInitialBasin==1 if the most recent basin the trajectory was in was the initial basin, hInitialBasin==0 otherwise (see Rien Ten Wolde, 2005)
    bool hInitialBasin;

    // the quantity of time the trajectory has spent during its most recent work unit in basins other than the one it started in
    double timeInOtherBasinsLast;

    // the total quantity of time the trajectory has spent in basins other than the one it started in
    double timeInOtherBasins;

    // streaming variance of the waiting time in between interface 0 forward crossing events
    StreamingVariance waitingTimeSV;

protected:
    lm::protowrap::Repeated<lm::io::LimitTracking> limitTrackingsWrap;
    lm::protowrap::NDArray<double> timeWrapForwardFlux, timeWrapBackwardFlux, timeWrapOtherBasinEntry;
};

}
}

#endif
