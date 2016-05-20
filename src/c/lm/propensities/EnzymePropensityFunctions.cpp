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


class FirstOrderMichaelisMentenU1 : public lm::me::PropensityFunction
{
public:
	static const uint REACTION_TYPE = 4003;

	FirstOrderMichaelisMentenU1(uint s1, uint s2, double k, double Rm, double Km) :PropensityFunction(REACTION_TYPE,1),s1(s1),s2(s2),k(k),Rm(Rm),Km(Km) {}
	uint s1;
	uint s2;
	double k;
	double Rm;
	double Km;

	void changeVolume(double volumeMultiplier) {}
	double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
	{
		double u1 = 1/(1+(k*double(speciesCounts[s1])));
		double MM = (Rm*double(speciesCounts[s2]))/(Km + double(speciesCounts[s2]));
		return u1*MM;
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
		if (sd1.len != 1) throw InvalidArgException("D", "FirstOrderMichaelisMentenU1 needs one species dependencies, had", sd1.len);
		utuple sd2 = getSpecificDependencies(reactionIndex, D, 2);
		if (sd2.len != 1) throw InvalidArgException("D", "FirstOrderMichaelisMentenU1 needs one species dependencies, had", sd2.len);

		// Find the rate constant. 
		if (k.len < 3) throw InvalidArgException("k", "FirstOrderMichaelisMentenU1 needs three parameters, had", k.len);

		return new FirstOrderMichaelisMentenU1(sd1[0], sd2[0], k[0], k[1], k[2]);
	}

	static lm::me::PropensityFunctionDefinition registerFunction()
	{
		const char* expressions[] = {"(1 / (1 + k1 * x1)) * ((k2 * x2) / (k3 +  x2))", "k2 * x2 * (1 / (1 + k1 * x1)) / (k3 + x2)", NULL};
		return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "FirstOrderMichaelisMentenU1", expressions, &create);
	}
};

class FirstOrderDoubleMichaelisMenten : public lm::me::PropensityFunction
{
public:
	static const uint REACTION_TYPE = 4004;

	FirstOrderDoubleMichaelisMenten(uint s1, uint s2, double k1, double k2, double k3) :PropensityFunction(REACTION_TYPE,1),s1(s1),s2(s2),k1(k1),k2(k2),k3(k3)	{}
	uint s1;
	uint s2;
	double k1;
	double k2;
	double k3;

	void changeVolume(double volumeMultiplier) {}
	double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
	{
		return k1*(double(speciesCounts[s1])/(k2 + double(speciesCounts[s1])))*(double(speciesCounts[s2])/(k3 + double(speciesCounts[s2])));
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
		if (sd1.len != 1) throw InvalidArgException("D", "FirstOrderDoubleMichaelisMenten needs one species dependencies, had", sd1.len);
		utuple sd2 = getSpecificDependencies(reactionIndex, D, 2);
		if (sd2.len != 1) throw InvalidArgException("D", "FirstOrderDoubleMichaelisMenten needs one species dependencies, had", sd2.len);

		// Find the rate constant.
		if (k.len < 3) throw InvalidArgException("k", "FirstOrderDoubleMichaelisMenten needs three parameters, had", k.len);

		return new FirstOrderDoubleMichaelisMenten(sd1[0], sd2[0], k[0], k[1], k[2]);
	}

	static lm::me::PropensityFunctionDefinition registerFunction()
	{
		const char* expressions[] = {"((k1 * x1) / (k2 + x1)) * (x2 / (k3 + x2))", "k1 * (x2 / (k3 + x2)) * (x1 / (k2 + x1))", "(k1 * (x2 / (k3 + x2))) * (x1) / (k2 + x1)", NULL};
		return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "FirstOrderDoubleMichaelisMenten", expressions, &create);
	}
};

class FirstOrderProductSubstrateDependent2Species : public lm::me::PropensityFunction
{
public:
	static const uint REACTION_TYPE = 4005;

	FirstOrderProductSubstrateDependent2Species(uint s1, uint s2, double Km, double Rm) :PropensityFunction(REACTION_TYPE,1),s1(s1),s2(s2),Km(Km),Rm(Rm) {}
	uint s1;
	uint s2;
	double Km;
	double Rm;

	void changeVolume(double volumeMultiplier) {}
	double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
	{
		double u4 = double(speciesCounts[s1])/(Km + double(speciesCounts[s1]));
		return u4*Rm*(double(speciesCounts[s2]) - double(speciesCounts[s1]));
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
		if (sd1.len != 1) throw InvalidArgException("D", "FirstOrderProductSubstrateDependent2Species needs one species dependencies, had", sd1.len);
		utuple sd2 = getSpecificDependencies(reactionIndex, D, 2);
		if (sd2.len != 1) throw InvalidArgException("D", "FirstOrderProductSubstrateDependent2Species needs one species dependencies, had", sd2.len);

		// Find the rate constant.
		if (k.len < 2) throw InvalidArgException("k", "FirstOrderProductSubstrateDependent2Species needs two parameters, had", k.len);

		return new FirstOrderProductSubstrateDependent2Species(sd1[0], sd2[0], k[0], k[1]);
	}

	static lm::me::PropensityFunctionDefinition registerFunction()
	{
		return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "FirstOrderProductSubstrateDependent2Species", "(x1 / (k1 + x1)) * k2 * (x2 - x1)", &create);
	}
};

class FirstOrderProductSubstrateDependent3Species : public lm::me::PropensityFunction
{
public:
	static const uint REACTION_TYPE = 4006;

	FirstOrderProductSubstrateDependent3Species(uint s1, uint s2, uint s3, double Km, double Rm) :PropensityFunction(REACTION_TYPE,1),s1(s1),s2(s2),s3(s3),Km(Km),Rm(Rm) {}
	uint s1;
	uint s2;
	uint s3;
	double Km;
	double Rm;

	void changeVolume(double volumeMultiplier) {}
	double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
	{
		double u5 = double(speciesCounts[s1])/(Km + double(speciesCounts[s1]));
		return u5*Rm*(double(speciesCounts[s2]) - double(speciesCounts[s3]));
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
		if (sd1.len != 1) throw InvalidArgException("D", "FirstOrderProductSubstrateDependent3Species needs one species dependencies, had", sd1.len);
		utuple sd2 = getSpecificDependencies(reactionIndex, D, 2);
		if (sd2.len != 1) throw InvalidArgException("D", "FirstOrderProductSubstrateDependent3Species needs one species dependencies, had", sd2.len);
		utuple sd3 = getSpecificDependencies(reactionIndex, D, 3);
		if (sd3.len != 1) throw InvalidArgException("D", "FirstOrderProductSubstrateDependent3Species needs one species dependencies, had", sd3.len);

		// Find the rate constant.
		if (k.len < 2) throw InvalidArgException("k", "FirstOrderProductSubstrateDependent3Species needs two parameters, had", k.len);

		return new FirstOrderProductSubstrateDependent3Species(sd1[0], sd2[0], sd3[0], k[0], k[1]);
	}

	static lm::me::PropensityFunctionDefinition registerFunction()
	{
		return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "FirstOrderProductSubstrateDependent3Species", "(x1 / (k1 + x1)) * k2 * (x2 - x3)", &create);
	}
};

class FirstOrderU6 : public lm::me::PropensityFunction
{
public:
	static const uint REACTION_TYPE = 4007;

	FirstOrderU6(uint s1, double a1, double a2, double sc, double k1, double k2) :PropensityFunction(REACTION_TYPE,1),s1(s1),a1(a1),a2(a2),sc(sc),k1(k1),k2(k2) {}
	uint s1;
	double a1;
	double a2;
	double sc;
	double k1;
	double k2;

	void changeVolume(double volumeMultiplier) {}
	double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
	{
		double u6 = 1/(1 + a1*exp(a2*(sc - double(speciesCounts[s1]))));
		return u6*k1*k2*double(speciesCounts[s1]);
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
		if (sd1.len != 1) throw InvalidArgException("D", "FirstOrderU6 needs one species dependencies, had", sd1.len);
		
		// Find the rate constant.
		if (k.len < 5) throw InvalidArgException("k", "FirstOrderU6 needs five parameters, had", k.len);

		return new FirstOrderU6(sd1[0], k[0], k[1], k[2], k[3], k[4]);
	}

	static lm::me::PropensityFunctionDefinition registerFunction()
	{
		const char* expressions[] = {"(1 / (1 + k1 * exp(k2 * (k3 - x1)))) * k4 * k5 * x1", "k4 * k5 * x1 * (1 / (1 + k1 * exp(k2 * (k3 - x1))))", NULL};
		return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "FirstOrderU6", expressions, &create);
	}
};

class FirstOrderU7 : public lm::me::PropensityFunction
{
public:
	static const uint REACTION_TYPE = 4008;

	FirstOrderU7(uint s1, double a1, double a2, double sc, double k1):PropensityFunction(REACTION_TYPE,1),s1(s1),a1(a1),a2(a2),sc(sc),k1(k1) {}
	uint s1;
	double a1;
	double a2;
	double sc;
	double k1;

	void changeVolume(double volumeMultiplier) {}
	double calculate(const double time, const int* speciesCounts, const uint numberSpecies) const
	{
		double u7 = 1/(1 + a1*exp(a2*(sc - double(speciesCounts[s1]))));
		return u7*k1*double(speciesCounts[s1]);
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
		if (sd1.len != 1) throw InvalidArgException("D", "FirstOrderU7 needs one species dependencies, had", sd1.len);
		
		// Find the rate constant.
		if (k.len < 4) throw InvalidArgException("k", "FirstOrderU7 needs four parameters, had", k.len);

		return new FirstOrderU7(sd1[0], k[0], k[1], k[2], k[3]);
	}

	static lm::me::PropensityFunctionDefinition registerFunction()
	{
		const char* expressions[] = {"(1 / (1 + k1 * exp(k2 * (k3 - x1)))) * k4 * x1", "k4 * x1 * (1 / (1 + k1 * exp(k2 * (k3 - x1))))", NULL};
		return lm::me::PropensityFunctionDefinition(REACTION_TYPE, "FirstOrderU7", expressions, &create);
	}
};

list<lm::me::PropensityFunctionDefinition> EnzymePropensityFunctions::getPropensityFunctionDefinitions()
{
    list<lm::me::PropensityFunctionDefinition> defs;
    defs.push_back(TwoSubstrateBindingPropensity::registerFunction());
    defs.push_back(ThreeSubstrateBindingPropensity::registerFunction());
    defs.push_back(FirstOrderMichaelisMenten::registerFunction());
    defs.push_back(FirstOrderMichaelisMentenU1::registerFunction());
    defs.push_back(FirstOrderDoubleMichaelisMenten::registerFunction());
    defs.push_back(FirstOrderProductSubstrateDependent2Species::registerFunction());
    defs.push_back(FirstOrderProductSubstrateDependent3Species::registerFunction());
    defs.push_back(FirstOrderU6::registerFunction());
    defs.push_back(FirstOrderU7::registerFunction());
    return defs;
}

}
}
