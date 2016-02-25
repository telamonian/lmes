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
#include "lm/Types.h"
#include "lm/cme/CMEOrderParameters.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/oparam/OrderParameterFunction.h"

using std::list;
using std::map;
using std::string;
using std::vector;

namespace lm {
namespace cme {

bool CMEOrderParameters::registered=CMEOrderParameters::registerClass();

bool CMEOrderParameters::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::oparam::OrderParameterFunctionCollection", "lm::cme::CMEOrderParameters", &CMEOrderParameters::allocateObject);
    return true;
}

void* CMEOrderParameters::allocateObject()
{
    return new CMEOrderParameters();
}

CMEOrderParameters::CMEOrderParameters()
{
}

CMEOrderParameters::~CMEOrderParameters()
{
}


class LinearCombinationOrderParameter : public lm::oparam::OrderParameterFunction
{
public:
    static const uint OPARAM_TYPE = 0;

    LinearCombinationOrderParameter(size_t size, uint* speciesIndex, double* speciesCoefficient)
    :OrderParameterFunction(OPARAM_TYPE),size(size),speciesIndex(speciesIndex),speciesCoefficient(speciesCoefficient) {}
    ~LinearCombinationOrderParameter()
    {
        if (speciesIndex != NULL) delete[] speciesIndex; speciesIndex = NULL;
        if (speciesCoefficient != NULL) delete[] speciesCoefficient; speciesCoefficient = NULL;
    }

    size_t size;
    uint* speciesIndex;
    double* speciesCoefficient;

    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        double value=0.0;
        for (size_t i=0; i<size; i++)
            value += speciesCoefficient[i]*double(speciesCounts[speciesIndex[i]]);
        return value;
    }

#ifdef OPT_AVX
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd value = _mm256_set1_pd(0.0);
        for (size_t i=0; i<size; i++)
            value = _mm256_add_pd(value, _mm256_mul_pd(_mm256_set1_pd(speciesCoefficient[i]), _mm256_load_pd(&speciesCounts[speciesIndex[i]*DOUBLES_PER_AVX])));
        return value;
    }
#endif

    static OrderParameterFunction* create(const lm::io::OrderParameters::OrderParameter& op)
    {
        if (op.type() != OPARAM_TYPE)
            throw lm::InvalidArgException("op.type", "Mismatch of types during creation of linear order parameter function",op.type(), OPARAM_TYPE);
        if (op.species_ids_size() != op.species_coefficients_size())
            throw lm::InvalidArgException("op.size", "Mismatch of sizes during creation of linear order parameter function",op.species_ids_size(), op.species_coefficients_size());

        size_t size = op.species_ids_size();
        uint* speciesIndex = new uint[size];
        double* speciesCoefficient = new double[size];
        for (size_t i=0; i<size; i++)
        {
            speciesIndex[i] = op.species_ids(i);
            speciesCoefficient[i] = op.species_coefficients(i);
        }

        return new LinearCombinationOrderParameter(size, speciesIndex, speciesCoefficient);
    }

    static lm::oparam::OrderParameterFunctionDefinition registerFunction()
    {
        return lm::oparam::OrderParameterFunctionDefinition(OPARAM_TYPE, &create);
    }
};

class TwoSpeciesOrderParameter : public lm::oparam::OrderParameterFunction
{
public:
    static const uint OPARAM_TYPE = 2;

    TwoSpeciesOrderParameter(uint s1, uint s2, double k1, double k2):OrderParameterFunction(OPARAM_TYPE),s1(s1),s2(s2),k1(k1),k2(k2) {}
    uint s1, s2;
    double k1, k2;

    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        return k1*double(speciesCounts[s1]) + k2*double(speciesCounts[s2]);
    }

#ifdef OPT_AVX
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd value = _mm256_mul_pd(_mm256_set1_pd(k1), _mm256_load_pd(&speciesCounts[s1*DOUBLES_PER_AVX]));
        return _mm256_fmadd_pd(_mm256_set1_pd(k2), _mm256_load_pd(&speciesCounts[s2*DOUBLES_PER_AVX]), value);
    }
#endif

    static OrderParameterFunction* create(const lm::io::OrderParameters::OrderParameter& op)
    {
        if (op.type() != OPARAM_TYPE)
            throw lm::InvalidArgException("op.type", "Mismatch of types during creation of two species order parameter function",op.type(), OPARAM_TYPE);
        if (op.species_ids_size() != 2 && op.species_coefficients_size() != 2)
            throw lm::InvalidArgException("op.size", "Mismatch of sizes during creation of two species order parameter function",op.species_ids_size(), op.species_coefficients_size());

        return new TwoSpeciesOrderParameter(op.species_ids(0), op.species_ids(1), op.species_coefficients(0), op.species_coefficients(1));
    }

    static lm::oparam::OrderParameterFunctionDefinition registerFunction()
    {
        return lm::oparam::OrderParameterFunctionDefinition(OPARAM_TYPE, &create);
    }
};


list<lm::oparam::OrderParameterFunctionDefinition> CMEOrderParameters::getOrderParameterFunctionDefinitions()
{
    list<lm::oparam::OrderParameterFunctionDefinition> defs;
    defs.push_back(LinearCombinationOrderParameter::registerFunction());
    defs.push_back(TwoSpeciesOrderParameter::registerFunction());
    return defs;
}



}
}
