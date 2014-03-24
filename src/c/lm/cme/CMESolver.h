/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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

#ifndef LM_CME_CMESOLVER_H_
#define LM_CME_CMESOLVER_H_

#include <map>
#include <list>
#include <string>
#include <utility>
#include "lm/Math.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/ParameterValues.pb.h"
#include "lm/resource/ResourceAllocator.h"
#include "lm/rng/RandomGenerator.h"
#include "lm/me/MESolver.h"

using std::map;
using std::pair;
using std::list;
using std::string;
using lm::resource::ResourceAllocator;
using lm::me::MESolver;
using lm::rng::RandomGenerator;

namespace lm {

namespace io {
class ReactionModel;
}

namespace cme {

class CMESolver : public MESolver
{
protected:
    struct PropensityArgs
    {
        virtual ~PropensityArgs() {}
    };
    struct ZerothOrderPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 0;
        ZerothOrderPropensityArgs(double k) :k(k) {}
        double k;
    };
    struct FirstOrderPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 1;
        FirstOrderPropensityArgs(uint si, double k) :si(si),k(k) {}
        uint si;
        double k;
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
        static const uint REACTION_TYPE = 4;
        MichaelisMentenPropensityArgs(uint si, double k0, double v0, double O) :si(si),k(k0*O),v(v0*O) {}
        uint si;
        double k;
        double v;
    };
    struct SpeciesLimit
    {
        int type;
        uint species;
        uint limit;
    };
    struct FPTTracking
    {
        uint species;
        uint minValueAchieved;
        uint maxValueAchieved;
        lm::io::FirstPassageTimes dataSet;
    };
    struct TrackedParameter
    {
        TrackedParameter(string name, double * valuePointer):name(name),valuePointer(valuePointer) {dataSet.set_parameter(name);}
        string name;
        double * valuePointer;
        lm::io::ParameterValues dataSet;
    };

public:
    CMESolver(RandomGenerator::Distributions neededDists);
    virtual ~CMESolver();
    virtual void initialize(unsigned int replicate, map<string,string> * parameters, ResourceAllocator::ComputeResources * resources);
    virtual void setReactionModel(lm::io::ReactionModel * reactionModel);
    virtual void buildModel(const uint numberSpecies, const uint numberReactions, const uint * initialSpeciesCounts, const uint * reactionTypesA, const double * k, const int * S, const uint * D, const uint kCols=1);
    virtual void setModelPropensityFunction(uint reaction, double (*propensityFunction)(double time, uint * speciesCounts, void * args), void * propensityFunctionArg);
    virtual void setSpeciesUpperLimit(uint species, uint limit);
    virtual void setSpeciesLowerLimit(uint species, uint limit);
    virtual void setFptTrackingList(list<uint> speciesList);
    virtual void addToParameterTrackingList(pair<string,double*>parameter);
    virtual void generateTrajectory()=0;

protected:
    static double zerothOrderPropensity(double time, uint * speciesCounts, void * pargs);
    static double firstOrderPropensity(double time, uint * speciesCounts, void * pargs);
    static double secondOrderPropensity(double time, uint * speciesCounts, void * pargs);
    static double secondOrderSelfPropensity(double time, uint * speciesCounts, void * pargs);
    static double kHillPropensity(double time, uint * speciesCounts, void * pargs);
    static double kHillTransportPropensity(double time, uint * speciesCounts, void * pargs);
    static double zerothOrderHeavisidePropensity(double time, uint * speciesCounts, void * pargs);
    static double zerothOrderKHillPropensity(double time, uint * speciesCounts, void * pargs);
    static double pdCooperateFitnessPropensity(double time, uint * speciesCounts, void * pargs);
    static double pdDefectFitnessPropensity(double time, uint * speciesCounts, void * pargs);
    static double pdReflectingCooperateFitnessPropensity(double time, uint * speciesCounts, void * pargs);
    static double pdReflectingDefectFitnessPropensity(double time, uint * speciesCounts, void * pargs);
    static double MichaelisMentenPropensity(double time, uint * speciesCounts, void * pargs);

    virtual void allocateModel(uint numberSpecies, uint numberReactions);
    virtual void destroyModel();
    virtual double recordParameters(double nextRecordTime, double recordInterval, double simulationTime);
    virtual void queueRecordedParameters(bool flush=false);

    inline void updateSpeciesCounts(uint r)
    {
        for (uint i=0; i<numberDependentSpecies[r]; i++)
        {
            speciesCounts[dependentSpecies[r][i]] += dependentSpeciesChange[r][i];
        }
    }

    inline bool reachedSpeciesLimit()
    {
        for (uint i=0; i<numberSpeciesLimits; i++)
        {
            SpeciesLimit l = speciesLimits[i];
            switch (l.type)
            {
            case -1:
                if (speciesCounts[l.species] <= l.limit) return true;
                break;
            case 1:
                if (speciesCounts[l.species] >= l.limit) return true;
                break;
            }
        }
        return false;
    }



protected:
    RandomGenerator::Distributions neededDists;
    unsigned int replicate;
    map<string,string> * parameters;
    ResourceAllocator::ComputeResources * resources;
    RandomGenerator * rng;

    // The reaction model.
    uint numberSpecies;
    uint numberSpeciesToTrack;
    uint numberReactions;
    uint * initialSpeciesCounts;                    // numberSpecies
    uint * speciesCounts;                           // numberSpecies
    uint * reactionTypes;							// numberReactions
    int * S;                                        // numberSpecies x numberReactions
    uint * D;                                       // numberSpecies x numberReactions
    void ** propensityFunctions;
    void ** propensityFunctionArgs;
    list<PropensityArgs *> propensityArgs;
    uint numberSpeciesLimits;
    SpeciesLimit * speciesLimits;
    uint numberFptTrackedSpecies;
    FPTTracking * fptTrackedSpecies;
    list<TrackedParameter> trackedParameters;

    // Dependency tables.
    uint *numberDependentSpecies;
    uint ** dependentSpecies;
    int ** dependentSpeciesChange;
    uint *numberDependentReactions;
    uint ** dependentReactions;
};

}
}

#endif
