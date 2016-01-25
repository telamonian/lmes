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

struct ZerothOrderPropensity : public PropensityFunctionArgs
{
    ZerothOrderPropensity(uint si, double k) :si(si),k(k) {}
    uint si;
    double k;

    static PropensityFunctionArgs* init(const int reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the dependencies.
        uint numberDependencies = 0;
        uint dependency = 0;
        for (uint i=0; i<D.shape[0]; i++)
        {
            if (D[utuple(i,reactionIndex)] == 1)
            {
                numberDependencies++;
                dependency = i;
            }
        }
        if (numberDependencies != 0) throw InvalidArgException("D", "zeroth order propensity had invalid number of dependencies",numberDependencies);

        // Find the rate costant.
        if (k.len < 1)  throw InvalidArgException("k", "zeroth order propensity needs one rate constant",k.len);

        return new ZerothOrderPropensity(dependency,k[0]);
    }

    static double calculate(const double time, const int* speciesCounts, const PropensityFunctionArgs* pargs)
    {
        ZerothOrderPropensity * args = (ZerothOrderPropensity*)pargs;
        return args->k;
    }
};

struct FirstOrderPropensity : public PropensityFunctionArgs
{
    FirstOrderPropensity(uint si, double k) :si(si),k(k) {}
    uint si;
    double k;

    static PropensityFunctionArgs* init(const int reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the dependencies.
        uint numberDependencies = 0;
        uint dependency = 0;
        for (uint i=0; i<D.shape[0]; i++)
        {
            if (D[utuple(i,reactionIndex)] == 1)
            {
                numberDependencies++;
                dependency = i;
            }
        }
        if (numberDependencies != 1) throw InvalidArgException("D", "first order propensity had invalid number of dependencies",numberDependencies);

        // Find the rate costant.
        if (k.len < 1)  throw InvalidArgException("k", "first order propensity needs one rate constant",k.len);

        return new FirstOrderPropensity(dependency,k[0]);
    }

    static double calculate(const double time, const int* speciesCounts, const PropensityFunctionArgs* pargs)
    {
        FirstOrderPropensity * args = (FirstOrderPropensity*)pargs;
        return args->k * (double)speciesCounts[args->si];
    }
};


list<PropensityDefinition> DefaultPropensityFunctions::getPropensityDefinitions()
{
    list<PropensityDefinition> defs;
    defs.push_back(PropensityDefinition(0, &ZerothOrderPropensity::calculate, &ZerothOrderPropensity::init));
    defs.push_back(PropensityDefinition(1, &FirstOrderPropensity::calculate, &FirstOrderPropensity::init));
    return defs;
}



}
}
