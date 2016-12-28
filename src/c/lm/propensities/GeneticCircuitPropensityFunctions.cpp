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
#include <string>
#include <vector>
#include "math.h"

#include "lm/ClassFactory.h"
#include "lm/me/PropensityFunction.h"
#include "lm/propensities/GeneticCircuitPropensityFunctions.h"


namespace lm {
namespace propensities {

bool GeneticCircuitPropensityFunctions::registered=GeneticCircuitPropensityFunctions::registerClass();

bool GeneticCircuitPropensityFunctions::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::me::PropensityFunctionCollection", "lm::propensities::GeneticCircuitPropensityFunctions", &GeneticCircuitPropensityFunctions::allocateObject);
    return true;
}

void* GeneticCircuitPropensityFunctions::allocateObject()
{
    return new GeneticCircuitPropensityFunctions();
}

GeneticCircuitPropensityFunctions::GeneticCircuitPropensityFunctions()
{
}

GeneticCircuitPropensityFunctions::~GeneticCircuitPropensityFunctions()
{
}

class TimeDependentHarmonicBirthPropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 2003;

    TimeDependentHarmonicBirthPropensity(uint xi, double x0, double k, double v):PropensityFunction(REACTION_TYPE,0),xi(xi),x0(x0),k(k),v(v) {}
    uint xi;
    double x0;
    double k;
    double v;

    void changeVolume(double volumeMultiplier) {}
    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        double x = double(speciesCounts[xi]);
        double xt = x0+v*time;
        double dx = xt-x;
        double p = (dx>0)?(k*dx):(0.0);
        return p;
    }

#ifdef OPT_AVX
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd x = _mm256_load_pd(&speciesCounts[xi*DOUBLES_PER_AVX]);
        avxd xt = _mm256_add_pd(_mm256_set1_pd(x0),_mm256_mul_pd(_mm256_set1_pd(v),time));
        avxd dx = _mm256_sub_pd(xt,x);
        avxd p = _mm256_mul_pd(_mm256_set1_pd(k),dx);
        avxd comp = _mm256_cmp_pd(dx, _mm256_setzero_pd(), _CMP_GT_OQ);
        avxd p2 = _mm256_blendv_pd(_mm256_setzero_pd(),p,comp);
        //printf("birth avx t=%0.2f, x=%0.2f, xt=%0.2f, dx=%0.2f, p=%0.2e\n",((double*)&time)[0],((double*)&x)[0],((double*)&xt)[0],((double*)&dx)[0],((double*)&p2)[0]);
        return p2;
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple dependencies = getDependencies(reactionIndex, D);
        if (dependencies.len != 1) throw InvalidArgException("D", "time dependent harmonic birth propensity needs one species dependency, had",dependencies.len);

        // Find the rate costants.
        if (k.len != 3)  throw InvalidArgException("k", "time dependent birth propensity needs three parameters, had",k.len);

        return new TimeDependentHarmonicBirthPropensity(dependencies[0],k[0],k[1],k[2]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, &create);
    }
};

class TimeDependentHarmonicDeathPropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 2004;

    TimeDependentHarmonicDeathPropensity(uint xi, double x0, double k, double v):PropensityFunction(REACTION_TYPE,0),xi(xi),x0(x0),k(k),v(v) {}
    uint xi;
    double x0;
    double k;
    double v;

    void changeVolume(double volumeMultiplier) {}
    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        double x = double(speciesCounts[xi]);
        double xt = x0+v*time;
        double dx = xt-x;
        double p = (dx<0)?(-k*dx):(0.0);
        return p;
    }

#ifdef OPT_AVX
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd x = _mm256_load_pd(&speciesCounts[xi*DOUBLES_PER_AVX]);
        avxd xt = _mm256_add_pd(_mm256_set1_pd(x0),_mm256_mul_pd(_mm256_set1_pd(v),time));
        avxd dx = _mm256_sub_pd(xt,x);
        avxd comp = _mm256_cmp_pd(dx, _mm256_setzero_pd(), _CMP_LT_OQ);
        avxd p = _mm256_mul_pd(_mm256_set1_pd(-k),dx);
        avxd p2 = _mm256_blendv_pd(_mm256_setzero_pd(),p,comp);
        //printf("death avx t=%0.2f, x=%0.2f, xt=%0.2f, dx=%0.2f, p=%0.2e\n",((double*)&time)[0],((double*)&x)[0],((double*)&xt)[0],((double*)&dx)[0],((double*)&p2)[0]);
        return p2;
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple dependencies = getDependencies(reactionIndex, D);
        if (dependencies.len != 1) throw InvalidArgException("D", "time dependent harmonic birth propensity needs one species dependency, had",dependencies.len);

        // Find the rate costants.
        if (k.len != 3)  throw InvalidArgException("k", "time dependent birth propensity needs three parameters, had",k.len);

        return new TimeDependentHarmonicDeathPropensity(dependencies[0],k[0],k[1],k[2]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, &create);
    }
};

class TimeDependentQuadraticBirthPropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 2001;

    TimeDependentQuadraticBirthPropensity(uint xi, double x0, double k, double v):PropensityFunction(REACTION_TYPE,0),xi(xi),x0(x0),k(k),v(v) {}
    uint xi;
    double x0;
    double k;
    double v;

    void changeVolume(double volumeMultiplier) {}
    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        double x = double(speciesCounts[xi]);
        double xt = x0+v*time;
        double dx = xt-x;
        double p = (dx>0)?(k*dx*dx):(0.0);
        return p;
    }

#ifdef OPT_AVX
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd x = _mm256_load_pd(&speciesCounts[xi*DOUBLES_PER_AVX]);
        avxd xt = _mm256_add_pd(_mm256_set1_pd(x0),_mm256_mul_pd(_mm256_set1_pd(v),time));
        avxd dx = _mm256_sub_pd(xt,x);
        avxd p = _mm256_mul_pd(_mm256_mul_pd(_mm256_set1_pd(k),dx),dx);
        avxd comp = _mm256_cmp_pd(dx, _mm256_setzero_pd(), _CMP_GT_OQ);
        avxd p2 = _mm256_blendv_pd(_mm256_setzero_pd(),p,comp);
        //printf("birth avx t=%0.2f, x=%0.2f, xt=%0.2f, dx=%0.2f, p=%0.2e\n",((double*)&time)[0],((double*)&x)[0],((double*)&xt)[0],((double*)&dx)[0],((double*)&p2)[0]);
        return p2;
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple dependencies = getDependencies(reactionIndex, D);
        if (dependencies.len != 1) throw InvalidArgException("D", "time dependent quadratic birth propensity needs one species dependency, had",dependencies.len);

        // Find the rate costants.
        if (k.len != 3)  throw InvalidArgException("k", "time dependent birth propensity needs three parameters, had",k.len);

        return new TimeDependentQuadraticBirthPropensity(dependencies[0],k[0],k[1],k[2]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, &create);
    }
};

class TimeDependentQuadraticDeathPropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 2002;

    TimeDependentQuadraticDeathPropensity(uint xi, double x0, double k, double v):PropensityFunction(REACTION_TYPE,0),xi(xi),x0(x0),k(k),v(v) {}
    uint xi;
    double x0;
    double k;
    double v;

    void changeVolume(double volumeMultiplier) {}
    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        double x = double(speciesCounts[xi]);
        double xt = x0+v*time;
        double dx = xt-x;
        double p = (dx<0)?(k*dx*dx):(0.0);
        return p;
    }

#ifdef OPT_AVX
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd x = _mm256_load_pd(&speciesCounts[xi*DOUBLES_PER_AVX]);
        avxd xt = _mm256_add_pd(_mm256_set1_pd(x0),_mm256_mul_pd(_mm256_set1_pd(v),time));
        avxd dx = _mm256_sub_pd(xt,x);
        avxd p = _mm256_mul_pd(_mm256_mul_pd(_mm256_set1_pd(k),dx),dx);
        avxd comp = _mm256_cmp_pd(dx, _mm256_setzero_pd(), _CMP_LT_OQ);
        avxd p2 = _mm256_blendv_pd(_mm256_setzero_pd(),p,comp);
        //printf("death avx t=%0.2f, x=%0.2f, xt=%0.2f, dx=%0.2f, p=%0.2e\n",((double*)&time)[0],((double*)&x)[0],((double*)&xt)[0],((double*)&dx)[0],((double*)&p2)[0]);
        return p2;
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple dependencies = getDependencies(reactionIndex, D);
        if (dependencies.len != 1) throw InvalidArgException("D", "time dependent quadratic birth propensity needs one species dependency, had",dependencies.len);

        // Find the rate costants.
        if (k.len != 3)  throw InvalidArgException("k", "time dependent birth propensity needs three parameters, had",k.len);

        return new TimeDependentQuadraticDeathPropensity(dependencies[0],k[0],k[1],k[2]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, &create);
    }
};

class ZerothOrderKHillPropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 8007;
    static const uint REACTION_TYPE_ALTERNATE_FORMAT = 8008;

    ZerothOrderKHillPropensity(uint xi, double x0, double k0, double k1, double h) :PropensityFunction(REACTION_TYPE,0),xi(xi),x0h(pow(x0,h)),k0(k0),dk(k1-k0),h(h) {}
    uint xi;
    double x0h;
    double k0;
    double dk;
    double h;

    void changeVolume(double volumeMultiplier) {}
    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        double x = double(speciesCounts[xi]);
        double xh = pow(x,h);
        double propensity = k0+((dk*xh)/(x0h+xh));
        return propensity;
    }

#if defined(OPT_AVX) && !defined(OPT_SVML)
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        double x0 = speciesCounts[xi*DOUBLES_PER_AVX];
        double x1 = speciesCounts[xi*DOUBLES_PER_AVX+1];
        double x2 = speciesCounts[xi*DOUBLES_PER_AVX+2];
        double x3 = speciesCounts[xi*DOUBLES_PER_AVX+3];
        avxd xh = _mm256_set_pd(pow(x3,h),pow(x2,h),pow(x1,h),pow(x0,h));
        return _mm256_add_pd(_mm256_set1_pd(k0),_mm256_div_pd(_mm256_mul_pd(_mm256_set1_pd(dk),xh),_mm256_add_pd(_mm256_set1_pd(x0h),xh)));
    }
#endif
#if defined(OPT_AVX) && defined(OPT_SVML)
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd x = _mm256_load_pd(&speciesCounts[xi*DOUBLES_PER_AVX]);
        avxd xh = _mm256_pow_pd(x, _mm256_set1_pd(h));
        return _mm256_add_pd(_mm256_set1_pd(k0),_mm256_div_pd(_mm256_mul_pd(_mm256_set1_pd(dk),xh),_mm256_add_pd(_mm256_set1_pd(x0h),xh)));
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple dependencies = getDependencies(reactionIndex, D);
        if (dependencies.len != 1) throw InvalidArgException("D", "zeroth order kinetic Hill propensity needs one species dependency, had",dependencies.len);

        // Find the rate costant.
        if (k.len != 4)  throw InvalidArgException("k", "zeroth order kinetic Hill propensity needs four parameters, had",k.len);

        return new ZerothOrderKHillPropensity(dependencies[0],k[0],k[1],k[2],k[3]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        const char* expressions[] = {"k2 + (k3 - k2) * x1^k4 / (k1^h + x1^h)", NULL};
        const char* unitsForConstants[] = {"item", "item/second", "item/second", "1", NULL};
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "ZerothOrderKHillPropensity", expressions, unitsForConstants, &create);
    }

    static PropensityFunction* createAlternateFormat(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple dependencies = getDependencies(reactionIndex, D);
        if (dependencies.len != 1) throw InvalidArgException("D", "zeroth order kinetic Hill propensity needs one species dependency, had",dependencies.len);

        // Find the rate costant.
        if (k.len != 4)  throw InvalidArgException("k", "zeroth order kinetic Hill propensity needs four parameters, had",k.len);

        return new ZerothOrderKHillPropensity(dependencies[0],k[0],k[1],k[1]+k[2],k[3]);
    }

    static lm::me::PropensityFunctionDefinition registerFunctionAlternateFormat()
    {
        const char* expressions[] = {"k2 + k3 * x1^k4 / (k1^k4 + x1^k4)", "k2 + k3 * (x1^k4 / (k1^k4 + x1^k4))", NULL};
        const char* unitsForConstants[] = {"item", "item/second", "item/second", "1", NULL};
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE_ALTERNATE_FORMAT, "ZerothOrderKHillPropensity", expressions, unitsForConstants, &createAlternateFormat);
    }
};

class ZerothOrderKHillTwoSpeciesOrPropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 8009;

    ZerothOrderKHillTwoSpeciesOrPropensity(uint xi, uint yi, double x0, double y0, double k0, double k1, double h) :PropensityFunction(REACTION_TYPE,0),xi(xi),x0h(pow(x0,h)),yi(yi),y0h(pow(y0,h)),k0(k0),dk(k1-k0),h(h) {}
    uint xi;
    double x0h;
    uint yi;
    double y0h;
    double k0;
    double dk;
    double h;

    void changeVolume(double volumeMultiplier) {}
    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        double x = double(speciesCounts[xi]);
        double xh = pow(x,h);
        double y = double(speciesCounts[yi]);
        double yh = pow(y,h);
        double maxhill = fmax(xh/(x0h+xh),yh/(y0h+yh));
        double propensity = k0+dk*maxhill;
        return propensity;
    }

#if defined(OPT_AVX) && !defined(OPT_SVML)
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        double x0 = speciesCounts[xi*DOUBLES_PER_AVX];
        double x1 = speciesCounts[xi*DOUBLES_PER_AVX+1];
        double x2 = speciesCounts[xi*DOUBLES_PER_AVX+2];
        double x3 = speciesCounts[xi*DOUBLES_PER_AVX+3];
        avxd xh = _mm256_set_pd(pow(x3,h),pow(x2,h),pow(x1,h),pow(x0,h));
        avxd xhill = _mm256_div_pd(xh,_mm256_add_pd(_mm256_set1_pd(x0h),xh));
        double y0 = speciesCounts[yi*DOUBLES_PER_AVX];
        double y1 = speciesCounts[yi*DOUBLES_PER_AVX+1];
        double y2 = speciesCounts[yi*DOUBLES_PER_AVX+2];
        double y3 = speciesCounts[yi*DOUBLES_PER_AVX+3];
        avxd yh = _mm256_set_pd(pow(y3,h),pow(y2,h),pow(y1,h),pow(y0,h));
        avxd yhill = _mm256_div_pd(yh,_mm256_add_pd(_mm256_set1_pd(y0h),yh));
        avxd maxhill = _mm256_max_pd(xhill,yhill);
        return _mm256_add_pd(_mm256_set1_pd(k0),_mm256_mul_pd(_mm256_set1_pd(dk),maxhill));
    }
#endif
#if defined(OPT_AVX) && defined(OPT_SVML)
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd x = _mm256_load_pd(&speciesCounts[xi*DOUBLES_PER_AVX]);
        avxd xh = _mm256_pow_pd(x, _mm256_set1_pd(h));
        avxd xhill = _mm256_div_pd(xh,_mm256_add_pd(_mm256_set1_pd(x0h),xh));
        avxd y = _mm256_load_pd(&speciesCounts[yi*DOUBLES_PER_AVX]);
        avxd yh = _mm256_pow_pd(y, _mm256_set1_pd(h));
        avxd yhill = _mm256_div_pd(yh,_mm256_add_pd(_mm256_set1_pd(y0h),yh));
        avxd maxhill = _mm256_max_pd(xhill,yhill);
        return _mm256_add_pd(_mm256_set1_pd(k0),_mm256_mul_pd(_mm256_set1_pd(dk),maxhill));
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple d1 = getSpecificDependencies(reactionIndex, D, 1);
        if (d1.len != 1) throw InvalidArgException("D", "zeroth order kinetic Hill two-species OR propensity needs one first species dependency, had",d1.len);
        utuple d2 = getSpecificDependencies(reactionIndex, D, 2);
        if (d2.len != 1) throw InvalidArgException("D", "zeroth order kinetic Hill two-species OR propensity needs one second species dependency, had",d2.len);

        // Find the rate costant.
        if (k.len != 5)  throw InvalidArgException("k", "zeroth order kinetic Hill two-species OR propensity needs four parameters, had",k.len);

        return new ZerothOrderKHillTwoSpeciesOrPropensity(d1[0],d2[0],k[0],k[1],k[2],k[3],k[4]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        //const char* expressions[] = {"k2 + (k3 - k2) * x1^k4 / (k1^h + x1^h)", NULL};
        //const char* unitsForConstants[] = {"item", "item/second", "item/second", "1", NULL};
        //return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "ZerothOrderKHillTwoSpeciesOrPropensity", expressions, unitsForConstants, &create);
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, &create);
    }
};

list<lm::me::PropensityFunctionDefinition> GeneticCircuitPropensityFunctions::getPropensityFunctionDefinitions()
{
    list<lm::me::PropensityFunctionDefinition> defs;
    defs.push_back(TimeDependentHarmonicBirthPropensity::registerFunction());
    defs.push_back(TimeDependentHarmonicDeathPropensity::registerFunction());
    defs.push_back(TimeDependentQuadraticBirthPropensity::registerFunction());
    defs.push_back(TimeDependentQuadraticDeathPropensity::registerFunction());
    defs.push_back(ZerothOrderKHillPropensity::registerFunction());
    defs.push_back(ZerothOrderKHillPropensity::registerFunctionAlternateFormat());
    defs.push_back(ZerothOrderKHillTwoSpeciesOrPropensity::registerFunction());
    return defs;
}

}
}
