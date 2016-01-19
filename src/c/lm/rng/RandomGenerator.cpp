/*
 * University of Illinois Open Source License
 * Copyright 2008-2012 Luthey-Schulten Group,
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 *               University of Illinois at Urbana-Champaign
 *               http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS WITH THE SOFTWARE.
 *
 * Author(s): Elijah Roberts
 */

#include <cmath>
#if defined(MACOSX)
#include <mach/mach_time.h>
#elif defined(LINUX)
#include <time.h>
#endif
#include "lm/Exceptions.h"
#include "lm/Types.h"
#include "lm/rng/RandomGenerator.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

namespace lm {
namespace rng {

RandomGenerator::RandomGenerator(uint32_t seedTop, uint32_t seedBottom, Distributions availableDists)
:seed(0LL),availableDists(availableDists)
{
    if (seedBottom == 0)
    {
        #if defined(MACOSX)
        seedBottom = (uint32_t)mach_absolute_time();
        #elif defined(LINUX)
        struct timespec seed_timespec;
        if (clock_gettime(CLOCK_REALTIME, &seed_timespec) != 0) throw lm::Exception("Error getting time to use for random seed.");
        seedBottom = seed_timespec.tv_nsec;
        #endif
    }

    if (seedTop == 0)
    {
        #if defined(MACOSX)
        seedTop = (uint32_t)mach_absolute_time();
        #elif defined(LINUX)
        struct timespec seed_timespec;
        if (clock_gettime(CLOCK_REALTIME, &seed_timespec) != 0) throw lm::Exception("Error getting time to use for random seed.");
        seedTop = seed_timespec.tv_nsec;
        #endif
    }

    seed = (((uint64_t)(seedTop))<<32)|(uint64_t)(seedBottom);
}

void RandomGenerator::getRandomDoubles(double * rngs, int numberRNGs)
{
    PROF_BEGIN(PROF_CACHE_RNG);
    for (int i=0; i<numberRNGs; i++)
    {
        rngs[i] = getRandomDouble();
    }
    PROF_END(PROF_CACHE_RNG);
}

void RandomGenerator::getExpRandomDoubles(double * rngs, int numberRNGs)
{
    PROF_BEGIN(PROF_CACHE_EXP_RNG);
    for (int i=0; i<numberRNGs; i++)
    {
        rngs[i] = getExpRandomDouble();
    }
    PROF_END(PROF_CACHE_EXP_RNG);
}

void RandomGenerator::getNormRandomDoubles(double * rngs, int numberRNGs)
{
    PROF_BEGIN(PROF_CACHE_NORM_RNG);
    for (int i=0; i<numberRNGs; i++)
    {
        rngs[i] = getNormRandomDouble();
    }
    PROF_END(PROF_CACHE_NORM_RNG);
}

}
}
