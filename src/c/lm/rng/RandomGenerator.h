/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 *               University of Illinois at Urbana-Champaign
 *               http://www.scs.uiuc.edu/~schulten
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
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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
 * Author(s): Elijah Roberts
 */

#ifndef LM_RNG_RANDOMGENERATOR_H_
#define LM_RNG_RANDOMGENERATOR_H_

#include "lm/Types.h"

namespace lm {
namespace rng {

class RandomGenerator
{
public:
    enum Distributions
    {
       NONE = 0x00,
       UNIFORM = 0x01,
       EXPONENTIAL = 0x02,
       NORMAL = 0x04,
       ALL=0xFF
    };

public:
    RandomGenerator(uint32_t seedTop, uint32_t seedBottom, Distributions availableDists=(Distributions)(ALL));
    virtual ~RandomGenerator() {}
    virtual uint64_t getSeed() {return seed;}

    virtual uint32_t getRandom()=0;
    virtual double getRandomDouble()=0;
    virtual double getExpRandomDouble()=0;
    virtual double getNormRandomDouble()=0;
    virtual void getRandomDoubles(double * rngs, int numberRNGs);
    virtual void getExpRandomDoubles(double * rngs, int numberRNGs);
    virtual void getNormRandomDoubles(double * rngs, int numberRNGs);

protected:
    uint64_t seed;
    Distributions availableDists;
};

}
}

#endif
