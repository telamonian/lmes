/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
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
#ifndef LM_LIMIT_LIMITTRACKINGLISTWRAP_H_
#define LM_LIMIT_LIMITTRACKINGLISTWRAP_H_

#include <map>
#include <vector>

#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/limit/LimitTracking.h"
#include "lm/limit/LimitTrackingWrap.h"
#include "lm/limit/TrajectoryLimit.h"
#include "lm/protowrap/Msg.h"
#include "lm/protowrap/Repeated.h"
#include "lm/protowrap/WrappedFields.h"
#include "lm/Types.h"

namespace lm {
namespace limit {

class LimitTrackingListWrap : public lm::protowrap::Msg<LimitTrackingListWrap, lm::io::LimitTrackingList>
{
    MSG_WRAP_CONSTRUCTORS(LimitTrackingListWrap)
    WRAPPED_FIELDS(repeated, lm::io::LimitTracking, limit_trackings)

public:
    inline void clearStateData()
    {
        for (lm::protowrap::Repeated<lm::io::LimitTracking>::iterator it=mutable_limit_trackings()->begin();it!=mutable_limit_trackings()->end();it++)
        {
            limitTrackingWrap.setWrappedMsg(&*it);
            limitTrackingWrap.clearStateData();
        }
    }

    inline void clearTimeSeriesData()
    {
        for (lm::protowrap::Repeated<lm::io::LimitTracking>::iterator it=mutable_limit_trackings()->begin();it!=mutable_limit_trackings()->end();it++)
        {
            limitTrackingWrap.setWrappedMsg(&*it);
            limitTrackingWrap.clearTimeSeriesData();
        }
    }

    inline void deserializeMetadataTo(TrackingMap* ltMap) const
    {
        for (lm::protowrap::Repeated<lm::io::LimitTracking>::const_iterator it=limit_trackings().begin(); it!=limit_trackings().end(); ++it)
        {
            // TODO: CV! my nemesis. Fix the need for the const_cast here, probably via some kind of const_LimitTrackingWrap class
            limitTrackingWrap.setWrappedMsg(const_cast<lm::io::LimitTracking*>(&*it));

            // intermediate ptr variable limitTrackingPtr included for readability
            lm::limit::LimitTracking* limitTrackingPtr = &(*ltMap)[it->limit_id()];
            limitTrackingWrap.deserializeMetadataTo(limitTrackingPtr);
        }
    }

    inline void deserializeTo(TrackingMap* ltMap) const
    {
        for (lm::protowrap::Repeated<lm::io::LimitTracking>::const_iterator it=limit_trackings().begin(); it!=limit_trackings().end(); ++it)
        {
            // TODO: CV! my nemesis. Fix the need for the const_cast here, probably via some kind of const_LimitTrackingWrap class
            limitTrackingWrap.setWrappedMsg(const_cast<lm::io::LimitTracking*>(&*it));

            // intermediate ptr variable limitTrackingPtr included for readability
            lm::limit::LimitTracking* limitTrackingPtr = &(*ltMap)[it->limit_id()];
            limitTrackingWrap.deserializeMetadataTo(limitTrackingPtr);
            limitTrackingWrap.deserializeTo(limitTrackingPtr);
        }
    }

    inline void serializeMetadataFrom(uint64_t trajectoryID_readfrom, const TrackingMap& ltMap, const lm::limit::TrajectoryLimit* limits)
    {
        for (TrackingMap::const_iterator it=ltMap.begin(); it!=ltMap.end(); ++it)
        {
            limitTrackingWrap.setWrappedMsg(add_limit_trackings());
            limitTrackingWrap.serializeMetadataFrom(trajectoryID_readfrom, it->second);
        }
    }

    inline void serializeFrom(uint64_t trajectoryID_readfrom, const TrackingMap& ltMap, const lm::limit::TrajectoryLimit* limits)
    {
        for (TrackingMap::const_iterator it=ltMap.begin(); it!=ltMap.end(); ++it)
        {
            const lm::limit::TrajectoryLimit& l = limits[it->second.limit_id];
            limitTrackingWrap.setWrappedMsg(add_limit_trackings());

            if (l.addTrackingToCMEState)
            {
                limitTrackingWrap.serializeFrom(trajectoryID_readfrom, it->second);
            }
            else
            {
                limitTrackingWrap.serializeMetadataFrom(trajectoryID_readfrom, it->second);
            }
        }
    }

    inline lm::io::LimitTracking* addTrackingMsg(lm::input::TrajectoryLimit* limitToTrack, bool addToCMEState=false, bool addToOutput=true, int64_t count=1, bool terminate=true)
    {
        limitToTrack->set_terminate(terminate);

        limitToTrack->set_add_tracking_to_output(addToOutput);
        limitToTrack->set_add_tracking_to_cme_state(addToCMEState);

        // if count < 0, unset track_count. When unset, tracking data will be collected every time the limit is triggered and the limit will never trigger trajectory termination
        if (count < 0) limitToTrack->clear_track_count();
        else           limitToTrack->set_track_count(static_cast<uint>(count));

        // initialize the actual tracking message
        LimitTrackingWrap::WrappedMsg* trackingMsg = add_limit_trackings();
        trackingMsg->set_limit_id(limitToTrack->id());
        // if the limit has a track_count, set the corresponding field in trackingMsg (to 0)
        if (limitToTrack->has_track_count()) trackingMsg->set_count(0);
        return trackingMsg;
    }

    // version of addTracking message that allow for setting non terminating tracking without necessarily filling in every default value in the signature
    inline lm::io::LimitTracking* addTrackingMsgNonterminating(lm::input::TrajectoryLimit* limitToTrack, bool addToCMEState=false, bool addToOutput=true, int64_t count=-1)
    {
        return addTrackingMsg(limitToTrack, addToOutput, addToCMEState, count, false);
    }

    inline void set_all_trajectory_id(uint64_t trajectoryID) {_limit_trackings.SetAll(trajectoryID, &lm::io::LimitTracking::set_trajectory_id);}

protected:
    mutable lm::limit::LimitTrackingWrap limitTrackingWrap;
};

}
}

#endif /* LM_LIMIT_LIMITTRACKINGLISTWRAP_H_ */