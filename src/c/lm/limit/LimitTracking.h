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
#ifndef LM_LIMIT_LIMITTRACKING
#define LM_LIMIT_LIMITTRACKING

#include <map>
#include <vector>

#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/protowrap/Repeated.h"
#include "lm/Types.h"

namespace lm {
namespace limit {

class LimitTracking
{
public:
    typedef uint64_t DegreeAdvancementT;
    typedef double OrderParameterT;
    typedef int SpeciesT;
    typedef double TimeT;

    typedef lm::io::LimitTracking Msg;
    typedef lm::protowrap::Repeated<Msg> LimitTrackings;

    typedef std::vector<DegreeAdvancementT> DegreeAdvancementContainerT;
    typedef std::vector<OrderParameterT> OrderParameterContainerT;
    typedef std::vector<SpeciesT> SpeciesContainerT;
    typedef std::vector<TimeT> TimeContainerT;

public:
//    LimitTracking()
//    :degreeAdvancmentsWrapConst(degreeAdvancmentsWrap), orderParameterWrapConst(orderParameterWrap),
//     speciesWrapConst(speciesWrap), timesWrapConst(timesWrap) {}

    void deserializeFrom(const Msg& msgRef)
    {
        limitID = msgRef.limit_id();
        hasCount = msgRef.has_count();
        if (hasCount) count = msgRef.count();

        degreeAdvancmentsWrap.setMsg(msgRef.degree_advancements());
        degreeAdvancmentsWrap.get_data(degreeAdvancements);

        orderParameterWrap.setMsg(msgRef.order_parameter_values());
        orderParameterWrap.get_data(orderParameterValues);

        speciesWrap.setMsg(msgRef.species_counts());
        speciesWrap.get_data(speciesCounts);

        timesWrap.setMsg(msgRef.times());
        timesWrap.get_data(times);
    }

    bool trackingEnabled(uint64_t maxCount)
    {
        // if the limit tracking has a count, use this to determine if tracking is currently enabled
        if (hasCount)
        {
            // return true if count less than or equal to maxCount (passed in from the associated limit), false otherwise
            return (count <= maxCount);
        }
            // if the limit tracking has no count, by default tracking is enabled
        else
        {
            return true;
        }
    }

    bool terminationSignaled(uint64_t maxCount)
    {
        // if the limit tracking has a count, use this to determinate if we should signal for termination of the trajectory
        if (hasCount)
        {
            // return true if count greater than or equal to maxCount, false otherwise
            return (count >= maxCount);
        }
            // if the limit tracking does not have a countdown, by default we never terminate
        else
        {
            return false;
        }
    }

    // handle necessary tasks when the associated limit is triggered (eg decrement countdown, record state, etc)
    void trackLimit()    //(uint64_t maxCount)
    {
        count++;
//        if (trackingEnabled(maxCount))
//        {
//
//        }
//        return terminationSignaled(maxCount);
    }

    void serializeMetadataTo(Msg* msg, uint64_t trajectoryID) const
    {
        msg->set_trajectory_id(trajectoryID);
        msg->set_limit_id(limitID);

        if (hasCount) msg->set_count(count);
    }

    void serializeTo(Msg* msg, uint64_t trajectoryId) const
    {
        serializeTo(msg, trajectoryId, degreeAdvancements, orderParameterValues, speciesCounts, times);
    }

    void serializeTo(Msg* msg, uint64_t trajectoryID, const DegreeAdvancementContainerT& degreeAdvancementsRef,
                     const OrderParameterContainerT& orderParameterValuesRef, const SpeciesContainerT& speciesCountsRef,
                     const TimeContainerT& timesRef) const
    {
        serializeMetadataTo(msg, trajectoryID);

        degreeAdvancmentsWrap.setMsg(msg->mutable_degree_advancements());
        degreeAdvancmentsWrap.set_array(degreeAdvancementsRef, utuple(degreeAdvancementsRef.size()), false);

        orderParameterWrap.setMsg(msg->mutable_order_parameter_values());
        orderParameterWrap.set_array(orderParameterValuesRef, utuple(orderParameterValuesRef.size()), false);

        speciesWrap.setMsg(msg->mutable_species_counts());
        speciesWrap.set_array(speciesCountsRef, utuple(speciesCountsRef.size()), false);

        timesWrap.setMsg(msg->mutable_times());
        timesWrap.set_array(timesRef, utuple(timesRef.size()), false);
    }

//    /*
//     * - method for getting the total time spent after the tracked limit had been triggered but before the limit tracked by otherLimitTracking had been triggered
//     *     - example:
//     *         - if the times when this limit tracking saw its limit triggered look like this
//     *             - {0.0, 1.1, 1.2, 19.0}
//     *         - and the times when otherTrackingLimit saw its limit triggered look like this
//     *             - {.5, 5.2, 12.9, 21.3}
//     *         - then the return value will be
//     *             - (.5 - 0.0) + (5.2 - 1.1) + (21.3 - 19.0) = 6.9
//     */
//    double sumTimeIntervalsBetweenLimits(LimitTracking& otherLimitTracking, double startTime=0.0, double endTime=NAN)
//    {}

public:
    int limitID;
    bool hasCount;
    uint64_t count;

    DegreeAdvancementContainerT degreeAdvancements;
    OrderParameterContainerT orderParameterValues;
    SpeciesContainerT speciesCounts;
    TimeContainerT times;

    mutable lm::protowrap::NDArray<DegreeAdvancementT> degreeAdvancmentsWrap;
    mutable lm::protowrap::NDArray<OrderParameterT> orderParameterWrap;
    mutable lm::protowrap::NDArray<SpeciesT> speciesWrap;
    mutable lm::protowrap::NDArray<TimeT> timesWrap;

//    const lm::protowrap::NDArray<DegreeAdvancementT>& degreeAdvancmentsWrapConst;
//    const lm::protowrap::NDArray<OrderParameterT>& orderParameterWrapConst;
//    const lm::protowrap::NDArray<SpeciesT>& speciesWrapConst;
//    const lm::protowrap::NDArray<TimeT>& timesWrapConst;
};

typedef std::map<int, LimitTracking> TrackingMapT;

}
}

#endif /* LM_LIMIT_LIMITTRACKING */