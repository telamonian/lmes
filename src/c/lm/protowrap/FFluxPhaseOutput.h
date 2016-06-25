/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
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
#ifndef LM_PROTWRAP_FFLUXPHASEOUTPUT_H_
#define LM_PROTWRAP_FFLUXPHASEOUTPUT_H_

#include <algorithm>
#include <limits>
#include <map>
#include <vector>

#include "lm/EnumHelper.h"
#include "lm/fflux/io/FFluxPhaseOutput.pb.h"
#include "lm/limit/LimitCheckFunctions.h"
#include "lm/io/LimitTracking.pb.h"
#include "lm/protowrap/NDArray.h"
#include "lm/protowrap/RepeatedMap.h"
#include "lm/Types.h"

using lm::protowrap::RepeatedMap;

namespace lm {
namespace protowrap {

typedef lm::fflux::io::FFluxPhaseOutput FFluxPhaseOutputMsg;
typedef lm::fflux::io::StartPoint StartPointMsg;
typedef lm::fflux::io::EndPoint EndPointMsg;

typedef std::vector<int32_t> PointKey;

// map key getter and setter for the EndPointMap typedef
template <typename PointMsg>
PointKey getPointKey(const PointMsg& pointMsgConst)
{
    return PointKey(pointMsgConst.species_coordinates().begin(), pointMsgConst.species_coordinates().end());
}
template <typename PointMsg>
void setPointKey(PointMsg* pointMsg, const PointKey& pointKey)
{
    pointMsg->mutable_species_coordinates()->clear();
    for (PointKey::const_iterator it=pointKey.begin();it!=pointKey.end();it++)
    {
        pointMsg->add_species_coordinates(*it);
    }
}
typedef RepeatedMap<EndPointMsg, PointKey, &getPointKey, &setPointKey> EndPointMap;
typedef std::map<PointKey, EndPointMap> EndPointMapMap;

//class EndPoint
//{
//    EndPointMsg* msgPtr;
//};
//
//class StartPoint
//{
//public:
//    StartPointMsg* msgPtr;
//    EndPointMap sucessfulEndPointMap;
//    EndPointMap failedEndPointMap;
//};
//typedef std::map<PointKey, StartPoint> StartPointMap;

class FFluxPhaseOutput
{
public:
    typedef FFluxPhaseOutputMsg Msg;
    typedef lm::protowrap::Repeated<lm::io::LimitTracking> TrackingsWrap;

    FFluxPhaseOutput(): msgPtr(NULL) {}
    FFluxPhaseOutput(Msg* msgMutablePtr): msgPtr(NULL) {setMsg(msgMutablePtr);}

// mutators
    Msg* getMsg()
    {
        return msgPtr;
    }

    void setMsg(Msg* newMsgMutablePtr)
    {
        msgPtr = newMsgMutablePtr;

        sucessfulEndPointMap.setRepFieldPtr(getMsg()->mutable_sucessful_trajectory_end_points());
    }

    void addEndPointFromLimitTrackingsPhaseZero(const lm::io::TrajectoryState& trajectoryState, int burnInCount)
    {
        // TODO: include consistency check constraining (phaseZeroSamples > burnInCount) somewhere

        // set wrapper on the limit_trackings field
        trackingWrap.setRepFieldPtr(trajectoryState.limit_trackings());

        // consistency checks
        if (trackingWrap.size()!=3) throw ConsistencyException("Finished Forward Flux phase zero trajectories should have 3 tracked limits in their outputs; trajectory id %llu has %d", trajectoryState.trajectory_id(), trackingWrap.size());
        for (int i=0;i<3;i++)
        {
            if (trackingWrap.Get(i).limit_id()!=i) throw ConsistencyException("Finished Forward Flux phase zero trajectories should have 3 tracked limits in their outputs with limit_ids {0, 1, 2}; trajectory id %llu has limit tracking index %d with limit_id %d", trajectoryState.trajectory_id(), i, trackingWrap.Get(i).limit_id();
        }

        // fetch forth some data from limit 0 (ie forward flux) tracking
        speciesCountWrap.setMsg(trackingWrap.Get(0).species_counts());
        timeWrapForwardFlux.setMsg(trackingWrap.Get(0).times());
        int32_t* speciesCountDataForwardFlux = speciesCountWrap.get_data(true);
        double* timeDataForwardFlux = timeWrapForwardFlux.get_data(true);

        // set total time, subtract out burn in time, and mark that we have "blocked" (ie accounted for) trajectory time up to the burn in time
        double burnInTime = burnInCount > 0 ? timeDataForwardFlux[burnInCount - 1] : 0.0;
        double totalTime = timeDataForwardFlux[timeWrapForwardFlux.size() - 1];
        
        // load points from forward flux events into sucessful endpoints
        uint rows = speciesCountWrap.shape(0);
        uint columns = speciesCountWrap.shape(1);
        for (int i=burnInCount;i<rows;i++)
        {
            pointKey.assign(speciesCountDataForwardFlux[i*columns], speciesCountDataForwardFlux[i*columns+rows]);
            EndPointMsg* endPointMsg = sucessfulEndPointMap[pointKey];
            endPointMsg->set_count(endPointMsg->count() + 1);
            endPointMsg->add_times(timeDataForwardFlux[i]);
            endPointMsg->set_success(true);
        }

        if (speciesCountWrap.compressed_deflate()) delete[] speciesCountDataForwardFlux;
        if (timeWrapForwardFlux.compressed_deflate()) delete[] timeDataForwardFlux;

        // correct totalTime for burn in and for time spent outside of the region of the starting basin (see Valeriani 2007)
        totalTime -= (getOtherBasinTimeCorrection(trajectoryState, burnInTime, totalTime) + burnInTime);
        msgPtr->set_sucessful_trajectories_launched_total_time(totalTime);
    }

    // this function encapsulates part of addEndPointFromLimitTrackingsPhaseZero, and so relies on the consistency checks run at the begininng of that function
    double getOtherBasinTimeCorrection(const lm::io::TrajectoryState& trajectoryState, double burnInTime, double endTime)
    {
        double timeCorrection = 0.0;

        timeWrapBackwardFlux.setMsg(trackingWrap.Get(1).times());
        timeWrapOtherBasinEntry.setMsg(trackingWrap.Get(2).times());

        // If the trajectory ever passed into another basin, get the sum time of the intervals between entry into another basin and reentry into the starting basin
        if (timeWrapOtherBasinEntry.size() > 0)
        {
            double *timeDataBackwardFlux, *timeDataBackwardFluxEnd, *timeDataOtherBasinEntry, *timeDataOtherBasinEntryEnd;
            timeDataOtherBasinEntry = timeWrapOtherBasinEntry.get_data(true);
            timeDataOtherBasinEntryEnd = timeDataOtherBasinEntry +  timeWrapOtherBasinEntry.size();
            timeDataBackwardFlux = timeWrapBackwardFlux.get_data(true);
            timeDataBackwardFluxEnd = timeDataBackwardFlux +  timeWrapBackwardFlux.size();

            timeCorrection = sumTimeIntervals(timeDataOtherBasinEntry, timeDataOtherBasinEntryEnd, timeDataBackwardFlux, timeDataBackwardFluxEnd, burnInTime, endTime);

            if (timeWrapBackwardFlux.compressed_deflate()) delete[] timeDataBackwardFlux;
            if (timeWrapOtherBasinEntry.compressed_deflate()) delete[] timeDataOtherBasinEntry;
        }

        return timeCorrection;
    }

    // TODO: handle startTime and endTime in a more robust way and/or checked way
    static double sumTimeIntervals(double* entryTimes, double* entryTimesEnd, double* exitTimes, double* exitTimesEnd, double startTime=0.0, double endTime=std::numeric_limits<double>::infinity())
    {
        double sumTime = 0.0;

        // find the first interval entry time after the start time
        entryTimes = checkLimitSTDAdapter<TrajLimEnums::MAX, false, double>(std::find_if, entryTimes, entryTimesEnd, startTime);
        // if we didn't find an appropriate entry time, just return 0.0
        if (entryTimes==entryTimesEnd) return sumTime;

        while (true)
        {
            // try to find the next interval exit time
            exitTimes = checkLimitSTDAdapter<TrajLimEnums::MAX, false, double>(std::find_if, exitTimes, exitTimesEnd, *entryTimes);
            if (exitTimes==exitTimesEnd)               // If have an entryTime with no exitTime, add the difference between the last entryTime and the endTime, and then break
            {
                if (not endTime==std::numeric_limits<double>::infinity()) sumTime += (endTime - *entryTimes);
                return sumTime;
            }
            else                                       // Otherwise, we have found the next exit time. Add the length of this interval to the sumTime
            {
                sumTime += (*exitTimes - *entryTimes);
            }

            // try to find the next interval entry time
            entryTimes = checkLimitSTDAdapter<TrajLimEnums::MAX, false, double>(std::find_if, entryTimes, entryTimesEnd, *exitTimes);
            if (entryTimes==entryTimesEnd)            // If we can't find another entryTime, there are no more intervals so break
            {
                return sumTime;
            }
        }
    }

    void addEndPointFromLimitTracking(const TrackingsWrap::GoogleT& limitTrackings)
    {
        speciesCountWrap.setMsg(limitTracking.species_counts());
        timeWrap.setMsg(limitTracking.times());
        int32_t* speciesCountData = speciesCountWrap.get_data(true);
        double* timeData = timeWrap.get_data(true);

        uint rows = speciesCountWrap.shape(0);
        uint columns = speciesCountWrap.shape(1);
        for (int i=0;i<rows;i++)
        {
            pointKey.assign(speciesCountData[i*columns], speciesCountData[i*columns+rows]);
            EndPointMsg* endPointMsg = sucessfulEndPointMap[pointKey];
            endPointMsg->set_count(endPointMsg->count() + 1);
            endPointMsg->add_times(timeData[i]);
            endPointMsg->set_success(isSucessfulEndPoint);
        }

        if (speciesCountWrap.compressed_deflate()) delete[] speciesCountData;
        if (timeWrap.compressed_deflate()) delete[] timeData;
    }

public:
    EndPointMap sucessfulEndPointMap;

protected:
    Msg* msgPtr;
    PointKey pointKey;
    lm::protowrap::NDArray<int32_t> speciesCountWrap;
    lm::protowrap::NDArray<double> timeWrapForwardFlux, timeWrapBackwardFlux, timeWrapOtherBasinEntry;
    TrackingsWrap trackingWrap;
};

}
}


#endif /* LM_PROTOWRAP_FFLUXPHASEOUTPUT_H_ */
