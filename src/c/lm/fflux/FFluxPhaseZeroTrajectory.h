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

enum EventKind {FLUX,
                BASIN_ENTRY,
                BASIN_EXIT,
                END};

// pair of (eventKind, eventTime)
typedef std::pair<EventKind, double> Event;
// pair of (Event, dwellTimeAccumulated)
typedef std::pair<Event, double> WaitingTimeScratchpad;

class FFluxPhaseZeroTrajectory : public lm::trajectory::Trajectory
{
public:
//    FFluxPhaseZeroTrajectory(const lm::input::Input& input, uint64_t phase, uint64_t id)
//        :Trajectory(input, phase, id),hInitialBasin(true),timeInOtherBasinsLast(0.0),timeInOtherBasins(0.0),waitingTimeScratchpad(Event(BASIN_ENTRY,0),0)
//    {
//    }
//
//    template <typename InputIterator> FFluxPhaseZeroTrajectory(const lm::input::Input& input, InputIterator speciesStart, InputIterator speciesEnd, double startTime, uint64_t phase, uint64_t id)
//        :Trajectory(input, speciesStart, speciesEnd, startTime, phase, id),hInitialBasin(true),timeInOtherBasinsLast(0.0),timeInOtherBasins(0.0),waitingTimeScratchpad(Event(BASIN_ENTRY,0),0)
//    {
//    }
//
//    FFluxPhaseZeroTrajectory(const lm::io::TrajectoryState& initialState, uint64_t phase, uint64_t id)
//        :Trajectory(initialState, phase, id),hInitialBasin(true),timeInOtherBasinsLast(0.0),timeInOtherBasins(0.0),waitingTimeScratchpad(Event(BASIN_ENTRY,0),0)
//    {
//    }

    FFluxPhaseZeroTrajectory(const lm::input::Input& input, uint64_t phase, uint64_t id)
    :Trajectory(input, phase, id),waitingTimeScratchpad(Event(BASIN_ENTRY,0),0)
    {
    }

    template <typename InputIterator> FFluxPhaseZeroTrajectory(const lm::input::Input& input, InputIterator speciesStart, InputIterator speciesEnd, double startTime, uint64_t phase, uint64_t id)
    :Trajectory(input, speciesStart, speciesEnd, startTime, phase, id),waitingTimeScratchpad(Event(BASIN_ENTRY,0),0)
    {
    }

    FFluxPhaseZeroTrajectory(const lm::io::TrajectoryState& initialState, uint64_t phase, uint64_t id)
    :Trajectory(initialState, phase, id),waitingTimeScratchpad(Event(BASIN_ENTRY,0),0)
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
        timeWrapBasinEntry.setWrappedMsg(limitTrackingsWrap.Get(1).times());
        timeWrapBasinExit.setWrappedMsg(limitTrackingsWrap.Get(2).times());

        // If the trajectory was previously in a non-initial basin, or if it passed into a non-initial basin during this work unit, accumulate the time the trajectory spent in a non-initial basin during its most recent work unit
        if (timeWrapForwardFlux.size() > 0 or timeWrapBasinEntry.size() > 0 or timeWrapBasinExit.size() > 0)
        {
            double *fluxTimes, *fluxTimesEnd, *entryTimes, *entryTimesEnd, *exitTimes, *exitTimesEnd;
            fluxTimes = timeWrapForwardFlux.get_data(true);
            fluxTimesEnd = fluxTimes +  timeWrapForwardFlux.size();
            entryTimes = timeWrapBasinEntry.get_data(true);
            entryTimesEnd = entryTimes +  timeWrapBasinEntry.size();
            exitTimes = timeWrapBasinExit.get_data(true);
            exitTimesEnd = exitTimes +  timeWrapBasinExit.size();

            getWaitingTimes(waitingTimeScratchpad, waitingTimeSV, fluxTimes, fluxTimesEnd, entryTimes, entryTimesEnd, exitTimes, exitTimesEnd);

            if (timeWrapBasinEntry.compressed_deflate()) delete[] entryTimes;
            if (timeWrapBasinEntry.compressed_deflate()) delete[] entryTimes;
            if (timeWrapBasinExit.compressed_deflate()) delete[] exitTimes;
        }
    }

    static void getWaitingTimes(WaitingTimeScratchpad& wtScratch, StreamingVariance& wtsv, const double* fluxTimes, const double* fluxTimesEnd, const double* entryTimes, const double* entryTimesEnd, const double* exitTimes, const double* exitTimesEnd)
    {
        Event event;
        Event previousEvent = wtScratch.first;
        double waitingTime = wtScratch.second;

        while (true)
        {
            event = findNextEvent(previousEvent.first, &fluxTimes, fluxTimesEnd, &entryTimes, entryTimesEnd, &exitTimes, exitTimesEnd);
            switch (event.first)
            {
            case FLUX:
//                if (previousEvent.first!=BASIN_ENTRY)
//                {
//                    throw ConsistencyException("A FLUX event can only be preceded by a BASIN_ENTRY");
//                }

                waitingTime += event.second - previousEvent.second;
                wtsv.push_back(waitingTime);
                waitingTime = 0;
                break;
            case BASIN_ENTRY:
                if (previousEvent.first==FLUX)
                {
                    waitingTime += event.second - previousEvent.second;
                }
//                else if (previousEvent.first==BASIN_ENTRY)
//                {
//                    throw ConsistencyException("Can't have two BASIN_ENTRY events in a row");
//                }
                break;
            case BASIN_EXIT:
                if (previousEvent.first==FLUX)
                {
                    waitingTime += event.second - previousEvent.second;
                }
//                else if (previousEvent.first==BASIN_ENTRY)
//                {
//                    throw ConsistencyException("Can't have a BASIN_EXIT immediately following a BASIN_ENTRY");
//                }
                break;
            case END:
                wtScratch.first = previousEvent;
                wtScratch.second = waitingTime;
                return;
                break;
            }
            previousEvent = event;
        }
    }

    static Event findNextEvent(EventKind previousEventKind, const double** fluxTimes, const double* fluxTimesEnd, const double** entryTimes, const double* entryTimesEnd, const double** exitTimes, const double* exitTimesEnd)
    {
        checkLimitCurry<TrajLimEnums::MAX, false, double> greaterThanCurry(0.0);
        switch (previousEventKind)
        {
        case FLUX:
            // next event will be either a BASIN_ENTRY or a BASIN_EXIT
            if (*entryTimes != entryTimesEnd)
            {
                // there's at least one more BASIN_ENTRY event
                if (*exitTimes != exitTimesEnd)
                {
                    // there's also at least one more BASIN_EXIT event, so compare event times
                    if (**entryTimes < **exitTimes)
                    {
                        // if here, either the next BASIN_ENTRY happens before the next BASIN_EXIT, or there's at least one BASIN_ENTRY left and no more BASIN_EXIT
                        return returnEvent(BASIN_ENTRY, entryTimes, entryTimesEnd);
                    }
                    else
                    {
                        return returnEvent(BASIN_EXIT, exitTimes, exitTimesEnd);
                    }
                }
                else
                {
                    // there are no more BASIN_EXIT events, so return the next BASIN_ENTRY
                    return returnEvent(BASIN_ENTRY, entryTimes, entryTimesEnd);
                }
            }
            else
            {
                // there are no more BASIN_ENTRY events, so the only possible next event is a BASIN_EXIT
                return returnEvent(BASIN_EXIT, exitTimes, exitTimesEnd);
            }
            break;
        case BASIN_ENTRY:
            // the next event will be a FLUX
            return returnEvent(FLUX, fluxTimes, fluxTimesEnd);
            break;
        case BASIN_EXIT:
            // the next event will be a BASIN_ENTRY
            if (*entryTimes != entryTimesEnd)
            {
                // multiple BASIN_EXIT events may have occured before the BASIN_ENTRY, so first increment exitTimes as appropriate
                std::find_if(*exitTimes, exitTimesEnd, greaterThanCurry.setLimitVal(**entryTimes));

                // now return the actual event
                return Event(BASIN_ENTRY, *(*entryTimes)++);
            }
            else
            {
                // no more BASIN_ENTRY, so at the END
                return Event(END, -1.0);
            }
            break;
//            if (*entryTimes != entryTimesEnd)
//            {
//
//            }
//            Event retEv = returnEvent(BASIN_ENTRY, entryTimes, entryTimesEnd);
//
//            // multiple BASIN_EXIT events may have occured before the BASIN_ENTRY, increment exitTimes as appropriate
//            std::find_if(*exitTimes, exitTimesEnd, greaterThanCurry.setLimitVal());
//
//            return retEv;
//            break;
        case END:
//            throw ConsistencyException("findNextEvent should never be called with END as the previousEventKind, so something has gone wrong.");
            // for now, this will just return the same END Event
            return Event(END, -1.0);
            break;
        }
    }

    static Event returnEvent(EventKind eventKind, const double** timeList, const double* timeListEnd)
    {
        const double* timeListPtr = (*timeList)++;
        if (timeListPtr != timeListEnd)
        {
            // return the appropriate event if we're not at the end of the timeList
            return Event(eventKind, *timeListPtr);
        }
        else
        {
            // otherwise, return the END event
            return Event(END, -1.0);
        }
    }

//    void old_accumulateTimeInOtherBasins(const lm::io::TrajectoryState& trajectoryState)
//    {
//        // set wrapper on the limit_trackings field
//        limitTrackingsWrap.setWrappedField(trajectoryState.limit_tracking_list().limit_trackings());
//
//        // set wrappers on the ndarrays with the limit-triggering times
//        timeWrapForwardFlux.setWrappedMsg(limitTrackingsWrap.Get(0).times());
//        timeWrapBasinEntry.setWrappedMsg(limitTrackingsWrap.Get(1).times());
//        timeWrapBasinExit.setWrappedMsg(limitTrackingsWrap.Get(2).times());
//
//        timeInOtherBasinsLast = 0.0;
//        // If the trajectory was previously in a non-initial basin, or if it passed into a non-initial basin during this work unit, accumulate the time the trajectory spent in a non-initial basin during its most recent work unit
//        if ((not hInitialBasin) or timeWrapBasinExit.size() > 0)
//        {
//            double startTime = getSimTime();
//            double endTime = trajectoryState.cme_state().species_counts().time(trajectoryState.cme_state().species_counts().time_size() - 1);
//
//            double *timeDataBackwardFlux, *timeDataBackwardFluxEnd, *timeDataOtherBasinEntry, *timeDataOtherBasinEntryEnd;
//            timeDataOtherBasinEntry = timeWrapBasinExit.get_data(true);
//            timeDataOtherBasinEntryEnd = timeDataOtherBasinEntry +  timeWrapBasinExit.size();
//            timeDataBackwardFlux = timeWrapBasinEntry.get_data(true);
//            timeDataBackwardFluxEnd = timeDataBackwardFlux +  timeWrapBasinEntry.size();
//
//            timeInOtherBasinsLast = sumTimeIntervals(timeDataOtherBasinEntry, timeDataOtherBasinEntryEnd, timeDataBackwardFlux, timeDataBackwardFluxEnd, startTime, endTime, &hInitialBasin);
//            timeInOtherBasins += timeInOtherBasinsLast;
//
//            if (timeWrapBasinEntry.compressed_deflate()) delete[] timeDataBackwardFlux;
//            if (timeWrapBasinExit.compressed_deflate()) delete[] timeDataOtherBasinEntry;
//        }
//    }
//
//    // TODO: handle startTime and endTime in a more robust way and/or checked way
//    static double sumTimeIntervals(double* entryTimes, double* entryTimesEnd, double* exitTimes, double* exitTimesEnd, double startTime, double endTime, bool* notInInterval=NULL)
//    {
////        bool inverted = false;
//        double sumTime = 0.0;
//        checkLimitCurry<TrajLimEnums::MAX, false, double> greaterThanCurry(0.0);
//
//        // special handling if notInInterval is set to false at the start of the function call (implying that an interval had already started at startTime)
//        if (notInInterval!=NULL and (not *notInInterval))
//        {
////            inverted = true;
////            std::swap(entryTimes, entryTimes);
////            std::swap(entryTimesEnd, exitTimesEnd);
//
//            // find the first interval exit time after the start time
//            exitTimes = std::find_if(exitTimes, exitTimesEnd, greaterThanCurry.setLimitVal(startTime));
//
//            // if we didn't find an appropriate exit time, just return the length of the entire interval
//            if (exitTimes==exitTimesEnd)
//            {
//                return endTime - startTime;
//            }
//            // otherwise, add the difference between the first exitTime and the startTime
//            else
//            {
//                sumTime += (*exitTimes - startTime);
//
//                // set that we are now in an interval
//                if (notInInterval!=NULL) *notInInterval = true;
//
//                // change the start time to coincide with the first exitTime
//                startTime = *exitTimes;
//            }
//        }
//
//        // find the first interval entry time after the start time
//        entryTimes = std::find_if(entryTimes, entryTimesEnd, greaterThanCurry.setLimitVal(startTime));
//
//        // if we didn't find an appropriate entry time, return now
//        if (entryTimes==entryTimesEnd) return sumTime;
//
//        while (true)
//        {
//            // try to find the next interval exit time
//            exitTimes = std::find_if(exitTimes, exitTimesEnd, greaterThanCurry.setLimitVal(*entryTimes));
//            if (exitTimes==exitTimesEnd)               // If have an entryTime with no exitTime, add the difference between the endtime and the last entryTime, and then break
//            {
//                sumTime += (endTime - *entryTimes);
//                if (notInInterval!=NULL) *notInInterval = false;
//                return sumTime;
//            }
//            else                                       // Otherwise, we have found the next exit time. Add the length of this interval to the sumTime
//            {
//                sumTime += (*exitTimes - *entryTimes);
//            }
//
//            // try to find the next interval entry time
//            entryTimes = std::find_if(entryTimes, entryTimesEnd, greaterThanCurry.setLimitVal(*exitTimes));
//            if (entryTimes==entryTimesEnd)            // If we can't find another entryTime, there are no more intervals so break
//            {
//                if (notInInterval!=NULL) *notInInterval = true;
//                return sumTime;
//            }
//        }
//
////        if (inverted)
////        {
////            if (notInInterval!=NULL) *notInInterval = !(*notInInterval);
////            return (endTime - startTime) - sumTime;
////        }
////        else
////        {
////            return sumTime;
////        }
//    }

public:
//    // hInitialBasin==1 if the most recent basin the trajectory was in was the initial basin, hInitialBasin==0 otherwise (see Rien Ten Wolde, 2005)
//    bool hInitialBasin;
//
//    // the quantity of time the trajectory has spent during its most recent work unit in basins other than the one it started in
//    double timeInOtherBasinsLast;
//
//    // the total quantity of time the trajectory has spent in basins other than the one it started in
//    double timeInOtherBasins;

    // streaming variance of the waiting time in between interface 0 forward crossing events
    StreamingVariance waitingTimeSV;
    WaitingTimeScratchpad waitingTimeScratchpad;

protected:
    lm::protowrap::Repeated<lm::io::LimitTracking> limitTrackingsWrap;
    lm::protowrap::NDArray<double> timeWrapForwardFlux, timeWrapBasinEntry, timeWrapBasinExit;
};

}
}

#endif
