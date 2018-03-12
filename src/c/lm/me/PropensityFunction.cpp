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
#include <map>
#include <cstdlib>
#include <string>
#include <vector>

#include "lm/ClassFactory.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/me/PropensityFunction.h"

using std::list;
using std::map;
using std::string;
using std::vector;

namespace lm {
namespace me {

PropensityFunctionFactory::PropensityFunctionFactory()
{
    // Get a list of all the propensity function collections that have been registered.
    list<string> collections = lm::ClassFactory::getInstance().getAllSubclasses("lm::me::PropensityFunctionCollection");

    for (list<string>::iterator it=collections.begin(); it != collections.end(); it++)
    {
        PropensityFunctionCollection* c = (PropensityFunctionCollection*)lm::ClassFactory::getInstance().allocateObjectOfClass("lm::me::PropensityFunctionCollection", *it);
        list<PropensityFunctionDefinition> defs = c->getPropensityFunctionDefinitions();
        for (list<PropensityFunctionDefinition>::iterator it2=defs.begin(); it2 != defs.end(); it2++)
        {
            if (functions.count(it2->type) == 0)
            {
                functions[it2->type] = *it2;
                functionSources[it2->type] = *it;
            }
            else
            {
                Print::printf(Print::WARNING, "Multiple definitions for propensity function %d, ignoring function from class %s", it2->type, it->c_str());
            }
        }
    }
}

PropensityFunctionFactory::~PropensityFunctionFactory()
{
}

PropensityFunction* PropensityFunctionFactory::createPropensityFunction(uint type, int reactionIndex, ndarray<int> S, ndarray<uint> D, tuple<double>K)
{
    if (functions.count(type) == 0)
        throw lm::InvalidArgException("type","the specified propensity function was not found",type);
    PropensityFunctionCreator f = functions[type].create;
    return (*f)(reactionIndex, S, D, K);
}

void PropensityFunctionFactory::printRegisteredFunctions(int verbosity)
{
    Print::printf(verbosity, "The following propensity functions are registered:");
    for (map<uint,PropensityFunctionDefinition>::iterator it=functions.begin(); it != functions.end(); it++)
    {
        PropensityFunctionDefinition def = it->second;
        if (def.expressions.size() == 0)
            Print::printf(verbosity, "% 5d\t%-40s\t  %-30s\tsource=%s", def.type, def.name.c_str(), "", functionSources[it->first].c_str());
        else if (def.expressions.size() == 1)
            Print::printf(verbosity, "% 5d\t%-40s\tp=%-30s\tsource=%s", def.type, def.name.c_str(), def.expressions.front().c_str(), functionSources[it->first].c_str());
        else
        {
            list<string>::iterator it2=def.expressions.begin();
            Print::printf(verbosity, "% 5d\t%-40s\tp=%-30s\tsource=%s", def.type, def.name.c_str(), it2->c_str(), functionSources[it->first].c_str());
            for (it2++; it2 != def.expressions.end(); it2++)
                Print::printf(verbosity, "                                                           p=%-30s", it2->c_str());
        }
    }
}


map<uint,PropensityFunctionDefinition> PropensityFunctionFactory::getFunctions()
{
    return functions;
}

}
}
