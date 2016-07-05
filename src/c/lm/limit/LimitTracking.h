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

class LimitTracking
{
public:
    typedef uint64_t DegreeAdvancementT;
    typedef double   OrderParameterT;
    typedef int      SpeciesT;
    typedef double   TimeT;

    typedef std::vector<DegreeAdvancementT> DegreeAdvancementContainer;
    typedef std::vector<OrderParameterT> OrderParameterContainer;
    typedef std::vector<SpeciesT> SpeciesContainer;
    typedef std::vector<TimeT> TimeContainer;

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
typedef std::map<int, LimitTracking> TrackingMap;

}
}

#endif /* LM_LIMIT_LIMITTRACKING_H_ */