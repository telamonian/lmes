/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
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
 * Author(s): Elijah Roberts
 */

#include <cmath>
#include <list>
 
#include "lm/ClassFactory.h"
#include "lm/Exceptions.h"
#include "lm/me/PropensityFunction.h"

#include "custom_propensity_functions.h"

using std::list;

extern "C" {
    void registerClasses(ExternalClassDefinitions* definitions)
    {
        definitions->numberClasses = 1;
        definitions->baseClassNames = new const char*[definitions->numberClasses];
        definitions->classNames = new const char*[definitions->numberClasses];
        definitions->allocators = new ClassAllocator[definitions->numberClasses];
        
        definitions->baseClassNames[0] = "lm::me::PropensityFunction";
        definitions->classNames[0] = "lm::example::MyHillPropensity";
        definitions->allocators[0] = &lm::example::CustomPropensityFunctions::allocateObject;
        
        //, "lm::cme::CMEOrderParameters", 
    }
}

namespace lm {
namespace example {

class MyHillPropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 9999;

    MyHillPropensity(uint si, uint xi, uint x0, double k0, double k1, double h)
    :PropensityFunction(REACTION_TYPE,1),si(si),xi(xi),x0h(pow(x0,h)),k0(k0),dk(k1-k0),h(h) {}
    
    uint si;
    uint xi;
    double x0h;
    double k0;
    double dk;
    double h;

    void changeVolume(double volumeMultiplier) {}
    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        uint x = speciesCounts[xi];
        double xh = pow(x,h);
        double k = k0+((dk*xh)/(xh+x0h));
        return ((double)speciesCounts[si]) * k;
    }

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple sd = getSpecificDependencies(reactionIndex, D, 1);
        utuple xd = getSpecificDependencies(reactionIndex, D, 2);
        if (sd.len != 1) throw InvalidArgException("D", "MyHillPropensity needs one species dependency, had",sd.len);
        if (xd.len != 1) throw InvalidArgException("D", "MyHillPropensity needs one function dependency, had",xd.len);

        // Find the rate costant.
        if (k.len != 4)  throw InvalidArgException("k", "MyHillPropensity needs four parameters, had",k.len);

        return new MyHillPropensity(sd[0], xd[0], lround(k[0]), k[1], k[2], k[3]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, &create);
    }
};

void* CustomPropensityFunctions::allocateObject()
{
    return new CustomPropensityFunctions();
}

CustomPropensityFunctions::CustomPropensityFunctions()
{
}

CustomPropensityFunctions::~CustomPropensityFunctions()
{
}

list<lm::me::PropensityFunctionDefinition> CustomPropensityFunctions::getPropensityFunctionDefinitions()
{
    list<lm::me::PropensityFunctionDefinition> defs;
    defs.push_back(MyHillPropensity::registerFunction());
    return defs;
}

}
}
