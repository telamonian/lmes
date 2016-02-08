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
#include "lm/Types.h"
#include "lm/rng/RandomGenerator.h"
#include "lm/rng/XORShift.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

namespace lm {
namespace rng {

XORShift::XORShift(uint32_t seedTop, uint32_t seedBottom)
:RandomGenerator(seedTop,seedBottom),state(1),isNextGaussianValid(false)
{
}

uint32_t XORShift::getRandom()
{
    unsigned long long v = state++;
    v ^= seed;
    v = v * 3935559000370003845ULL + 2691343689449507681ULL;
    v ^= v >> 21; v ^= v << 37; v ^= v >> 4;
    v *= 2685821657736338717ULL;
    return v>>32;
}

/**
 * Returns a float value in the range [0.0 1.0).
 */
double XORShift::getRandomDouble()
{
    uint32_t r = getRandom();
    return ((double)r)*(2.328306436539e-10); //1/(2^32)
}

/**
 * Returns an unsigned int value in the range [low high). Very slightly biased towards low
 */
unsigned int XORShift::getRandomIntFromRange(unsigned int low, unsigned int high)
{
	uint32_t r = getRandom();
	return floor(r*(high - low)*(2.328306436539e-10) + low); //1/((2^32))
}

/**
 * Returns an exponentially distributed value.
 */
double XORShift::getExpRandomDouble()
{
    uint64_t r;
    while ((r=(uint64_t)getRandom()) == 0);
    double d = ((double)r)*(2.328306435997e-10); //1/((2^32)+1) range (0.0 1.0)
    return -log(d);
}

/**
 * Returns an normally distributed value.
 */
double XORShift::getNormRandomDouble()
{
    if (isNextGaussianValid)
    {
        isNextGaussianValid = false;
        return nextGaussian;
    }

    // Generate two uniform random numbers.
    uint64_t r;
    while ((r=(uint64_t)getRandom()) == 0);
    double d1 = ((double)r)*(2.328306435997e-10); //1/((2^32)+1) range (0.0 1.0)
    while ((r=(uint64_t)getRandom()) == 0);
    double d2 = ((double)r)*(2.328306435997e-10 * 6.2831853071795860); //1/((2^32)+1)*PI range (0.0 2PI)

    // Transform using Box-Muller.
    isNextGaussianValid = true;
    double s = sqrt(-2.0 * log(d1));
    #if defined(LINUX)
    double ret;
    sincos(d2, &nextGaussian, &ret);
    nextGaussian *= s;
    return s*ret;
    #else
    nextGaussian = s * sin(d2);
    return s * cos(d2);
    #endif
}

void XORShift::getRandomDoubles(double * rngs, int numberRNGs, bool greaterThanZero, bool bufferAvxAligned)
{
#ifdef OPT_AVX
    if (!bufferAvxAligned) {
#endif
        PROF_BEGIN(PROF_CACHE_RNG);
        for (int i=0; i<numberRNGs; i++)
        {
            uint32_t r = getRandom();
            rngs[i] = ((double)r)*(2.328306436539e-10); //1/(2^32)
        }
        PROF_END(PROF_CACHE_RNG);
#ifdef OPT_AVX
    } else {
        PROF_BEGIN(PROF_CACHE_RNG);

        // Convert to double and normalize using avx.
        const avxd norm = greaterThanZero?_mm256_set1_pd(2.328306435996595202819747782996e-10):_mm256_set1_pd(2.328306436538696289062500000000e-10);// 1/(2^32+1) or 1/(2^32)
        const avxd half = _mm256_set1_pd(0.5);
        const uint LOOPS = 2;
        avxi irng[LOOPS];
        for (int i=0; i<numberRNGs; i+=INT32S_PER_AVX*LOOPS)
        {
            for (int j=0; j<INT32S_PER_AVX*LOOPS; j++)
                ((int32_t*)&irng)[j] = getRandom();

            for (int j=0; j<LOOPS; j++)
            {
                // Process the four lo rngs.
                __m128i irngHalf = _mm256_extractf128_si256(irng[j], 0);
                avxd rng = _mm256_fmadd_pd(_mm256_cvtepi32_pd(irngHalf), norm, half);    // Range (-0.5-0.5)+0.5
                _mm256_store_pd(&rngs[i+j*2*DOUBLES_PER_AVX],rng);

                // Process the four hi rngs.
                irngHalf = _mm256_extractf128_si256(irng[j], 1);
                rng = _mm256_fmadd_pd(_mm256_cvtepi32_pd(irngHalf), norm, half);    // Range (-0.5-0.5)+0.5
                _mm256_store_pd(&rngs[i+j*2*DOUBLES_PER_AVX+DOUBLES_PER_AVX],rng);
            }
        }

        // Free the int buffer.
        PROF_END(PROF_CACHE_RNG);
    }
#endif
}

void XORShift::getExpRandomDoubles(double * rngs, int numberRNGs, bool bufferAvxAligned)
{
#ifdef OPT_AVX
    if (!bufferAvxAligned) {
#endif
        PROF_BEGIN(PROF_CACHE_EXP_RNG);
        for (int i=0; i<numberRNGs; i++)
        {
            uint64_t r;
            while ((r=(uint64_t)getRandom()) == 0);
            double d = ((double)r)*(2.328306435997e-10); //1/((2^32)+1) range (0.0 1.0)
            rngs[i] = -log(d);
        }
        PROF_END(PROF_CACHE_EXP_RNG);
#if defined(OPT_AVX) && !defined(OPT_SVML)
    } else {
        PROF_BEGIN(PROF_CACHE_RNG);
        getRandomDoubles(rngs, numberRNGs, true, true);
        for (int i=0; i<numberRNGs; i++)
            rngs[i] = -log(rngs[i]);
        PROF_END(PROF_CACHE_RNG);
    }
#endif
#if defined(OPT_AVX) && defined(OPT_SVML)
    } else {
        PROF_BEGIN(PROF_CACHE_RNG);
        getRandomDoubles(rngs, numberRNGs, true, true);
        avxd minusone = _mm256_set1_pd(-1.0);
        for (int i=0; i<numberRNGs; i+=DOUBLES_PER_AVX)
            _mm256_store_pd(&rngs[i],_mm256_mul_pd(minusone,_mm256_log_pd(_mm256_load_pd(&rngs[i]))));
        PROF_END(PROF_CACHE_RNG);
    }
#endif
}

void XORShift::getNormRandomDoubles(double * rngs, int numberRNGs, bool bufferAvxAligned)
{
    PROF_BEGIN(PROF_CACHE_NORM_RNG);
    // Generate an even number of rngs that does not exceed the buffer size.
    int i;
    for (i=0; i<(numberRNGs>>1)<<1; i+=2)
    {
        // Generate two uniform random numbers.
        uint64_t r;
        while ((r=(uint64_t)getRandom()) == 0);
        double d1 = ((double)r)*(2.328306435997e-10); //1/((2^32)+1) range (0.0 1.0)
        while ((r=(uint64_t)getRandom()) == 0);
        double d2 = ((double)r)*(2.328306435997e-10 * 6.2831853071795860); //1/((2^32)+1)*PI range (0.0 2PI)

        // Transform using Box-Muller.
        double s = sqrt(-2.0 * log(d1));
        #if defined(LINUX)
        sincos(d2, &rngs[i], &rngs[i+1]);
        rngs[i] *= s;
        rngs[i+1] *= s;
        #else
        rngs[i] = s * sin(d2);
        rngs[i+1] = s * cos(d2);
        #endif
    }

    // If there is one more number to generate, do so and discard the second.
    if (i<numberRNGs)
    {
        // Generate two uniform random numbers.
        uint64_t r;
        while ((r=(uint64_t)getRandom()) == 0);
        double d1 = ((double)r)*(2.328306435997e-10); //1/((2^32)+1) range (0.0 1.0)
        while ((r=(uint64_t)getRandom()) == 0);
        double d2 = ((double)r)*(2.328306435997e-10 * 6.2831853071795860); //1/((2^32)+1)*PI range (0.0 2PI)

        // Transform using Box-Muller.
        isNextGaussianValid = true;
        double s = sqrt(-2.0 * log(d1));
        #if defined(LINUX)
        sincos(d2, &rngs[i], &rngs[i+1]);
        rngs[i] *= s;
        rngs[i+1] *= s;
        #else
        rngs[i] = s * sin(d2);
        rngs[i+1] = s * cos(d2);
        #endif
    }
    PROF_END(PROF_CACHE_NORM_RNG);
}

}
}
