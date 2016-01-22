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

#include "lm/ClassFactory.h"
#include "lm/me/DefaultPropensityFunctions.h"
#include "lm/me/PropensityFunction.h"

namespace lm {
namespace me {

bool DefaultPropensityFunctions::registered=DefaultPropensityFunctions::registerClass();

bool DefaultPropensityFunctions::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::me::PropensityFunctionCollection", "lm::me::DefaultPropensityFunctions", &DefaultPropensityFunctions::allocateObject);
    return true;
}

void* DefaultPropensityFunctions::allocateObject()
{
    return new DefaultPropensityFunctions();
}

DefaultPropensityFunctions::DefaultPropensityFunctions()
{
}

DefaultPropensityFunctions::~DefaultPropensityFunctions()
{
}

struct FirstOrderPropensity : public PropensityFunctionArgs
{
    FirstOrderPropensity(uint si, double k) :si(si),k(k) {}
    uint si;
    double k;

    static PropensityFunctionArgs* init(int reactionIndex, ndarray<int> S, ndarray<uint> D, ndarray<double>K)
    {
        return new FirstOrderPropensity(0,0.0);
    }

    static double calculate(double time, int* speciesCounts, PropensityFunctionArgs* pargs)
    {
        FirstOrderPropensity * args = (FirstOrderPropensity*)pargs;
        return args->k * (double)speciesCounts[args->si];
    }
};


list<PropensityDefinition> DefaultPropensityFunctions::getPropensityDefinitions()
{
    list<PropensityDefinition> defs;
    defs.push_back(PropensityDefinition(1, &FirstOrderPropensity::calculate, &FirstOrderPropensity::init));
    return defs;
}



}
}
