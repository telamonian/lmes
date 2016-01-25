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

#include <list>
#include <string>

#include "lm/ClassFactory.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/me/PropensityFunction.h"

using std::list;
using std::list;

namespace lm {
namespace me {

PropensityFunctions::PropensityFunctions()
{
    // Get a list of all the propensity function collections that have been registered.
    list<string> collections = lm::ClassFactory::getInstance().getAllSubclasses("lm::me::PropensityFunctionCollection");

    for (list<string>::iterator it=collections.begin(); it != collections.end(); it++)
    {
        PropensityFunctionCollection* c = (PropensityFunctionCollection*)lm::ClassFactory::getInstance().allocateObjectOfClass("lm::me::PropensityFunctionCollection", *it);
        list<PropensityDefinition> defs = c->getPropensityDefinitions();
        for (list<PropensityDefinition>::iterator it2=defs.begin(); it2 != defs.end(); it2++)
        {
            if (functions.count(it2->id) == 0)
                functions[it2->id] = *it2;
            else
                Print::printf(Print::WARNING, "Multiple definitions for propensity function %d, ignoring function from class %s", it2->id, it->c_str());
        }
    }
}

PropensityFunctions::~PropensityFunctions()
{
}

PropensityFunction PropensityFunctions::getPropensityFunction(int id)
{
    if (functions.count(id) == 1)
        return functions[id].function;
    throw lm::InvalidArgException("id","the specified propensity function was not found",id);
}

PropensityFunctionArgs* PropensityFunctions::getPropensityFunctionArgs(int id, int reactionIndex, ndarray<int> S, ndarray<uint> D, tuple<double>K)
{
    if (functions.count(id) == 0)
        throw lm::InvalidArgException("id","the specified propensity function arg creator was not found",id);

    PropensityFunctionArgsCreator f = functions[id].argsCreator;
    return (*f)(reactionIndex, S, D, K);
}

PropensityFunctionCollection::PropensityFunctionCollection()
{
}

PropensityFunctionCollection::~PropensityFunctionCollection()
{
}

}
}
