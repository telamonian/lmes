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

#ifndef LM_ME_PROPENSITYFUNCTION_H
#define LM_ME_PROPENSITYFUNCTION_H

#include <list>
#include <map>
#include <string>

#include "lm/Types.h"

using std::list;
using std::map;
using std::string;


// The base class for any propensity function arguemnts.
struct PropensityFunctionArgs
{
    virtual ~PropensityFunctionArgs() {}
};

// The type definition for a propensity function.
typedef double (*PropensityFunction)(double time, int* speciesCounts, PropensityFunctionArgs* args);

// The type definition for a function to create the propensity argumnets.
typedef PropensityFunctionArgs* (*PropensityFunctionArgsCreator)(int reactionIndex, ndarray<int> S, ndarray<uint> D, ndarray<double>K);

namespace lm {
namespace me {

struct PropensityDefinition
{
    PropensityDefinition():id(-1),function(NULL),argsCreator(NULL){}
    PropensityDefinition(int id, PropensityFunction function, PropensityFunctionArgsCreator argsCreator):id(id),function(function),argsCreator(argsCreator){}
    PropensityDefinition(const PropensityDefinition& p):id(p.id),function(p.function),argsCreator(p.argsCreator){}
    int id;
    PropensityFunction function;
    PropensityFunctionArgsCreator argsCreator;
};

class PropensityFunctions
{
public:
    PropensityFunctions();
    ~PropensityFunctions();
    PropensityFunction getPropensityFunction(int id);
    PropensityFunctionArgs* getPropensityFunctionArgs(int id, int reactionIndex, ndarray<int> S, ndarray<uint> D, ndarray<double>K);

private:
    map<int,PropensityDefinition> functions;
};

// The base class for a collection of propensity functions.
class PropensityFunctionCollection
{
public:
    PropensityFunctionCollection();
    virtual ~PropensityFunctionCollection();
    virtual list<PropensityDefinition> getPropensityDefinitions()=0;
};

}
}

#endif // LM_ME_PROPENSITYFUNCTION_H
