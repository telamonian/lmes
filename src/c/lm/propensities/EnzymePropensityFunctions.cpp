/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 * 		 http://biophysics.jhu.edu/roberts/
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
#include <string>
#include <vector>
#include "math.h"

#include "lm/ClassFactory.h"
#include "lm/me/PropensityFunction.h"
#include "lm/propensities/EnzymePropensityFunctions.h"


namespace lm {
namespace propensities {

bool EnzymePropensityFunctions::registered=EnzymePropensityFunctions::registerClass();

bool EnzymePropensityFunctions::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::me::PropensityFunctionCollection", "lm::propensities::EnzymePropensityFunctions", &EnzymePropensityFunctions::allocateObject);
    return true;
}

void* EnzymePropensityFunctions::allocateObject()
{
    return new EnzymePropensityFunctions();
}

EnzymePropensityFunctions::EnzymePropensityFunctions()
{
}

EnzymePropensityFunctions::~EnzymePropensityFunctions()
{
}

class TwoSubstrateBindingPropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 4000;

    TwoSubstrateBindingPropensity(uint xi, uint si, double k) :PropensityFunction(REACTION_TYPE,3),xi(xi),si(si),k(k) {}
    uint xi;
    uint si;
    double k;

    void changeVolume(double volumeMultiplier) {k/=(volumeMultiplier*volumeMultiplier);}
    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        int x = speciesCounts[xi];
        int s = speciesCounts[si];
        return k * double(x*s*(s-1));
    }

#ifdef OPT_AVX
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd x = _mm256_load_pd(&speciesCounts[xi*DOUBLES_PER_AVX]);
        avxd s = _mm256_load_pd(&speciesCounts[si*DOUBLES_PER_AVX]);
        avxd sm1 = _mm256_sub_pd(s, _mm256_set1_pd(1.0));
        return _mm256_mul_pd(_mm256_set1_pd(k), _mm256_mul_pd(x,_mm256_mul_pd(s,sm1)));
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the x dependency.
        utuple d1 = getSpecificDependencies(reactionIndex, D, 1);
        if (d1.len != 1) throw InvalidArgException("D", "TwoSubstrateBindingPropensity had invalid number of x dependencies",d1.len);

        // Find the substrate dependency.
        utuple d2 = getSpecificDependencies(reactionIndex, D, 2);
        if (d2.len != 1) throw InvalidArgException("D", "TwoSubstrateBindingPropensity had invalid number of substrate dependencies",d2.len);

        // Find the rate costant.
        if (k.len != 1)  throw InvalidArgException("k", "TwoSubstrateBindingPropensity needs one rate constant",k.len);

        return new TwoSubstrateBindingPropensity(d1[0],d2[0],k[0]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        const char* expressions[] = {"k1 * x1 * x2 * x2", "k1 * x1 * x2^2", "k1 * x1 * x2 * (x2-1)", NULL};
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "TwoSubstrateBindingPropensity", expressions, &create);
    }
};

class ThreeSubstrateBindingPropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 4001;

    ThreeSubstrateBindingPropensity(uint xi, uint si, double k) :PropensityFunction(REACTION_TYPE,4),xi(xi),si(si),k(k) {}
    uint xi;
    uint si;
    double k;

    void changeVolume(double volumeMultiplier) {k/=(volumeMultiplier*volumeMultiplier*volumeMultiplier);}

    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        int x = speciesCounts[xi];
        int s = speciesCounts[si];
        return k * double(x*s*(s-1)*(s-2));
    }

#ifdef OPT_AVX
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd x = _mm256_load_pd(&speciesCounts[xi*DOUBLES_PER_AVX]);
        avxd s = _mm256_load_pd(&speciesCounts[si*DOUBLES_PER_AVX]);
        avxd sm1 = _mm256_sub_pd(s, _mm256_set1_pd(1.0));
        avxd sm2 = _mm256_sub_pd(s, _mm256_set1_pd(2.0));
        return _mm256_mul_pd(_mm256_set1_pd(k), _mm256_mul_pd(x,_mm256_mul_pd(s,_mm256_mul_pd(sm1,sm2))));
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the x dependency.
        utuple d1 = getSpecificDependencies(reactionIndex, D, 1);
        if (d1.len != 1) throw InvalidArgException("D", "ThreeSubstrateBindingPropensity had invalid number of x dependencies",d1.len);

        // Find the substrate dependency.
        utuple d2 = getSpecificDependencies(reactionIndex, D, 2);
        if (d2.len != 1) throw InvalidArgException("D", "ThreeSubstrateBindingPropensity had invalid number of substrate dependencies",d2.len);

        // Find the rate costant.
        if (k.len != 1)  throw InvalidArgException("k", "ThreeSubstrateBindingPropensity needs one rate constant",k.len);

        return new ThreeSubstrateBindingPropensity(d1[0],d2[0],k[0]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        const char* expressions[] = {"k1 * x1 * x2 * x2 * x2", "k1 * x1 * x2^3", "k1 * x1 * x2 * (x2-1) * (x2-2)", NULL};
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "ThreeSubstrateBindingPropensity", expressions, &create);
    }
};

class FirstOrderMichaelisMenten : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 4002;

    FirstOrderMichaelisMenten(uint s1, double Rm, double Km) :PropensityFunction(REACTION_TYPE,1),s1(s1),Rm(Rm),Km(Km) {}
    uint s1;
    double Rm;
    double Km;

    void changeVolume(double volumeMultiplier) {}
    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        return (Rm*double(speciesCounts[s1]))/(Km + double(speciesCounts[s1]));
    }

#ifdef OPT_AVX
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        return naiveCalculateAvx(this, time, speciesCounts, numberSpecies);
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple sd1 = getSpecificDependencies(reactionIndex, D, 1);
        if (sd1.len != 1) throw InvalidArgException("D", "FirstOrderMichaelisMenten needs one species dependencies, had", sd1.len);

        // Find the rate constant.
        if (k.len != 2) throw InvalidArgException("k", "FirstOrderMichaelisMenten needs two parameters, had", k.len);

        return new FirstOrderMichaelisMenten(sd1[0], k[0], k[1]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "FirstOrderMichaelisMenten", "(k1 * x1) / (k2 + x1)", &create);
    }
};

list<lm::me::PropensityFunctionDefinition> EnzymePropensityFunctions::getPropensityFunctionDefinitions()
{
    list<lm::me::PropensityFunctionDefinition> defs;
    defs.push_back(TwoSubstrateBindingPropensity::registerFunction());
    defs.push_back(ThreeSubstrateBindingPropensity::registerFunction());
    defs.push_back(FirstOrderMichaelisMenten::registerFunction());
    return defs;
}

}
}
