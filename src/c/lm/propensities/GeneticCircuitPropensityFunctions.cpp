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

class ZerothOrderImpulsePropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 2005;

    ZerothOrderImpulsePropensity(double k, double deltaK, double ti, double tf) :PropensityFunction(REACTION_TYPE,0),k(k),deltaK(deltaK),ti(ti),tf(tf) {}
    double k;
    double deltaK;
    double ti;
    double tf;

    void changeVolume(double volumeMultiplier) {k*=volumeMultiplier;}
    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        return (time>=ti&&time<=tf)?(k+deltaK):k;
    }

#ifdef OPT_AVX
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd compi = _mm256_cmp_pd(time, _mm256_set1_pd(ti), _CMP_GE_OQ);
        avxd compf = _mm256_cmp_pd(time, _mm256_set1_pd(tf), _CMP_LE_OQ);
        avxd comp = _mm256_and_pd(compi, compf);
        return _mm256_blendv_pd(_mm256_set1_pd(k), _mm256_set1_pd(k+deltaK), comp);
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple dependencies = getDependencies(reactionIndex, D);
        if (dependencies.len != 1) throw InvalidArgException("D", "zeroth order impulse propensity had invalid number of dependencies",dependencies.len);

        // Find the rate costant.
        if (k.len < 4)  throw InvalidArgException("k", "zeroth order impulse propensity needs four rate constants",k.len);

        return new ZerothOrderImpulsePropensity(k[0],k[1],k[2],k[3]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, &create);
    }
};


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
        const char* expressions[] = {"k2 + (k3 - k2) * x1^k4 / (k1^k4 + x1^k4)", "k2 + (k3 - k2) * (x1^k4 / (k1^k4 + x1^k4))", "k2 + (k3 - k2) * (x1^k4 / (x1^k4 + k1^k4))", NULL};
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

    ZerothOrderKHillTwoSpeciesOrPropensity(uint xi, uint yi, double k0, double k1, double mid_x, double h_x, double mid_y, double h_y) :PropensityFunction(REACTION_TYPE,0),xi(xi),yi(yi),k0(k0),dk(k1-k0),mid_x_h(pow(mid_x,h_x)),h_x(h_x),mid_y_h(pow(mid_y,h_y)),h_y(h_y) {}
    uint xi;
    uint yi;
    double k0;
    double dk;
    double mid_x_h;
    double h_x;
    double mid_y_h;
    double h_y;

    void changeVolume(double volumeMultiplier) {}
    double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
    {
        double x = double(speciesCounts[xi]);
        double xh = pow(x,h_x);
        double y = double(speciesCounts[yi]);
        double yh = pow(y,h_y);
        double maxhill = fmax(xh/(mid_x_h+xh),yh/(mid_y_h+yh));
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
        avxd xh = _mm256_set_pd(pow(x3,h_x),pow(x2,h_x),pow(x1,h_x),pow(x0,h_x));
        avxd xhill = _mm256_div_pd(xh,_mm256_add_pd(_mm256_set1_pd(mid_x_h),xh));
        double y0 = speciesCounts[yi*DOUBLES_PER_AVX];
        double y1 = speciesCounts[yi*DOUBLES_PER_AVX+1];
        double y2 = speciesCounts[yi*DOUBLES_PER_AVX+2];
        double y3 = speciesCounts[yi*DOUBLES_PER_AVX+3];
        avxd yh = _mm256_set_pd(pow(y3,h_y),pow(y2,h_y),pow(y1,h_y),pow(y0,h_y));
        avxd yhill = _mm256_div_pd(yh,_mm256_add_pd(_mm256_set1_pd(mid_y_h),yh));
        avxd maxhill = _mm256_max_pd(xhill,yhill);
        return _mm256_add_pd(_mm256_set1_pd(k0),_mm256_mul_pd(_mm256_set1_pd(dk),maxhill));
    }
#endif
#if defined(OPT_AVX) && defined(OPT_SVML)
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd x = _mm256_load_pd(&speciesCounts[xi*DOUBLES_PER_AVX]);
        avxd xh = _mm256_pow_pd(x, _mm256_set1_pd(h_x));
        avxd xhill = _mm256_div_pd(xh,_mm256_add_pd(_mm256_set1_pd(mid_x_h),xh));
        avxd y = _mm256_load_pd(&speciesCounts[yi*DOUBLES_PER_AVX]);
        avxd yh = _mm256_pow_pd(y, _mm256_set1_pd(h_y));
        avxd yhill = _mm256_div_pd(yh,_mm256_add_pd(_mm256_set1_pd(mid_y_h),yh));
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
        if (k.len != 6)  throw InvalidArgException("k", "zeroth order kinetic Hill two-species OR propensity needs six parameters, had",k.len);

        return new ZerothOrderKHillTwoSpeciesOrPropensity(d1[0],d2[0],k[0],k[1],k[2],k[3],k[4],k[5]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        //const char* expressions[] = {"k2 + (k3 - k2) * x1^k4 / (k1^h + x1^h)", NULL};
        //const char* unitsForConstants[] = {"item", "item/second", "item/second", "1", NULL};
        //return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "ZerothOrderKHillTwoSpeciesOrPropensity", expressions, unitsForConstants, &create);
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, &create);
    }
};

class FirstOrderKHillPropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 8010;
    static const uint REACTION_TYPE_ALTERNATE_FORMAT = 8011;

    FirstOrderKHillPropensity(uint si, uint xi, double x0, double k0, double k1, double h) :PropensityFunction(REACTION_TYPE,1),si(si),xi(xi),x0h(pow(x0,h)),k0(k0),dk(k1-k0),h(h) {}
    int si;
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
        double k = k0+((dk*xh)/(x0h+xh));
        return k * double(speciesCounts[si]);
    }

#if defined(OPT_AVX) && !defined(OPT_SVML)
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        double x0 = speciesCounts[xi*DOUBLES_PER_AVX];
        double x1 = speciesCounts[xi*DOUBLES_PER_AVX+1];
        double x2 = speciesCounts[xi*DOUBLES_PER_AVX+2];
        double x3 = speciesCounts[xi*DOUBLES_PER_AVX+3];
        avxd xh = _mm256_set_pd(pow(x3,h),pow(x2,h),pow(x1,h),pow(x0,h));
        avxd k = _mm256_add_pd(_mm256_set1_pd(k0),_mm256_div_pd(_mm256_mul_pd(_mm256_set1_pd(dk),xh),_mm256_add_pd(_mm256_set1_pd(x0h),xh)));
        return _mm256_mul_pd(k, _mm256_load_pd(&speciesCounts[si*DOUBLES_PER_AVX]));
    }
#endif
#if defined(OPT_AVX) && defined(OPT_SVML)
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd x = _mm256_load_pd(&speciesCounts[xi*DOUBLES_PER_AVX]);
        avxd xh = _mm256_pow_pd(x, _mm256_set1_pd(h));
        avxd k = _mm256_add_pd(_mm256_set1_pd(k0),_mm256_div_pd(_mm256_mul_pd(_mm256_set1_pd(dk),xh),_mm256_add_pd(_mm256_set1_pd(x0h),xh)));
        return _mm256_mul_pd(k, _mm256_load_pd(&speciesCounts[si*DOUBLES_PER_AVX]));
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple d1 = getSpecificDependencies(reactionIndex, D, 1);
        if (d1.len != 1) throw InvalidArgException("D", "first order kinetic Hill propensity needs one first species dependency, had",d1.len);
        utuple d2 = getSpecificDependencies(reactionIndex, D, 2);
        if (d2.len != 1) throw InvalidArgException("D", "first order kinetic Hill propensity needs one second species dependency, had",d2.len);

        // Find the rate costant.
        if (k.len != 4)  throw InvalidArgException("k", "first order kinetic Hill propensity needs four parameters, had",k.len);

        return new FirstOrderKHillPropensity(d1[0],d2[0],k[0],k[1],k[2],k[3]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        const char* expressions[] = {"x1 * (k2 + (k3 - k2) * x2^k4 / (k1^k4 + x2^k4))", NULL};
        const char* unitsForConstants[] = {"item", "1/second", "1/second", "1", NULL};
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "FirstOrderKHillPropensity", expressions, unitsForConstants, &create);
    }

    static PropensityFunction* createAlternateFormat(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple d1 = getSpecificDependencies(reactionIndex, D, 1);
        if (d1.len != 1) throw InvalidArgException("D", "first order kinetic Hill propensity needs one first species dependency, had",d1.len);
        utuple d2 = getSpecificDependencies(reactionIndex, D, 2);
        if (d2.len != 1) throw InvalidArgException("D", "first order kinetic Hill propensity needs one second species dependency, had",d2.len);

        // Find the rate costant.
        if (k.len != 4)  throw InvalidArgException("k", "first order kinetic Hill propensity needs four parameters, had",k.len);

        return new FirstOrderKHillPropensity(d1[0],d2[0],k[0],k[1],k[1]+k[2],k[3]);
    }

    static lm::me::PropensityFunctionDefinition registerFunctionAlternateFormat()
    {
        const char* expressions[] = {"x1 * (k2 + k3 * x2^k4 / (k1^k4 + x2^k4))", "x1 * (k2 + k3 * (x2^k4 / (k1^k4 + x2^k4)))", NULL};
        const char* unitsForConstants[] = {"item", "1/second", "1/second", "1", NULL};
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE_ALTERNATE_FORMAT, "FirstOrderKHillPropensity", expressions, unitsForConstants, &createAlternateFormat);
    }
};

class FirstOrderNegativeKHillPropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 8012;
    static const uint REACTION_TYPE_ALTERNATE_FORMAT = 8013;

    FirstOrderNegativeKHillPropensity(uint si, uint xi, double x0, double k0, double k1, double h) :PropensityFunction(REACTION_TYPE,1),si(si),xi(xi),x0h(pow(x0,h)),k0(k0),dk(k1-k0),h(h) {}
    int si;
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
        double k = k0+((dk*x0h)/(x0h+xh));
        return k * double(speciesCounts[si]);
    }

#if defined(OPT_AVX) && !defined(OPT_SVML)
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        double x0 = speciesCounts[xi*DOUBLES_PER_AVX];
        double x1 = speciesCounts[xi*DOUBLES_PER_AVX+1];
        double x2 = speciesCounts[xi*DOUBLES_PER_AVX+2];
        double x3 = speciesCounts[xi*DOUBLES_PER_AVX+3];
        avxd xh = _mm256_set_pd(pow(x3,h),pow(x2,h),pow(x1,h),pow(x0,h));
        avxd k = _mm256_add_pd(_mm256_set1_pd(k0),_mm256_div_pd(_mm256_mul_pd(_mm256_set1_pd(dk),_mm256_set1_pd(x0h)),_mm256_add_pd(_mm256_set1_pd(x0h),xh)));
        return _mm256_mul_pd(k, _mm256_load_pd(&speciesCounts[si*DOUBLES_PER_AVX]));
    }
#endif
#if defined(OPT_AVX) && defined(OPT_SVML)
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd x = _mm256_load_pd(&speciesCounts[xi*DOUBLES_PER_AVX]);
        avxd xh = _mm256_pow_pd(x, _mm256_set1_pd(h));
        avxd k = _mm256_add_pd(_mm256_set1_pd(k0),_mm256_div_pd(_mm256_mul_pd(_mm256_set1_pd(dk),_mm256_set1_pd(x0h)),_mm256_add_pd(_mm256_set1_pd(x0h),xh)));
        return _mm256_mul_pd(k, _mm256_load_pd(&speciesCounts[si*DOUBLES_PER_AVX]));
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple d1 = getSpecificDependencies(reactionIndex, D, 1);
        if (d1.len != 1) throw InvalidArgException("D", "first order kinetic Hill propensity needs one first species dependency, had",d1.len);
        utuple d2 = getSpecificDependencies(reactionIndex, D, 2);
        if (d2.len != 1) throw InvalidArgException("D", "first order kinetic Hill propensity needs one second species dependency, had",d2.len);

        // Find the rate costant.
        if (k.len != 4)  throw InvalidArgException("k", "first order kinetic Hill propensity needs four parameters, had",k.len);

        return new FirstOrderNegativeKHillPropensity(d1[0],d2[0],k[0],k[1],k[2],k[3]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        const char* expressions[] = {"x1 * (k2 + (k3 - k2) * k1^k4 / (k1^k4 + x2^k4))", NULL};
        const char* unitsForConstants[] = {"item", "1/second", "1/second", "1", NULL};
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "FirstOrderNegativeKHillPropensity", expressions, unitsForConstants, &create);
    }

    static PropensityFunction* createAlternateFormat(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple d1 = getSpecificDependencies(reactionIndex, D, 1);
        if (d1.len != 1) throw InvalidArgException("D", "first order kinetic Hill propensity needs one first species dependency, had",d1.len);
        utuple d2 = getSpecificDependencies(reactionIndex, D, 2);
        if (d2.len != 1) throw InvalidArgException("D", "first order kinetic Hill propensity needs one second species dependency, had",d2.len);

        // Find the rate costant.
        if (k.len != 4)  throw InvalidArgException("k", "first order kinetic Hill propensity needs four parameters, had",k.len);

        return new FirstOrderNegativeKHillPropensity(d1[0],d2[0],k[0],k[1],k[1]+k[2],k[3]);
    }

    static lm::me::PropensityFunctionDefinition registerFunctionAlternateFormat()
    {
        const char* expressions[] = {"x1 * (k2 + k3 * k1^k4 / (k1^k4 + x2^k4))", "x1 * (k2 + k3 * (k1^k4 / (k1^k4 + x2^k4)))", NULL};
        const char* unitsForConstants[] = {"item", "1/second", "1/second", "1", NULL};
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE_ALTERNATE_FORMAT, "FirstOrderNegativeKHillPropensity", expressions, unitsForConstants, &createAlternateFormat);
    }
};

class SecondOrderKHillPropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 8014;
    static const uint REACTION_TYPE_ALTERNATE_FORMAT = 8015;

    SecondOrderKHillPropensity(uint s0i, uint s1i, uint xi, double x0, double k0, double k1, double h) :PropensityFunction(REACTION_TYPE,1),s0i(s1i),s1i(s1i),xi(xi),x0h(pow(x0,h)),k0(k0),dk(k1-k0),h(h) {}
    int s0i, s1i;
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
        double k = k0+((dk*xh)/(x0h+xh));
        return k * double(speciesCounts[s0i]) * double(speciesCounts[s1i]);
    }

#if defined(OPT_AVX) && !defined(OPT_SVML)
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        double x0 = speciesCounts[xi*DOUBLES_PER_AVX];
        double x1 = speciesCounts[xi*DOUBLES_PER_AVX+1];
        double x2 = speciesCounts[xi*DOUBLES_PER_AVX+2];
        double x3 = speciesCounts[xi*DOUBLES_PER_AVX+3];
        avxd xh = _mm256_set_pd(pow(x3,h),pow(x2,h),pow(x1,h),pow(x0,h));
        avxd k = _mm256_add_pd(_mm256_set1_pd(k0),_mm256_div_pd(_mm256_mul_pd(_mm256_set1_pd(dk),xh),_mm256_add_pd(_mm256_set1_pd(x0h),xh)));
        return _mm256_mul_pd(k, _mm256_mul_pd(_mm256_load_pd(&speciesCounts[s0i*DOUBLES_PER_AVX]),_mm256_load_pd(&speciesCounts[s1i*DOUBLES_PER_AVX])));
    }
#endif
#if defined(OPT_AVX) && defined(OPT_SVML)
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd x = _mm256_load_pd(&speciesCounts[xi*DOUBLES_PER_AVX]);
        avxd xh = _mm256_pow_pd(x, _mm256_set1_pd(h));
        avxd k = _mm256_add_pd(_mm256_set1_pd(k0),_mm256_div_pd(_mm256_mul_pd(_mm256_set1_pd(dk),xh),_mm256_add_pd(_mm256_set1_pd(x0h),xh)));
        return _mm256_mul_pd(k, _mm256_mul_pd(_mm256_load_pd(&speciesCounts[s0i*DOUBLES_PER_AVX]),_mm256_load_pd(&speciesCounts[s1i*DOUBLES_PER_AVX])));
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple d1 = getSpecificDependencies(reactionIndex, D, 1);
        if (d1.len != 1) throw InvalidArgException("D", "second order kinetic Hill propensity needs one first species dependency, had",d1.len);
        utuple d2 = getSpecificDependencies(reactionIndex, D, 2);
        if (d2.len != 1) throw InvalidArgException("D", "second order kinetic Hill propensity needs one second species dependency, had",d2.len);
        utuple d3 = getSpecificDependencies(reactionIndex, D, 3);
        if (d3.len != 1) throw InvalidArgException("D", "second order kinetic Hill propensity needs one third species dependency, had",d3.len);

        // Find the rate costant.
        if (k.len != 4)  throw InvalidArgException("k", "second order kinetic Hill propensity needs four parameters, had",k.len);

        return new SecondOrderKHillPropensity(d1[0],d2[0],d3[0],k[0],k[1],k[2],k[3]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        const char* expressions[] = {"x1 * x2 * (k2 + (k3 - k2) * x3^k4 / (k1^k4 + x3^k4))", NULL};
        const char* unitsForConstants[] = {"item", "1/(item*second)", "1/(item*second)", "1", NULL};
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "SecondOrderKHillPropensity", expressions, unitsForConstants, &create);
    }

    static PropensityFunction* createAlternateFormat(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple d1 = getSpecificDependencies(reactionIndex, D, 1);
        if (d1.len != 1) throw InvalidArgException("D", "second order kinetic Hill propensity needs one first species dependency, had",d1.len);
        utuple d2 = getSpecificDependencies(reactionIndex, D, 2);
        if (d2.len != 1) throw InvalidArgException("D", "second order kinetic Hill propensity needs one second species dependency, had",d2.len);
        utuple d3 = getSpecificDependencies(reactionIndex, D, 3);
        if (d3.len != 1) throw InvalidArgException("D", "second order kinetic Hill propensity needs one third species dependency, had",d3.len);

        // Find the rate costant.
        if (k.len != 4)  throw InvalidArgException("k", "second order kinetic Hill propensity needs four parameters, had",k.len);

        return new SecondOrderKHillPropensity(d1[0],d2[0],d3[0],k[0],k[1],k[1]+k[2],k[3]);
    }

    static lm::me::PropensityFunctionDefinition registerFunctionAlternateFormat()
    {
        const char* expressions[] = {"x1 * x2 * (k2 + k3 * x3^k4 / (k1^k4 + x3^k4))", "x1 * x2 * (k2 + k3 * (x3^k4 / (k1^k4 + x3^k4)))", NULL};
        const char* unitsForConstants[] = {"item", "1/(item*second)", "1/(item*second)", "1", NULL};
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE_ALTERNATE_FORMAT, "SecondOrderKHillPropensity", expressions, unitsForConstants, &createAlternateFormat);
    }
};

class SecondOrderNegativeKHillPropensity : public lm::me::PropensityFunction
{
public:
    static const uint REACTION_TYPE = 8016;
    static const uint REACTION_TYPE_ALTERNATE_FORMAT = 8017;

    SecondOrderNegativeKHillPropensity(uint s0i, uint s1i, uint xi, double x0, double k0, double k1, double h) :PropensityFunction(REACTION_TYPE,1),s0i(s0i),s1i(s1i),xi(xi),x0h(pow(x0,h)),k0(k0),dk(k1-k0),h(h) {}
    int s0i, s1i;
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
        double k = k0+((dk*x0h)/(x0h+xh));
        return k * double(speciesCounts[s0i]) * double(speciesCounts[s1i]);
    }

#if defined(OPT_AVX) && !defined(OPT_SVML)
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        double x0 = speciesCounts[xi*DOUBLES_PER_AVX];
        double x1 = speciesCounts[xi*DOUBLES_PER_AVX+1];
        double x2 = speciesCounts[xi*DOUBLES_PER_AVX+2];
        double x3 = speciesCounts[xi*DOUBLES_PER_AVX+3];
        avxd xh = _mm256_set_pd(pow(x3,h),pow(x2,h),pow(x1,h),pow(x0,h));
        avxd k = _mm256_add_pd(_mm256_set1_pd(k0),_mm256_div_pd(_mm256_mul_pd(_mm256_set1_pd(dk),_mm256_set1_pd(x0h)),_mm256_add_pd(_mm256_set1_pd(x0h),xh)));
        return _mm256_mul_pd(k, _mm256_mul_pd(_mm256_load_pd(&speciesCounts[s0i*DOUBLES_PER_AVX]),_mm256_load_pd(&speciesCounts[s1i*DOUBLES_PER_AVX])));
    }
#endif
#if defined(OPT_AVX) && defined(OPT_SVML)
    avxd calculateAvx(const avxd time, const double* speciesCounts, const uint numberSpecies) const
    {
        avxd x = _mm256_load_pd(&speciesCounts[xi*DOUBLES_PER_AVX]);
        avxd xh = _mm256_pow_pd(x, _mm256_set1_pd(h));
        avxd k = _mm256_add_pd(_mm256_set1_pd(k0),_mm256_div_pd(_mm256_mul_pd(_mm256_set1_pd(dk),_mm256_set1_pd(x0h)),_mm256_add_pd(_mm256_set1_pd(x0h),xh)));
        return _mm256_mul_pd(k, _mm256_mul_pd(_mm256_load_pd(&speciesCounts[s0i*DOUBLES_PER_AVX]),_mm256_load_pd(&speciesCounts[s1i*DOUBLES_PER_AVX])));
    }
#endif

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple d1 = getSpecificDependencies(reactionIndex, D, 1);
        if (d1.len != 1) throw InvalidArgException("D", "second order kinetic Hill propensity needs one first species dependency, had",d1.len);
        utuple d2 = getSpecificDependencies(reactionIndex, D, 2);
        if (d2.len != 1) throw InvalidArgException("D", "second order kinetic Hill propensity needs one second species dependency, had",d2.len);
        utuple d3 = getSpecificDependencies(reactionIndex, D, 3);
        if (d3.len != 1) throw InvalidArgException("D", "second order kinetic Hill propensity needs one third species dependency, had",d3.len);

        // Find the rate costant.
        if (k.len != 4)  throw InvalidArgException("k", "second order kinetic Hill propensity needs four parameters, had",k.len);

        return new SecondOrderNegativeKHillPropensity(d1[0],d2[0],d3[0],k[0],k[1],k[2],k[3]);
    }

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        const char* expressions[] = {"x1 * x2 * (k2 + (k3 - k2) * k1^k4 / (k1^k4 + x3^k4))", NULL};
        const char* unitsForConstants[] = {"item", "1/(item*second)", "1/(item*second)", "1", NULL};
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "SecondOrderNegativeKHillPropensity", expressions, unitsForConstants, &create);
    }

    static PropensityFunction* createAlternateFormat(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple d1 = getSpecificDependencies(reactionIndex, D, 1);
        if (d1.len != 1) throw InvalidArgException("D", "second order kinetic Hill propensity needs one first species dependency, had",d1.len);
        utuple d2 = getSpecificDependencies(reactionIndex, D, 2);
        if (d2.len != 1) throw InvalidArgException("D", "second order kinetic Hill propensity needs one second species dependency, had",d2.len);
        utuple d3 = getSpecificDependencies(reactionIndex, D, 3);
        if (d3.len != 1) throw InvalidArgException("D", "second order kinetic Hill propensity needs one third species dependency, had",d3.len);

        // Find the rate costant.
        if (k.len != 4)  throw InvalidArgException("k", "second order kinetic Hill propensity needs four parameters, had",k.len);

        return new SecondOrderNegativeKHillPropensity(d1[0],d2[0],d3[0],k[0],k[1],k[1]+k[2],k[3]);
    }

    static lm::me::PropensityFunctionDefinition registerFunctionAlternateFormat()
    {
        const char* expressions[] = {"x1 * x2 * (k2 + k3 * k1^k4 / (k1^k4 + x3^k4))", "x1 * x2 * (k2 + k3 * (k1^k4 / (k1^k4 + x3^k4)))", NULL};
        const char* unitsForConstants[] = {"item", "1/(item*second)", "1/(item*second)", "1", NULL};
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE_ALTERNATE_FORMAT, "SecondOrderNegativeKHillPropensity", expressions, unitsForConstants, &createAlternateFormat);
    }
};

list<lm::me::PropensityFunctionDefinition> GeneticCircuitPropensityFunctions::getPropensityFunctionDefinitions()
{
    list<lm::me::PropensityFunctionDefinition> defs;
    defs.push_back(ZerothOrderImpulsePropensity::registerFunction());
    defs.push_back(TimeDependentHarmonicBirthPropensity::registerFunction());
    defs.push_back(TimeDependentHarmonicDeathPropensity::registerFunction());
    defs.push_back(TimeDependentQuadraticBirthPropensity::registerFunction());
    defs.push_back(TimeDependentQuadraticDeathPropensity::registerFunction());
    defs.push_back(ZerothOrderKHillPropensity::registerFunction());
    defs.push_back(ZerothOrderKHillPropensity::registerFunctionAlternateFormat());
    defs.push_back(ZerothOrderKHillTwoSpeciesOrPropensity::registerFunction());
    defs.push_back(FirstOrderKHillPropensity::registerFunction());
    defs.push_back(FirstOrderKHillPropensity::registerFunctionAlternateFormat());
    defs.push_back(FirstOrderNegativeKHillPropensity::registerFunction());
    defs.push_back(FirstOrderNegativeKHillPropensity::registerFunctionAlternateFormat());
    defs.push_back(SecondOrderKHillPropensity::registerFunction());
    defs.push_back(SecondOrderKHillPropensity::registerFunctionAlternateFormat());
    defs.push_back(SecondOrderNegativeKHillPropensity::registerFunction());
    defs.push_back(SecondOrderNegativeKHillPropensity::registerFunctionAlternateFormat());
    return defs;
}

}
}
