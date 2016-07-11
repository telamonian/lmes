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
#ifndef LM_LIMIT_LIMITTRACKINGWRAP_H_
#define LM_LIMIT_LIMITTRACKINGWRAP_H_

#include <map>
#include <vector>

#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/limit/LimitTracking.h"
#include "lm/protowrap/Msg.h"
#include "lm/protowrap/Repeated.h"
#include "lm/protowrap/WrappedFields.h"
#include "lm/Types.h"

namespace lm {
namespace limit {

class LimitTrackingWrap : public lm::protowrap::Msg<LimitTrackingWrap, lm::io::LimitTracking>
{
public:
    typedef lm::protowrap::NDArray<lm::limit::LimitTracking::DegreeAdvancementT> DegreeAdvancementsWrap;
    typedef lm::protowrap::NDArray<lm::limit::LimitTracking::OrderParameterT>    OrderParameterValuesWrap;
    typedef lm::protowrap::NDArray<lm::limit::LimitTracking::SpeciesT>           SpeciesCountsWrap;
    typedef lm::protowrap::NDArray<lm::limit::LimitTracking::TimeT>              TimesWrap;

    WRAPPED_FIELDS(required, uint64_t,                 trajectory_id,
                   required, int32_t,                  limit_id,
                   optional, uint64_t,                 count,
                   optional, DegreeAdvancementsWrap,   degree_advancements,
                   optional, OrderParameterValuesWrap, order_parameter_values,
                   optional, SpeciesCountsWrap,        species_counts,
                   optional, TimesWrap,                times)

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

    void deserializeTo(lm::limit::LimitTracking::DegreeAdvancementContainer* degreeAdvancements_writeto, lm::limit::LimitTracking::OrderParameterContainer* orderParameterValues_writeto,
                       lm::limit::LimitTracking::SpeciesContainer* speciesCounts_writeto, lm::limit::LimitTracking::TimeContainer* times_writeto) const
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

    void serializeFrom(const lm::limit::LimitTracking::DegreeAdvancementContainer& degreeAdvancements_readfrom, const lm::limit::LimitTracking::OrderParameterContainer& orderParameterValues_readfrom,
                       const lm::limit::LimitTracking::SpeciesContainer& speciesCounts_readfrom, const lm::limit::LimitTracking::TimeContainer& times_readfrom, bool compress=false)
    {
        // prevent any divide-by-zeros
        if (times_readfrom.size() > 0)
        {
            // times_readfrom.size() is the number of "rows" in this dataset
            _degree_advancements.set_array(degreeAdvancements_readfrom, utuple(times_readfrom.size(), degreeAdvancements_readfrom.size()/times_readfrom.size()), compress);
            _order_parameter_values.set_array(orderParameterValues_readfrom, utuple(times_readfrom.size(), orderParameterValues_readfrom.size()/times_readfrom.size()), compress);
            _species_counts.set_array(speciesCounts_readfrom, utuple(times_readfrom.size(), speciesCounts_readfrom.size()/times_readfrom.size()), compress);
            _times.set_array(times_readfrom, utuple(times_readfrom.size()), compress);
        }
        else
        {
            _degree_advancements.set_array(degreeAdvancements_readfrom, utuple(0), compress);
            _order_parameter_values.set_array(orderParameterValues_readfrom, utuple(0), compress);
            _species_counts.set_array(speciesCounts_readfrom, utuple(0), compress);
            _times.set_array(times_readfrom, utuple(0), compress);
        }
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

}
}

#endif /* LM_LIMIT_LIMITTRACKINGWRAP_H_ */