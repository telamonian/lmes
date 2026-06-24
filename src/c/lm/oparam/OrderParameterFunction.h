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

#ifndef LM_OPARAM_ORDERPARAMETERFUNCTION_H
#define LM_OPARAM_ORDERPARAMETERFUNCTION_H

#include <limits>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "lm/Types.h"
#include "lm/input/OrderParameters.pb.h"

using std::list;
using std::map;
using std::string;
using std::vector;


namespace lm {
namespace oparam {

// The base class for any propensity function.
class OrderParameterFunction
{
public:
    OrderParameterFunction(const uint type):type(type){}
    virtual ~OrderParameterFunction() {}
    uint getType() const {return type;}
    virtual double calculate(const double time, const int* speciesCounts, const uint numberSpecies)const=0;
#ifdef OPT_AVX
    virtual avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const;
#endif

protected:
    const uint type;
};

// The type definition for a function to create the propensity function.
typedef OrderParameterFunction* (*OrderParameterFunctionCreator)(const lm::input::OrderParameter& msg);

struct OrderParameterFunctionDefinition
{
    OrderParameterFunctionDefinition():type(std::numeric_limits<uint>::max()),create(NULL){}
    OrderParameterFunctionDefinition(uint type, OrderParameterFunctionCreator create):type(type),create(create){}
    OrderParameterFunctionDefinition(const OrderParameterFunctionDefinition& p):type(p.type),create(p.create){}
    uint type;
    OrderParameterFunctionCreator create;
};

class OrderParameterFunctionFactory
{
public:
    OrderParameterFunctionFactory();
    ~OrderParameterFunctionFactory();
    OrderParameterFunction* createOrderParameterFunction(const lm::input::OrderParameter& msg);

private:
    map<uint,OrderParameterFunctionDefinition> functions;
};

// The base class for a collection of propensity functions.
class OrderParameterFunctionCollection
{
public:
    OrderParameterFunctionCollection();
    virtual ~OrderParameterFunctionCollection();
    virtual list<OrderParameterFunctionDefinition> getOrderParameterFunctionDefinitions()=0;
};

}
}

#endif // LM_OPARAM_ORDERPARAMETERFUNCTION_H
