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
 * Author(s): Elijah Roberts, Max Klein
 */

#include <list>
#include <map>
#include <string>
#include <vector>

#include "lm/ClassFactory.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/oparam/OrderParameterFunction.h"

using std::list;
using std::map;
using std::string;
using std::vector;

namespace lm {
namespace oparam {

#ifdef OPT_AVX
avxd OrderParameterFunction::calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
{
    double* results;
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&results, DOUBLES_PER_AVX*sizeof(double), DOUBLES_PER_AVX*sizeof(double)));
    int* intSpeciesCounts = new int[numberSpecies];
    for (uint i=0; i<DOUBLES_PER_AVX; i++)
    {
        for (uint j=0; j<numberSpecies; j++)
        {
            intSpeciesCounts[j] = (int)(speciesCounts[j*DOUBLES_PER_AVX+i]+0.5);
        }
        results[i] = calculate(((double*)&time)[i], intSpeciesCounts, numberSpecies);
    }
    avxd ret = _mm256_load_pd(results);
    delete[] intSpeciesCounts;
    free(results);
    return ret;
}
#endif

OrderParameterFunctionFactory::OrderParameterFunctionFactory()
{
    // Get a list of all the order parameter function collections that have been registered.
    list<string> collections = lm::ClassFactory::getInstance().getAllSubclasses("lm::oparam::OrderParameterFunctionCollection");

    for (list<string>::iterator it=collections.begin(); it != collections.end(); it++)
    {
        OrderParameterFunctionCollection* c = (OrderParameterFunctionCollection*)lm::ClassFactory::getInstance().allocateObjectOfClass("lm::oparam::OrderParameterFunctionCollection", *it);
        list<OrderParameterFunctionDefinition> defs = c->getOrderParameterFunctionDefinitions();
        for (list<OrderParameterFunctionDefinition>::iterator it2=defs.begin(); it2 != defs.end(); it2++)
        {
            if (functions.count(it2->type) == 0)
                functions[it2->type] = *it2;
            else
                Print::printf(Print::WARNING, "Multiple definitions for order parameter function %d, ignoring function from class %s", it2->type, it->c_str());
        }
    }
}

OrderParameterFunctionFactory::~OrderParameterFunctionFactory()
{
}

OrderParameterFunction* OrderParameterFunctionFactory::createOrderParameterFunction(const lm::io::OrderParameters::OrderParameter& op)
{
    if (functions.count(op.type()) == 0)
        throw lm::InvalidArgException("op.type","the specified order parameter function was not found",op.type());
    OrderParameterFunctionCreator f = functions[op.type()].create;
    return (*f)(op);
}

OrderParameterFunctionCollection::OrderParameterFunctionCollection()
{
}

OrderParameterFunctionCollection::~OrderParameterFunctionCollection()
{
}

}
}
