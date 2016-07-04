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
#ifndef LM_LIMIT_LIMITTRACKING_H_
#define LM_LIMIT_LIMITTRACKING_H_

#include <map>
#include <vector>

#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/protowrap/Msg.h"
#include "lm/protowrap/Repeated.h"
#include "lm/Types.h"

namespace lm {
namespace limit {

typedef uint64_t DegreeAdvancementT;
typedef double   OrderParameterT;
typedef int      SpeciesT;
typedef double   TimeT;

typedef std::vector<DegreeAdvancementT> DegreeAdvancementContainer;
typedef std::vector<OrderParameterT> OrderParameterContainer;
typedef std::vector<SpeciesT> SpeciesContainer;
typedef std::vector<TimeT> TimeContainer;


class LimitTracking
{
public:
    bool trackingEnabled(uint64_t maxCount)
    {
        // if the limit tracking has a count, use this to determine if tracking is currently enabled
        if (has_count)
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
        if (has_count)
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

public:
    int limit_id;
    bool has_count;
    uint64_t count;

    DegreeAdvancementContainer degree_advancements;
    OrderParameterContainer order_parameter_values;
    SpeciesContainer species_counts;
    TimeContainer times;
};

class LimitTrackingWrap : public lm::protowrap::Msg<LimitTrackingWrap, lm::io::LimitTracking>
{
    WRAPPED_FIELDS(required, uint64_t,                                   trajectory_id,
                   required, int32_t,                                    limit_id,
                   optional, uint64_t,                                   count,
                   optional, lm::protowrap::NDArray<DegreeAdvancementT>, degree_advancements,
                   optional, lm::protowrap::NDArray<OrderParameterT>,    order_parameter_values,
                   optional, lm::protowrap::NDArray<SpeciesT>,           species_counts,
                   optional, lm::protowrap::NDArray<TimeT>,              times)

public:
    void deserializeMetadataTo(int32_t* limitID_writeto, bool* hasCount_writeto, uint64_t* count_writeto) const
    {
        *limitID_writeto = limit_id();
        *hasCount_writeto = has_count();
        if (*hasCount_writeto) *count_writeto = count();
    }

    void deserializeMetadataTo(LimitTracking* lt) const
    {
        deserializeMetadataTo(&lt->limit_id, &lt->has_count, &lt->count);
    }

    void deserializeTo(DegreeAdvancementContainer* degreeAdvancements_writeto, OrderParameterContainer* orderParameterValues_writeto,
                       SpeciesContainer* speciesCounts_writeto, TimeContainer* times_writeto) const
    {
        degree_advancements().get_data(degreeAdvancements_writeto);

        order_parameter_values().get_data(orderParameterValues_writeto);

        species_counts().get_data(speciesCounts_writeto);

        times().get_data(times_writeto);
    }

    void deserializeTo(LimitTracking* lt) const
    {
        deserializeMetadataTo(lt);
        deserializeTo(&lt->degree_advancements, &lt->order_parameter_values, &lt->species_counts, &lt->times);
    }

    void serializeMetadataFrom(uint64_t trajectoryID_readfrom, int32_t limitID_readfrom, bool hasCount_readfrom, uint64_t count_readfrom)
    {
        set_trajectory_id(trajectoryID_readfrom);
        set_limit_id(limitID_readfrom);

        if (hasCount_readfrom) set_count(count_readfrom);
    }

    void serializeMetadataFrom(uint64_t trajectoryID_readfrom, const LimitTracking& lt)
    {
        serializeMetadataFrom(trajectoryID_readfrom, lt.limit_id, lt.has_count, lt.count);
    }

    void serializeFrom(const DegreeAdvancementContainer& degreeAdvancements_readfrom, const OrderParameterContainer& orderParameterValues_readfrom,
                     const SpeciesContainer& speciesCounts_readfrom, const TimeContainer& times_readfrom, bool compress=false)
    {
        mutable_degree_advancements()->set_array(degreeAdvancements_readfrom, utuple(degreeAdvancements_readfrom.size()), compress);

        mutable_order_parameter_values()->set_array(orderParameterValues_readfrom, utuple(orderParameterValues_readfrom.size()), compress);

        mutable_species_counts()->set_array(speciesCounts_readfrom, utuple(speciesCounts_readfrom.size()), compress);

        mutable_times()->set_array(times_readfrom, utuple(times_readfrom.size()), compress);
    }

    void serializeFrom(uint64_t trajectoryID_readfrom, const LimitTracking& lt)
    {
        serializeMetadataFrom(trajectoryID_readfrom, lt);
        serializeFrom(lt.degree_advancements, lt.order_parameter_values, lt.species_counts, lt.times);
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

};

typedef std::map<int, LimitTracking> TrackingMapT;
typedef lm::protowrap::Repeated<LimitTrackingWrap::WrappedMsg> LimitTrackingRepeated;

}
}

#endif /* LM_LIMIT_LIMITTRACKING_H_ */