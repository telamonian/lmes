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

#include "lm/fflux/FFluxTrajectory.h"
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

class FFluxPhaseZeroTrajectory : public lm::fflux::FFluxTrajectory
{
public:
    FFluxPhaseZeroTrajectory(const lm::input::Input& input, uint64_t phase, uint64_t id)
    :FFluxTrajectory(input, phase, id),initialWaitingTime(0),previousEvent(BASIN_ENTRY,0)
    {
    }

    template <typename InputIterator> FFluxPhaseZeroTrajectory(const lm::input::Input& input, InputIterator speciesStart, InputIterator speciesEnd, double startTime, uint64_t phase, uint64_t id)
    :FFluxTrajectory(input, speciesStart, speciesEnd, startTime, phase, id),initialWaitingTime(0),previousEvent(BASIN_ENTRY,0)
    {
    }

    FFluxPhaseZeroTrajectory(const lm::io::TrajectoryState& initialState, uint64_t phase, uint64_t id)
    :FFluxTrajectory(initialState, phase, id),initialWaitingTime(0),previousEvent(BASIN_ENTRY,0)
    {
    }

    virtual ~FFluxPhaseZeroTrajectory()
    {
    }

    virtual void processState(const lm::io::TrajectoryState& trajectoryState)
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

            initialWaitingTime = getWaitingTimes(initialWaitingTime, &previousEvent, &phaseWeightSV, fluxTimes, fluxTimesEnd, entryTimes, entryTimesEnd, exitTimes, exitTimesEnd);

            if (timeWrapBasinEntry.compressed_deflate()) delete[] entryTimes;
            if (timeWrapBasinEntry.compressed_deflate()) delete[] entryTimes;
            if (timeWrapBasinExit.compressed_deflate()) delete[] exitTimes;
        }
    }

    template <typename Accumulator>
    static double getWaitingTimes(double waitingTime, Event* previousEvent, Accumulator* acc, const double* fluxTimes, const double* fluxTimesEnd, const double* entryTimes, const double* entryTimesEnd, const double* exitTimes, const double* exitTimesEnd)
    {
        Event event;

        while (true)
        {
            event = findNextEvent(previousEvent->first, &fluxTimes, fluxTimesEnd, &entryTimes, entryTimesEnd, &exitTimes, exitTimesEnd);
            switch (event.first)
            {
            case FLUX:
//                if (previousEvent.first!=BASIN_ENTRY)
//                {
//                    throw ConsistencyException("A FLUX event can only be preceded by a BASIN_ENTRY");
//                }

                waitingTime += event.second - previousEvent->second;
                acc->push_back(waitingTime);
                waitingTime = 0;
                break;
            case BASIN_ENTRY:
                if (previousEvent->first==FLUX)
                {
                    waitingTime += event.second - previousEvent->second;
                }
//                else if (previousEvent.first==BASIN_ENTRY)
//                {
//                    throw ConsistencyException("Can't have two BASIN_ENTRY events in a row");
//                }
                break;
            case BASIN_EXIT:
                if (previousEvent->first==FLUX)
                {
                    waitingTime += event.second - previousEvent->second;
                }
//                else if (previousEvent.first==BASIN_ENTRY)
//                {
//                    throw ConsistencyException("Can't have a BASIN_EXIT immediately following a BASIN_ENTRY");
//                }
                break;
            case END:
                return waitingTime;
                break;
            }
            *previousEvent = event;
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
        case END:
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

public:
    double initialWaitingTime;
    Event previousEvent;

protected:
    lm::protowrap::NDArray<double> timeWrapBasinExit;
};

}
}

#endif
