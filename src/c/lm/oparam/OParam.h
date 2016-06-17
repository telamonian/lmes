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
#ifndef LM_OPARAM_OPARAM
#define LM_OPARAM_OPARAM

#include "lm/input/OrderParameters.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/Types.h"

namespace lm {
namespace oparam {

class OParam
{
public:
    OParam();
    virtual ~OParam();
    virtual void init(const lm::input::OrderParameter& opRef);
    virtual void initValues(int* speciesCounts, double time);
    virtual double calc(const int* speciesCounts, double time) const = 0;
    virtual double calc(const lm::io::TrajectoryState& state) const;
    virtual double calcAndStore(int* speciesCounts, double time);
    double get() {return val;}
    double getPrev() {return prevVal;}
    void set(double newVal) {val = newVal;}

protected:
    lm::input::OrderParameter* op;
    double val;
    double prevVal;
};

class OParamLinear : public OParam
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

    OParamLinear();
    virtual ~OParamLinear() {}
    virtual void init(const lm::input::OrderParameter& opRef);
    virtual double calc(const int* speciesCounts, double time) const;
public:
    uint size;
    const uint* speciesID;
    const double* speciesCoefficient;
};

class OParamTwoSpecies : public OParam
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

    OParamTwoSpecies();
    virtual ~OParamTwoSpecies() {}
    virtual void init(const lm::input::OrderParameter& opRef);
    virtual double calc(const int* speciesCounts, double time) const;
public:
    uint s1, s2;
    double k1, k2;
};

}
}

#endif /* LM_OPARAM_OPARAM */
