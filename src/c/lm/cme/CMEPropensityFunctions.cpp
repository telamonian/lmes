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

#include "lm/ClassFactory.h"
#include "lm/cme/CMEPropensityFunctions.h"
#include "lm/me/PropensityFunctions.h"

using std::list;
using std::map;
using std::string;
using std::vector;

namespace lm {
namespace cme {

bool CMEPropensityFunctions::registered=CMEPropensityFunctions::registerClass();

bool CMEPropensityFunctions::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::me::PropensityFunctionCollection", "lm::cme::CMEPropensityFunctions", &CMEPropensityFunctions::allocateObject);
    return true;
}

void* CMEPropensityFunctions::allocateObject()
{
    return new CMEPropensityFunctions();
}

CMEPropensityFunctions::CMEPropensityFunctions()
{
}

CMEPropensityFunctions::~CMEPropensityFunctions()
{
}

struct ZerothOrderPropensity : public lm::me::PropensityFunction
{
    static const uint REACTION_TYPE = 0;

    ZerothOrderPropensity(double k):k(k) {}
    double k;

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, &create, &calculate);
    }

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple dependencies = getDependencies(reactionIndex, D);
        if (dependencies.len != 0) throw InvalidArgException("D", "zeroth order propensity had invalid number of dependencies",dependencies.len);

        // Find the rate costant.
        if (k.len < 1)  throw InvalidArgException("k", "zeroth order propensity needs one rate constant",k.len);

        return new ZerothOrderPropensity(k[0]);
    }

    static double calculate(const double time, const int* speciesCounts, const PropensityFunction* pargs)
    {
        ZerothOrderPropensity * args = (ZerothOrderPropensity*)pargs;
        return args->k;
    }
};

struct FirstOrderPropensity : public lm::me::PropensityFunction
{
    static const uint REACTION_TYPE = 1;

    FirstOrderPropensity(uint si, double k) :si(si),k(k) {}
    uint si;
    double k;

    static lm::me::PropensityFunctionDefinition registerFunction()
    {
        return lm::me::PropensityFunctionDefinition(REACTION_TYPE, &create, &calculate);
    }

    static PropensityFunction* create(const uint reactionIndex, const ndarray<int> S, const ndarray<uint> D, const tuple<double>k)
    {
        // Find the species dependencies.
        utuple dependencies = getDependencies(reactionIndex, D);
        if (dependencies.len != 1) throw InvalidArgException("D", "first order propensity had invalid number of dependencies",dependencies.len);

        // Find the rate costant.
        if (k.len < 1)  throw InvalidArgException("k", "first order propensity needs one rate constant",k.len);

        return new FirstOrderPropensity(dependencies[0],k[0]);
    }

    static double calculate(const double time, const int* speciesCounts, const PropensityFunction* pargs)
    {
        FirstOrderPropensity * args = (FirstOrderPropensity*)pargs;
        return args->k * (double)speciesCounts[args->si];
    }
};


list<lm::me::PropensityFunctionDefinition> CMEPropensityFunctions::getPropensityFunctionDefinitions()
{
    list<lm::me::PropensityFunctionDefinition> defs;
    defs.push_back(ZerothOrderPropensity::registerFunction());
    defs.push_back(FirstOrderPropensity::registerFunction());
    return defs;
}



}
}


/**
    struct ZerothOrderTimeDependentPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 1000;
        ZerothOrderTimeDependentPropensityArgs(double ki, double kf, double tf) :ki(ki),kf(kf),tf(tf) {}
        double ki, kf, tf;
    };
    struct FirstOrderTimeDependentPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 1001;
        FirstOrderTimeDependentPropensityArgs(uint si, double ki, double kf, double tf) :si(si),ki(ki),kf(kf),tf(tf) {}
        uint si;
        double ki, kf, tf;
    };
    struct SecondOrderPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 2;
        SecondOrderPropensityArgs(uint s1i, uint s2i, double k) :s1i(s1i),s2i(s2i),k(k) {}
        uint s1i, s2i;
        double k;
    };
    struct SecondOrderSelfPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 3;
        SecondOrderSelfPropensityArgs(uint si, double k) :si(si),k(k) {}
        uint si;
        double k;
    };
    struct KHillPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 4;
        KHillPropensityArgs(uint si, double k0, double dk, double I50, double Iex, double h) :si(si),k(k0+(dk/(pow(I50/Iex,h)+1))) {}
        uint si;
        double k;
    };
    struct KHillTransportPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 5;
        KHillTransportPropensityArgs(uint si, uint xi, double k0, double dk, double I50, double Iex, double kit, double kid, double KM, double h, double V) :si(si),xi(xi),k0(k0),dk(dk),IRh(pow(I50/Iex,h)),ITp(kit/(kid*(Iex+KM)*NA*V)),h(h) {}
        uint si;
        uint xi;
        double k0;
        double dk;
        double IRh;
        double ITp;
        double h;
    };
    struct ZerothOrderHeavisidePropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 6;
        ZerothOrderHeavisidePropensityArgs(uint xi, uint x0, double k0, double k1) :xi(xi),x0(x0),k0(k0),k1(k1) {}
        uint xi;
        uint x0;
        double k0;
        double k1;
    };
    struct ZerothOrderNegativeFeedbackPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 8006;
        ZerothOrderNegativeFeedbackPropensityArgs(uint xi, double X, double beta, double h) :xi(xi),X(X),beta(beta),h(h) {}
        uint xi;
        double X;
        double beta;
        double h;
    };
    struct ZerothOrderKHillPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 8007;
        ZerothOrderKHillPropensityArgs(uint xi, uint x0, double k0, double k1, double h) :xi(xi),x0h(pow(x0,h)),k0(k0),dk(k1-k0),h(h) {}
        uint xi;
        double x0h;
        double k0;
        double dk;
        double h;
    };
    struct FirstOrderKHillPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 8013;
        FirstOrderKHillPropensityArgs(uint si, uint xi, uint x0, double k0, double k1, double h) :si(si),xi(xi),x0h(pow(x0,h)),k0(k0),dk(k1-k0),h(h) {}
        uint si;
        uint xi;
        double x0h;
        double k0;
        double dk;
        double h;
    };
    struct SecondOrderKHillPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 8014;
        SecondOrderKHillPropensityArgs(uint s1i, uint s2i, uint xi, uint x0, double k0, double k1, double h) :s1i(s1i),s2i(s2i),xi(xi),x0h(pow(x0,h)),k0(k0),dk(k1-k0),h(h) {}
        uint s1i;
        uint s2i;
        uint xi;
        double x0h;
        double k0;
        double dk;
        double h;
    };
    struct PDFitnessPropensityArgs : public PropensityArgs
    {
        static const uint COOPERATE_REACTION_TYPE = 8008;
        static const uint DEFECT_REACTION_TYPE = 8009;
        static const uint REFLECTING_COOPERATE_REACTION_TYPE = 8010;
        static const uint REFLECTING_DEFECT_REACTION_TYPE = 8011;
        PDFitnessPropensityArgs(uint ni, double N, double c, double b, double s, double lowBoundary=0.0, double highBoundary=0.0) :ni(ni),N(N),c(c),b(b),s(s),lowBoundary((uint)round(lowBoundary)),highBoundary((uint)round(highBoundary)) {}
        uint ni;
        double N;
        double c;
        double b;
        double s;
        uint lowBoundary;
        uint highBoundary;
    };
    struct MichaelisMentenPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 8012;
        MichaelisMentenPropensityArgs(uint si, double k0, double v0, double O) :si(si),k(k0*O),v(v0*O) {}
        uint si;
        double k;
        double v;
    };
    struct ZerothOrderNegativeFeedbackExtrinsicPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 8019;
        ZerothOrderNegativeFeedbackExtrinsicPropensityArgs(uint xi, double k, double beta, int mi, double invM) :xi(xi),k(k),beta(beta),mi(mi),invM(invM) {}
        uint xi;
        double k;
        double beta;
        uint mi;
        double invM;
    };
    struct EffectiveBurstPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 8100;
        EffectiveBurstPropensityArgs(uint ni, int N, double b) :ni(ni),N(N),b(b) {}
        uint ni;
        int N;
        double b;
    };

    static double zerothOrderTimeDependentPropensity(double time, uint * speciesCounts, void * pargs);
    static double firstOrderTimeDependentPropensity(double time, uint * speciesCounts, void * pargs);
    static double secondOrderPropensity(double time, uint * speciesCounts, void * pargs);
    static double secondOrderSelfPropensity(double time, uint * speciesCounts, void * pargs);
    static double kHillPropensity(double time, uint * speciesCounts, void * pargs);
    static double kHillTransportPropensity(double time, uint * speciesCounts, void * pargs);
    static double zerothOrderHeavisidePropensity(double time, uint * speciesCounts, void * pargs);
    static double zerothOrderNegativeFeedbackPropensity(double time, uint * speciesCounts, void * pargs);
    static double zerothOrderKHillPropensity(double time, uint * speciesCounts, void * pargs);
    static double firstOrderKHillPropensity(double time, uint * speciesCounts, void * pargs);
    static double secondOrderKHillPropensity(double time, uint * speciesCounts, void * pargs);
    static double pdCooperateFitnessPropensity(double time, uint * speciesCounts, void * pargs);
    static double pdDefectFitnessPropensity(double time, uint * speciesCounts, void * pargs);
    static double pdReflectingCooperateFitnessPropensity(double time, uint * speciesCounts, void * pargs);
    static double pdReflectingDefectFitnessPropensity(double time, uint * speciesCounts, void * pargs);
    static double MichaelisMentenPropensity(double time, uint * speciesCounts, void * pargs);
    static double effectiveBurstPropensity(double time, uint * speciesCounts, void * pargs);

double CMESolver::zerothOrderTimeDependentPropensity(double time, uint * speciesCounts, void * pargs)
{
    ZerothOrderTimeDependentPropensityArgs * args = (ZerothOrderTimeDependentPropensityArgs *)pargs;
    //printf("zeroth order time is: %.3f\n", time);
    //printf("zeroth order time dep prop: %.3f\n", args->ki + (args->kf - args->ki)*(time/args->tf));
    if (args->tf > time) {
        return args->ki + (args->kf - args->ki)*(time/args->tf);
    }
    else
    {
        return args->kf;
    }
}

double CMESolver::firstOrderTimeDependentPropensity(double time, uint * speciesCounts, void * pargs)
{
    FirstOrderTimeDependentPropensityArgs * args = (FirstOrderTimeDependentPropensityArgs *)pargs;
    //printf("first order time is: %.3f\n", time);
    //printf("first order time dep prop: %.3f\n", args->ki + (args->kf - args->ki)*(time/args->tf) * (double)speciesCounts[args->si]);
    if (args->tf > time) {
        return (args->ki + (args->kf - args->ki)*(time/args->tf)) * (double)speciesCounts[args->si];
    }
    else
    {
        return args->kf * (double)speciesCounts[args->si];
    }
}

double CMESolver::secondOrderPropensity(double time, uint * speciesCounts, void * pargs)
{
    SecondOrderPropensityArgs * args = (SecondOrderPropensityArgs *)pargs;
    return args->k * ((double)speciesCounts[args->s1i]) * ((double)speciesCounts[args->s2i]);
}

double CMESolver::secondOrderSelfPropensity(double time, uint * speciesCounts, void * pargs)
{
    SecondOrderSelfPropensityArgs * args = (SecondOrderSelfPropensityArgs *)pargs;
    uint count = speciesCounts[args->si];
    if (count >= 2)
        return args->k * ((double)count) * ((double)(count-1));
    return 0.0;
}

double CMESolver::kHillPropensity(double time, uint * speciesCounts, void * pargs)
{
    KHillPropensityArgs * args = (KHillPropensityArgs *)pargs;
    return args->k * (double)speciesCounts[args->si];
}

double CMESolver::kHillTransportPropensity(double time, uint * speciesCounts, void * pargs)
{
    KHillTransportPropensityArgs * args = (KHillTransportPropensityArgs *)pargs;

    uint s = speciesCounts[args->si];
    if (s == 0) return 0.0;

    double x = (double)speciesCounts[args->xi];
    double xh = pow(1+(args->ITp*x),args->h);
    double p = args->k0+((args->dk*xh)/(args->IRh+xh));

    //if (time == 0.0)
      //  Print::printf(Print::DEBUG, "Recalculating hill transport propensity for %d,%d with %e,%e,%e,%e,%e = %e", args->si, args->xi, args->k0, args->dk, args->IRh, args->ITp, args->h, p);

    return p;
}

double CMESolver::zerothOrderHeavisidePropensity(double time, uint * speciesCounts, void * pargs)
{
    ZerothOrderHeavisidePropensityArgs * args = (ZerothOrderHeavisidePropensityArgs *)pargs;
    return ((speciesCounts[args->xi]<args->x0)?(args->k0):(args->k1));
}

double CMESolver::zerothOrderNegativeFeedbackPropensity(double time, uint * speciesCounts, void * pargs)
{
    ZerothOrderNegativeFeedbackPropensityArgs * args = (ZerothOrderNegativeFeedbackPropensityArgs *)pargs;
    int x = (int)speciesCounts[args->xi];
    double X=args->X;
    double beta=args->beta;
    double p=X*((1.0+beta)/(1.0+beta*(x>=0?pow(double(x)/X,args->h):0.0)));
    //Print::printf(Print::DEBUG, "Recalculating zerothOrderNegativeFeedbackPropensity for %d (%d) with %e,%e,%e = %e", args->xi, x, args->X, args->beta, args->h, p);
    return p;
}

double CMESolver::zerothOrderKHillPropensity(double time, uint * speciesCounts, void * pargs)
{
    ZerothOrderKHillPropensityArgs * args = (ZerothOrderKHillPropensityArgs *)pargs;
    uint x = speciesCounts[args->xi];
    double xh = pow(x,args->h);

    return args->k0+((args->dk*xh)/(xh+args->x0h));
}

double CMESolver::firstOrderKHillPropensity(double time, uint * speciesCounts, void * pargs)
{
    FirstOrderKHillPropensityArgs * args = (FirstOrderKHillPropensityArgs *)pargs;
    uint x = speciesCounts[args->xi];
    double xh = pow(x,args->h);
    double k = args->k0+((args->dk*xh)/(xh+args->x0h));
    return ((double)speciesCounts[args->si]) * k;
}

double CMESolver::secondOrderKHillPropensity(double time, uint * speciesCounts, void * pargs)
{
    SecondOrderKHillPropensityArgs * args = (SecondOrderKHillPropensityArgs *)pargs;
    uint x = speciesCounts[args->xi];
    double xh = pow(x,args->h);
    double k = args->k0+((args->dk*xh)/(xh+args->x0h));
    return ((double)speciesCounts[args->s1i]) * ((double)speciesCounts[args->s2i]) * k;
}

double CMESolver::pdCooperateFitnessPropensity(double time, uint * speciesCounts, void * pargs)
{
    PDFitnessPropensityArgs * args = (PDFitnessPropensityArgs *)pargs;
    double n = (double)speciesCounts[args->ni];
    double N = args->N;
    double c = args->c;
    double b = args->b;
    double s = args->s;
    double prop=-(n*(n-N)*(N+b*n*s-c*N*s))/(N*(N+(b-c)*n*s));
    //Print::printf(Print::DEBUG, "Recalculating pd cooperate propensity for %d with %e,%e,%e,%e,%e = %e", args->ni, n,N,c,b,s,prop);
    return prop;
}

double CMESolver::pdDefectFitnessPropensity(double time, uint * speciesCounts, void * pargs)
{
    PDFitnessPropensityArgs * args = (PDFitnessPropensityArgs *)pargs;
    double n = (double)speciesCounts[args->ni];
    double N = args->N;
    double c = args->c;
    double b = args->b;
    double s = args->s;
    double prop=-(n*(n-N)*(N+b*n*s))/(N*(N+(b-c)*n*s));
    //Print::printf(Print::DEBUG, "Recalculating pd defect propensity for %d with %e,%e,%e,%e,%e = %e", args->ni, n,N,c,b,s,prop);
    return prop;
}

double CMESolver::pdReflectingCooperateFitnessPropensity(double time, uint * speciesCounts, void * pargs)
{
    PDFitnessPropensityArgs * args = (PDFitnessPropensityArgs *)pargs;
    double n = (double)speciesCounts[args->ni];
    double N = args->N;
    double c = args->c;
    double b = args->b;
    double s = args->s;
    double prop=(n<args->highBoundary)?(-(n*(n-N)*(N+b*n*s-c*N*s))/(N*(N+(b-c)*n*s))):(0.0);
    //if (n >= args->highBoundary-2)
    //	Print::printf(Print::DEBUG, "Recalculating reflecting pd cooperate propensity for %d (boundary=%d) with %e,%e,%e,%e,%e = %e", args->ni,args->highBoundary,n,N,c,b,s,prop);
    return prop;
}

double CMESolver::pdReflectingDefectFitnessPropensity(double time, uint * speciesCounts, void * pargs)
{
    PDFitnessPropensityArgs * args = (PDFitnessPropensityArgs *)pargs;
    double n = (double)speciesCounts[args->ni];
    double N = args->N;
    double c = args->c;
    double b = args->b;
    double s = args->s;
    double prop=(n>args->lowBoundary)?(-(n*(n-N)*(N+b*n*s))/(N*(N+(b-c)*n*s))):(0.0);
    //if (n <= args->lowBoundary+2)
    //	Print::printf(Print::DEBUG, "Recalculating reflecting pd defect propensity for %d (boundary=%d) with %e,%e,%e,%e,%e = %e", args->ni,args->lowBoundary,n,N,c,b,s,prop);
    return prop;
}

double CMESolver::MichaelisMentenPropensity(double time, uint * speciesCounts, void * pargs)
{
    MichaelisMentenPropensityArgs * args = (MichaelisMentenPropensityArgs *)pargs;
    return args->v*((double)speciesCounts[args->si]/(args->k + (double)speciesCounts[args->si]));
}

double CMESolver::effectiveBurstPropensity(double time, uint * speciesCounts, void * pargs)
{
    EffectiveBurstPropensityArgs * args = (EffectiveBurstPropensityArgs *)pargs;
    int n = (int)speciesCounts[args->ni];
    int N = args->N;
    double x=double(n)/double(N);
    double b = args->b;
    return N*((1+(b*x))/(1+b));
}



if (reactionTypes[i] == ZerothOrderTimeDependentPropensityArgs::REACTION_TYPE)
        {
            // Find the dependencies.
            uint numberDependencies = 0;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1)
                {
                    numberDependencies++;
                }
            }
            if (numberDependencies > 1000000)
            {
                throw InvalidArgException("D", "zeroth order time dependent reaction probably shouldn't have that many dependencies",numberDependencies);
            }
            else
            {
                propensityFunctions[i] = (void *)&zerothOrderTimeDependentPropensity;
                propensityFunctionArgs[i] =  (void *)new ZerothOrderTimeDependentPropensityArgs(K[i*kCols], K[i*kCols+1], K[i*kCols+2]);
                propensityArgs.push_back((PropensityArgs *)propensityFunctionArgs[i]);
            }
        }
        else if (reactionTypes[i] == FirstOrderTimeDependentPropensityArgs::REACTION_TYPE)
        {
            // Find the dependencies.
            uint numberDependencies = 0;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1)
                {
                    numberDependencies++;

                    // Set the table entry to the first non-zero dependency.
                    if (numberDependencies < 1000000)
                    {
                        propensityFunctions[i] = (void *)&firstOrderTimeDependentPropensity;
                        propensityFunctionArgs[i] =  (void *)new FirstOrderTimeDependentPropensityArgs(j, K[i*kCols], K[i*kCols+1], K[i*kCols+2]);
                        propensityArgs.push_back((PropensityArgs *)propensityFunctionArgs[i]);
                    }
                    else
                    {
                        throw InvalidArgException("D", "first order time dependent reaction probably shouldn't have that many dependencies",numberDependencies);
                    }
                }
            }
        }
        else if (reactionTypes[i] == SecondOrderPropensityArgs::REACTION_TYPE)
        {
            // Find the dependencies.
            uint numberDependencies = 0;
            uint firstDependency;
            uint secondDependency;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1)
                {
                    numberDependencies++;

                    // Set the table entry to the first two non-zero dependencies.
                    if (numberDependencies == 1)
                        firstDependency = j;
                    else if (numberDependencies == 2)
                        secondDependency = j;
                }
            }
            if (numberDependencies == 2)
            {
                propensityFunctions[i] = (void *)&secondOrderPropensity;
                propensityFunctionArgs[i] =  (void *)new SecondOrderPropensityArgs(firstDependency, secondDependency, K[i*kCols]);
                propensityArgs.push_back((PropensityArgs *)propensityFunctionArgs[i]);
            }
            else
            {
                printf("%d\n",numberDependencies);
                throw InvalidArgException("D", "second order reaction had invalid number of dependencies",numberDependencies);
            }
        }
        else if (reactionTypes[i] == SecondOrderSelfPropensityArgs::REACTION_TYPE)
        {
            // Find the dependencies.
            uint numberDependencies = 0;
            uint firstDependency;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1)
                {
                    numberDependencies++;

                    // Set the table entry to the first non-zero dependency.
                    if (numberDependencies == 1)
                        firstDependency = j;
                }
            }
            if (numberDependencies == 1)
            {
                propensityFunctions[i] = (void *)&secondOrderSelfPropensity;
                propensityFunctionArgs[i] =  (void *)new SecondOrderSelfPropensityArgs(firstDependency, K[i*kCols]);
                propensityArgs.push_back((PropensityArgs *)propensityFunctionArgs[i]);
            }
            else
            {
                throw InvalidArgException("D", "second order self reaction had invalid number of dependencies",numberDependencies);
            }
        }
        else if (reactionTypes[i] == KHillPropensityArgs::REACTION_TYPE)
        {
            // Find the dependencies.
            uint numberDependencies = 0;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1)
                {
                    numberDependencies++;

                    // Set the table entry to the first non-zero dependency.
                    if (numberDependencies == 1)
                    {
                        propensityFunctions[i] = (void *)&kHillPropensity;
                        propensityFunctionArgs[i] =  (void *)new KHillPropensityArgs(j, K[i*kCols], K[i*kCols+1], K[i*kCols+2], K[i*kCols+3], K[i*kCols+4]);
                        propensityArgs.push_back((PropensityArgs *)propensityFunctionArgs[i]);
                    }
                    else
                    {
                        throw InvalidArgException("D", "khill reaction had invalid number of dependencies",numberDependencies);
                    }
                }
            }
        }
        else if (reactionTypes[i] == KHillTransportPropensityArgs::REACTION_TYPE)
        {
            // Find the dependencies.
            uint firstDependency, secondDependency;
            uint numberDependencies = 0;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1)
                {
                    numberDependencies++;

                    // Find the two dependencies.
                    if (numberDependencies == 1)
                        firstDependency = j;
                    else if (numberDependencies == 2)
                        secondDependency = j;
                    else
                        throw InvalidArgException("D", "kinetic hill transport reaction had invalid number of dependencies",numberDependencies);
                }
            }

            // Figure out which dependency is the controlled species and which is the controlling.
            uint si, xi;
            if (S[firstDependency*numberReactions+i] == -1 && S[secondDependency*numberReactions+i] == 0)
            {
                si=firstDependency;
                xi=secondDependency;
            }
            else if (S[firstDependency*numberReactions+i] == 0 && S[secondDependency*numberReactions+i] == -1)
            {
                si=secondDependency;
                xi=firstDependency;
            }
            else
            {
                throw InvalidArgException("D", "kinetic hill transport reaction cannot be parsed",numberDependencies);
            }

            // Set up the propensity function.
            if (kCols < 9) throw InvalidArgException("kCols", "kinetic hill transport reaction requires nine K values",kCols);
            propensityFunctions[i] = (void *)&kHillTransportPropensity;
            propensityFunctionArgs[i] =  (void *)new KHillTransportPropensityArgs(si, xi, K[i*kCols], K[i*kCols+1], K[i*kCols+2], K[i*kCols+3], K[i*kCols+4], K[i*kCols+5], K[i*kCols+6], K[i*kCols+7], K[i*kCols+8]);
            propensityArgs.push_back((PropensityArgs *)propensityFunctionArgs[i]);
        }
        else if (reactionTypes[i] == ZerothOrderHeavisidePropensityArgs::REACTION_TYPE)
        {
            // Find the dependency.
            int xi=-1;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1)
                {
                    if (xi != -1) throw InvalidArgException("D", "zeroth order Heaviside reaction can only have one dependency");
                    xi = j;
                }
            }

            // Make sure we found the right dependencies.
            if (xi == -1) throw InvalidArgException("D", "zeroth order Heaviside reaction must have one dependency");

            // Set the table entry.
            propensityFunctions[i] = (void *)&zerothOrderHeavisidePropensity;
            propensityFunctionArgs[i] =  (void *)new ZerothOrderHeavisidePropensityArgs(xi, (uint)round(K[i*kCols]), K[i*kCols+1], K[i*kCols+2]);
            propensityArgs.push_back((PropensityArgs *)propensityFunctionArgs[i]);
        }
        else if (reactionTypes[i] == ZerothOrderNegativeFeedbackPropensityArgs::REACTION_TYPE)
        {
            // Find the dependency.
            int xi=-1;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1)
                {
                    if (xi != -1) throw InvalidArgException("D", "zeroth order negative feedback reaction can only have one dependency");
                    xi = j;
                }
            }

            // Make sure we found the right dependencies.
            if (xi == -1) throw InvalidArgException("D", "zeroth order negative feedback reaction must have one dependency");

            // Set the table entry.
            propensityFunctions[i] = (void *)&zerothOrderNegativeFeedbackPropensity;
            propensityFunctionArgs[i] =  (void *)new ZerothOrderNegativeFeedbackPropensityArgs(xi, K[i*kCols], K[i*kCols+1], K[i*kCols+2]);
            propensityArgs.push_back((PropensityArgs *)propensityFunctionArgs[i]);
        }
        else if (reactionTypes[i] == ZerothOrderKHillPropensityArgs::REACTION_TYPE)
        {
            // Find the dependency.
            int xi=-1;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1 || D[j*numberReactions+i] == 2 || D[j*numberReactions+i] == 3)
                {
                    if (xi != -1) throw InvalidArgException("D", "zeroth order KHill reaction can only have one dependency");
                    xi = j;
                }
            }

            // Make sure we found the right dependencies.
            if (xi == -1) throw InvalidArgException("D", "zeroth order KHill reaction must have one dependency");

            // Set the table entry.
            propensityFunctions[i] = (void *)&zerothOrderKHillPropensity;
            propensityFunctionArgs[i] =  (void *)new ZerothOrderKHillPropensityArgs(xi, (uint)round(K[i*kCols]), K[i*kCols+1], K[i*kCols+2], K[i*kCols+3]);
            propensityArgs.push_back((PropensityArgs *)propensityFunctionArgs[i]);
        }
        else if (reactionTypes[i] == FirstOrderKHillPropensityArgs::REACTION_TYPE)
        {
            // Find the dependency.
            int si=-1;
            int xi=-1;
            int count=0;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1 || D[j*numberReactions+i] == 3)
                {
                    if (si == -1)
                        si = j;
                    else
                        throw InvalidArgException("D", "first order KHill reaction can only have one species dependency");
                }
                if (D[j*numberReactions+i] == 2 || D[j*numberReactions+i] == 3)
                {
                    if (xi == -1)
                        xi = j;
                    else
                        throw InvalidArgException("D", "first order KHill reaction can only have one Hill dependency");
                }
            }

            // Make sure we found the right dependencies.
            if (si == -1) throw InvalidArgException("D", "first order KHill reaction must have one species dependency");
            if (xi == -1) throw InvalidArgException("D", "first order KHill reaction must have one Hill dependency");

            // Set the table entry.
            propensityFunctions[i] = (void *)&firstOrderKHillPropensity;
            propensityFunctionArgs[i] =  (void *)new FirstOrderKHillPropensityArgs(si, xi, (uint)round(K[i*kCols]), K[i*kCols+1], K[i*kCols+2], K[i*kCols+3]);
            propensityArgs.push_back((PropensityArgs *)propensityFunctionArgs[i]);
        }
        else if (reactionTypes[i] == SecondOrderKHillPropensityArgs::REACTION_TYPE)
        {
            // Find the dependency.
            int s1i=-1;
            int s2i=-1;
            int xi=-1;
            int count=0;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1 || D[j*numberReactions+i] == 3)
                {
                    if (s1i == -1)
                        s1i = j;
                    else if (s2i == -1)
                        s2i = j;
                    else
                        throw InvalidArgException("D", "second order KHill reaction can only have two species dependencies");
                }
                if (D[j*numberReactions+i] == 2 || D[j*numberReactions+i] == 3)
                {
                    if (xi == -1)
                        xi = j;
                    else
                        throw InvalidArgException("D", "second order KHill reaction can only have one Hill dependency");
                }
            }

            // Make sure we found the right dependencies.
            if (s1i == -1 || s2i == -1) throw InvalidArgException("D", "second order KHill reaction must have two species dependencies");
            if (xi == -1) throw InvalidArgException("D", "second order KHill reaction must have one Hill dependency");

            // Set the table entry.
            propensityFunctions[i] = (void *)&secondOrderKHillPropensity;
            propensityFunctionArgs[i] =  (void *)new SecondOrderKHillPropensityArgs(s1i, s2i, xi, (uint)round(K[i*kCols]), K[i*kCols+1], K[i*kCols+2], K[i*kCols+3]);
            propensityArgs.push_back((PropensityArgs *)propensityFunctionArgs[i]);
        }
        else if (reactionTypes[i] == PDFitnessPropensityArgs::COOPERATE_REACTION_TYPE)
        {
            // Find the dependency.
            int xi=-1;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1)
                {
                    if (xi != -1) throw InvalidArgException("D", "PD cooperate fitness reaction can only have one dependency");
                    xi = j;
                }
            }

            // Make sure we found the right dependencies.
            if (xi == -1) throw InvalidArgException("D", "PD cooperate fitness reaction must have one dependency");

            // Set the table entry.
            propensityFunctions[i] = (void *)&pdCooperateFitnessPropensity;
            if (globalPDFitnessPropensityArgs == NULL)
            {
                globalPDFitnessPropensityArgs = new PDFitnessPropensityArgs(xi, (uint)round(K[i*kCols]), K[i*kCols+1], K[i*kCols+2], K[i*kCols+3]);
                propensityArgs.push_back(globalPDFitnessPropensityArgs);
            }
            propensityFunctionArgs[i] = (void *)globalPDFitnessPropensityArgs;

        }
        else if (reactionTypes[i] == PDFitnessPropensityArgs::DEFECT_REACTION_TYPE)
        {
            // Find the dependency.
            int xi=-1;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1)
                {
                    if (xi != -1) throw InvalidArgException("D", "PD defect fitness reaction can only have one dependency");
                    xi = j;
                }
            }

            // Make sure we found the right dependencies.
            if (xi == -1) throw InvalidArgException("D", "PD defect fitness reaction must have one dependency");

            // Set the table entry.
            propensityFunctions[i] = (void *)&pdDefectFitnessPropensity;
            if (globalPDFitnessPropensityArgs == NULL)
            {
                globalPDFitnessPropensityArgs = new PDFitnessPropensityArgs(xi, (uint)round(K[i*kCols]), K[i*kCols+1], K[i*kCols+2], K[i*kCols+3]);
                propensityArgs.push_back(globalPDFitnessPropensityArgs);
            }
            propensityFunctionArgs[i] = (void *)globalPDFitnessPropensityArgs;
        }
        else if (reactionTypes[i] == PDFitnessPropensityArgs::REFLECTING_COOPERATE_REACTION_TYPE)
        {
            // Find the dependency.
            int xi=-1;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1)
                {
                    if (xi != -1) throw InvalidArgException("D", "PD reflecting cooperate fitness reaction can only have one dependency");
                    xi = j;
                }
            }

            // Make sure we found the right dependencies.
            if (xi == -1) throw InvalidArgException("D", "PD reflecting cooperate fitness reaction must have one dependency");

            // Set the table entry.
            propensityFunctions[i] = (void *)&pdReflectingCooperateFitnessPropensity;
            if (globalPDFitnessPropensityArgs == NULL)
            {
                globalPDFitnessPropensityArgs = new PDFitnessPropensityArgs(xi, (uint)round(K[i*kCols]), K[i*kCols+1], K[i*kCols+2], K[i*kCols+3], K[i*kCols+4], K[i*kCols+5]);
                propensityArgs.push_back(globalPDFitnessPropensityArgs);
            }
            globalPDFitnessPropensityArgs->lowBoundary = (uint)round(K[i*kCols+4]);
            globalPDFitnessPropensityArgs->highBoundary = (uint)round(K[i*kCols+5]);
            propensityFunctionArgs[i] = (void *)globalPDFitnessPropensityArgs;

        }
        else if (reactionTypes[i] == PDFitnessPropensityArgs::REFLECTING_DEFECT_REACTION_TYPE)
        {
            // Find the dependency.
            int xi=-1;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1)
                {
                    if (xi != -1) throw InvalidArgException("D", "PD reflecting defect fitness reaction can only have one dependency");
                    xi = j;
                }
            }

            // Make sure we found the right dependencies.
            if (xi == -1) throw InvalidArgException("D", "PD reflecting defect fitness reaction must have one dependency");

            // Set the table entry.
            propensityFunctions[i] = (void *)&pdReflectingDefectFitnessPropensity;
            if (globalPDFitnessPropensityArgs == NULL)
            {
                globalPDFitnessPropensityArgs = new PDFitnessPropensityArgs(xi, (uint)round(K[i*kCols]), K[i*kCols+1], K[i*kCols+2], K[i*kCols+3], K[i*kCols+4], K[i*kCols+5]);
                propensityArgs.push_back(globalPDFitnessPropensityArgs);
            }
            globalPDFitnessPropensityArgs->lowBoundary = (uint)round(K[i*kCols+4]);
            globalPDFitnessPropensityArgs->highBoundary = (uint)round(K[i*kCols+5]);
            propensityFunctionArgs[i] = (void *)globalPDFitnessPropensityArgs;
        }
        else if (reactionTypes[i] == EffectiveBurstPropensityArgs::REACTION_TYPE)
        {
            // Find the dependencies.
            uint numberDependencies = 0;
            for (uint j=0; j<numberSpecies; j++)
            {
                if (D[j*numberReactions+i] == 1)
                {
                    numberDependencies++;

                    // Set the table entry to the first non-zero dependency.
                    if (numberDependencies == 1)
                    {
                        propensityFunctions[i] = (void *)&effectiveBurstPropensity;
                        propensityFunctionArgs[i] =  (void *)new EffectiveBurstPropensityArgs(j, int(round(K[i*kCols])), K[i*kCols+1]);
                        propensityArgs.push_back((PropensityArgs *)propensityFunctionArgs[i]);
                    }
                    else
                    {
                        throw InvalidArgException("D", "first order reaction had invalid number of dependencies",numberDependencies);
                    }
                }
            }
        }

        */
