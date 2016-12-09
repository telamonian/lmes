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

#include <limits>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "lm/Print.h"
#include "lm/Types.h"

using std::list;
using std::map;
using std::string;
using std::vector;

namespace lm {
namespace me {

// The base class for any propensity function.
class PropensityFunction
{
public:
    inline static utuple getDependencies(const uint reactionIndex, const ndarray<uint> D);
    inline static utuple getSpecificDependencies(const uint reactionIndex, const ndarray<uint> D, const uint dependencyType);
#ifdef OPT_AVX
    inline static avxd naiveCalculateAvx(const PropensityFunction* fn, const avxd time, const double* speciesCounts, const uint numberSpecies);
#endif

public:
    PropensityFunction(const uint type, uint order):type(type),order(order){}
    virtual ~PropensityFunction() {}
    uint getType() const {return type;}
    uint getOrder() const {return order;}
    virtual void changeVolume(double volumeMultiplier)=0;
    virtual double calculate(const double time, const int* speciesCounts, const uint numberSpecies)const=0;
#ifdef OPT_AVX
    virtual avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const=0;
#endif

protected:
    const uint type;
    const uint order;
};

// The type definition for a function to create the propensity function.
typedef PropensityFunction* (*PropensityFunctionCreator)(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k);

struct PropensityFunctionDefinition
{
    PropensityFunctionDefinition():type(std::numeric_limits<uint>::max()),name("undefined"),expressions(),create(NULL){}
    PropensityFunctionDefinition(uint type, PropensityFunctionCreator create):type(type),name("unknown"),expressions(),create(create){}
    PropensityFunctionDefinition(uint type, string name, string expression, PropensityFunctionCreator create):type(type),name(name),expressions(1,expression),create(create){}
    PropensityFunctionDefinition(uint type, string name, const char** expressionsArray, PropensityFunctionCreator create):type(type),name(name),expressions(),create(create)
    {
        int i=0;
        while (expressionsArray[i] != NULL)
            expressions.push_back(expressionsArray[i++]);
    }
    PropensityFunctionDefinition(uint type, string name, string expression, const char** constantUnitsArray, PropensityFunctionCreator create):type(type),name(name),expressions(1,expression),create(create)
    {
        int i=0;
        while (constantUnitsArray[i] != NULL)
        {
            constantUnits.push_back(constantUnitsArray[i++]);
        }
    }
    PropensityFunctionDefinition(uint type, string name, const char** expressionsArray, const char** constantUnitsArray, PropensityFunctionCreator create):type(type),name(name),expressions(),create(create)
    {
        int i=0;
        while (expressionsArray[i] != NULL)
            expressions.push_back(expressionsArray[i++]);
        i=0;
        while (constantUnitsArray[i] != NULL)
            constantUnits.push_back(constantUnitsArray[i++]);
    }
    PropensityFunctionDefinition(const PropensityFunctionDefinition& p):type(p.type),name(p.name),expressions(p.expressions),constantUnits(p.constantUnits),create(p.create){}
    string getConstantUnits(int i)
    {
        if (i < constantUnits.size()) return constantUnits[i];
        return "1";
    }

    uint type;
    string name;
    list<string> expressions;
    vector<string> constantUnits;
    PropensityFunctionCreator create;
};

class PropensityFunctionFactory
{
public:
    PropensityFunctionFactory();
    ~PropensityFunctionFactory();
    PropensityFunction* createPropensityFunction(uint type, int reactionIndex, ndarray<int> S, ndarray<uint> D, tuple<double>k);
    void printRegisteredFunctions(int verbosity=Print::DEBUG);
    map<uint,PropensityFunctionDefinition> getFunctions();

private:
    map<uint,PropensityFunctionDefinition> functions;
    map<uint,string> functionSources;
};

// The base class for a collection of propensity functions.
class PropensityFunctionCollection
{
public:
    PropensityFunctionCollection() {}
    virtual ~PropensityFunctionCollection() {}
    virtual list<PropensityFunctionDefinition> getPropensityFunctionDefinitions()=0;
};

utuple PropensityFunction::getDependencies(const uint reactionIndex, const ndarray<uint> D)
{
    if (reactionIndex >= D.shape[1]) throw InvalidArgException("reactionIndex", "index was too large for the dependency matrix",reactionIndex,D.shape[1]);

    // Find the dependencies.
    vector<uint> dependencyVector;
    for (uint i=0; i<D.shape[0]; i++)
    {
        uint d = D[utuple(i,reactionIndex)];
        if (d != 0)
            dependencyVector.push_back(i);
    }
    return utuple(dependencyVector);
}

utuple PropensityFunction::getSpecificDependencies(const uint reactionIndex, const ndarray<uint> D, const uint dependencyType)
{
    if (reactionIndex >= D.shape[1]) throw InvalidArgException("reactionIndex", "index was too large for the dependency matrix",reactionIndex,D.shape[1]);

    // Find the dependencies.
    vector<uint> dependencyVector;
    for (uint i=0; i<D.shape[0]; i++)
    {
        uint d = D[utuple(i,reactionIndex)];
        if (d == dependencyType)
            dependencyVector.push_back(i);
    }
    return utuple(dependencyVector);
}

#ifdef OPT_AVX
avxd PropensityFunction::naiveCalculateAvx(const PropensityFunction* fn, const avxd time, const double* speciesCounts, const uint numberSpecies)
{
    avxd results;
    int* intSpeciesCounts = new int[numberSpecies];
    for (uint i=0; i<DOUBLES_PER_AVX; i++)
    {
        for (uint j=0; j<numberSpecies; j++)
        {
            intSpeciesCounts[j] = (int)(speciesCounts[j*DOUBLES_PER_AVX+i]+0.5);
        }
        ((double*)&results)[i] = fn->calculate(((double*)&time)[i], intSpeciesCounts, numberSpecies);
    }
    delete[] intSpeciesCounts;
    return results;
}
#endif


}
}

#endif // LM_ME_PROPENSITYFUNCTION_H
