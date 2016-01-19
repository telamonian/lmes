/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012-2015 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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
#ifndef LM_CME_CMESOLVER_H_
#define LM_CME_CMESOLVER_H_

#include <algorithm>
#include <cstdio>
#include <deque>
#include <list>
#include <map>
#include <pthread.h>
#include <string>
#include <utility>
#include <vector>
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/ParameterValues.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/main/Main.h"
#include "lm/Math.h"
#include "lm/me/MESolver.h"
#include "lm/oparam/OParams.h"
#include "lm/rng/RandomGenerator.h"
#include "lm/thread/Thread.h"
#include "lm/tiling/Tilings.h"
#include "lm/Types.h"

using std::list;
using std::map;
using std::pair;
using std::string;
using std::vector;
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
    struct ZerothOrderTimeDependentPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 1000;
        ZerothOrderTimeDependentPropensityArgs(double ki, double kf, double tf) :ki(ki),kf(kf),tf(tf) {}
        double ki, kf, tf;
    };
    struct FirstOrderPropensityArgs : public PropensityArgs
    {
        static const uint REACTION_TYPE = 1;
        FirstOrderPropensityArgs(uint si, double k) :si(si),k(k) {}
        uint si;
        double k;
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

    class SpeciesLimit
    {
    public:
        enum limit_type_t {MIN, MAX, DECREASING_ASCENDING, INCREASING_ASCENDING, DECREASING_DESCENDING, INCREASING_DESCENDING};
        limit_type_t type;
        int species;
        double limit;
    };
    class FPTTracking
    {
    public:
        int species;
        int minValueAchieved;
        int maxValueAchieved;
        std::deque<std::pair<int,double> > fptValues;
        void serializeTo(uint64_t trajectoryId, lm::io::FirstPassageTimes* fpt)
        {
            fpt->set_trajectory_id(trajectoryId);
            fpt->set_species(species);
            fpt->set_number_entries(fptValues.size());
            for (std::deque<std::pair<int,double> >::iterator it=fptValues.begin(); it != fptValues.end(); it++)
            {
                fpt->add_species_count(it->first);
                fpt->add_first_passage_time(it->second);
            }
        }
    };
    struct TrackedParameter
    {
        TrackedParameter(string name, double * valuePointer):name(name),valuePointer(valuePointer) {dataSet.set_parameter(name);}
        string name;
        double * valuePointer;
        lm::io::ParameterValues dataSet;
    };
    class TilingHist
    {
    public:
        // consructor reads in a TilingHistBuf object
        TilingHist(): numberTileVals(0), tilingID(), tileVals(NULL)
        {
        }
        ~TilingHist()
        {
            delete tileVals;
        }
        // consructor reads in a TilingHistBuf object
        void init(const lm::io::TilingHist& tHistBuf)
        {
            numberTileVals = tHistBuf.tile_vals_size();
            tilingID = tHistBuf.tiling_id();
            tileVals = new double[numberTileVals];
            for (uint i=0;i<numberTileVals;i++)
            {
                tileVals[i] = tHistBuf.tile_vals(i);
            }
        }
        // this function writes out to a TilingHistBuf object
        void serializeTo(lm::io::TilingHist* tHistBuf)
        {
            tHistBuf->set_tiling_id(tilingID);
            tHistBuf->clear_tile_vals();
            for (uint i=0;i<numberTileVals;i++)
            {
                tHistBuf->add_tile_vals(tileVals[i]);
            }
        }
        uint numberTileVals;
        uint tilingID;
        double* tileVals;
    };

public:
    CMESolver(RandomGenerator::Distributions neededDists);
    virtual ~CMESolver();
    virtual void setComputeResources(vector<int> cpus, vector<int> gpus);
    virtual bool needsReactionModel() {return true;}
    virtual void setReactionModel(const lm::io::ReactionModel& rm);
    virtual bool needsDiffusionModel() {return false;}
    virtual void setDiffusionModel(const lm::io::DiffusionModel& dm) {}
    virtual bool needsOrderParameters() {return ffluxFlag;}
    virtual void setOrderParameters(const lm::io::OrderParameters& opsBuf);
    virtual bool needsTilings() {return ffluxFlag;}
    virtual void setTilings(const lm::io::Tilings& tilingsBuf);
    virtual void reset();
    virtual void getState(lm::io::TrajectoryState* state);
    virtual void setState(const lm::io::TrajectoryState& state);
    virtual void setLimits(const lm::io::TrajectoryLimits& limits);
    virtual lm::io::TrajectoryLimits::LimitType getFinalLimitType();

protected:
    virtual void setSpeciesUpperLimit(int species, int limit);
    virtual void setSpeciesLowerLimit(int species, int limit);
    virtual void setSpeciesDecreasingLimit(lm::io::TrajectoryLimits::Arrangement, int opID, double limit);
    virtual void setSpeciesIncreasingLimit(lm::io::TrajectoryLimits::Arrangement, int opID, double limit);
    virtual void addToParameterTrackingList(pair<string,double*>parameter);

    static double zerothOrderPropensity(double time, uint * speciesCounts, void * pargs);
    static double zerothOrderTimeDependentPropensity(double time, uint * speciesCounts, void * pargs);
    static double firstOrderPropensity(double time, uint * speciesCounts, void * pargs);
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

    //virtual double recordParameters(double nextRecordTime, double recordInterval, double simulationTime);
    //virtual void queueRecordedParameters(bool flush=false);

    inline void performReactionEvent(uint r)
    {
    	// Update the counts according to the dependency tables.
        for (int i=0; i<(int)reactionModel->numberDependentSpecies[r]; i++)
        {
            speciesCounts[reactionModel->dependentSpecies[r][i]] += reactionModel->dependentSpeciesChange[r][i];
            updatedSpeciesCounts();
        }
        if (ffluxFlag==true)
        {
            // Update the order parameters, if required
            for (int i=0; i<oparams->size(); i++)
            {
                (*oparams)[i]->calc(speciesCounts);
            }
            // Update the tilingHists, if required
//            for (int i=0;i<numberTilingHists;i++)
//            {
//                tilingHists[i].tileVals[(*tilings)[tilingHists[i].tilingID]->getTileIndex((*oparams)[(*tilings)[tilingHists[i].tilingID]->getOrderParameterID()]->get())] += timeStep;
//            }
        }
    }

    inline void updatedSpeciesCounts()
    {
        // Update the first passage time tables.
        for (int i=0; i<numberFptTrackedSpecies; i++)
        {
            int speciesCount = speciesCounts[fptTrackedSpecies[i].species];
            while (speciesCount < fptTrackedSpecies[i].minValueAchieved)
            {
                fptTrackedSpecies[i].fptValues.push_front(std::pair<int,double>(--fptTrackedSpecies[i].minValueAchieved,time));
            }
            while (speciesCount > fptTrackedSpecies[i].maxValueAchieved)
            {
                fptTrackedSpecies[i].fptValues.push_back(std::pair<int,double>(++fptTrackedSpecies[i].maxValueAchieved,time));
            }
        }
    }

    inline bool reachedSpeciesLimit()
    {
        for (uint i=0; i<numberSpeciesLimits; i++)
        {
            SpeciesLimit l = speciesLimits[i];
            switch (l.type)
            {
            case SpeciesLimit::MIN:
                if (int(speciesCounts[l.species]) <= l.limit)
                {
                    finalLimitType = lm::io::TrajectoryLimits::MINSPECIESCOUNT;
                    return true;
                }
                break;
            case SpeciesLimit::MAX:
                if (int(speciesCounts[l.species]) >= l.limit)
                {
                    finalLimitType = lm::io::TrajectoryLimits::MAXSPECIESCOUNT;
                    return true;
                }
                break;
            // use the ASCENDING limit checks when starting to the left of the limit
            case SpeciesLimit::DECREASING_ASCENDING:
            	if ((*oparams)[l.species]->getPrev() >= l.limit && (*oparams)[l.species]->get() < l.limit)
                {
                    finalLimitType = lm::io::TrajectoryLimits::DECREASINGORDERPARAMETER;
                    return true;
                }
            	break;
            case SpeciesLimit::INCREASING_ASCENDING:
            	if ((*oparams)[l.species]->getPrev() < l.limit && (*oparams)[l.species]->get() >= l.limit)
                {
                    finalLimitType = lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER;
                    return true;
                }
            	break;
            // use the DESCENDING limit checks when starting to the right of the limit
            case SpeciesLimit::DECREASING_DESCENDING:
                if ((*oparams)[l.species]->getPrev() > l.limit && (*oparams)[l.species]->get() <= l.limit)
                {
                    finalLimitType = lm::io::TrajectoryLimits::DECREASINGORDERPARAMETER;
                    return true;
                }
                break;
            case SpeciesLimit::INCREASING_DESCENDING:
                if ((*oparams)[l.species]->getPrev() <= l.limit && (*oparams)[l.species]->get() > l.limit)
                {
                    finalLimitType = lm::io::TrajectoryLimits::INCREASINGORDERPARAMETER;
                    return true;
                }
                break;
            }

        }
        return false;
    }

protected:
    RandomGenerator::Distributions neededDists;
    RandomGenerator * rng;
    lm::oparam::OParams* oparams;
    lm::tiling::Tilings* tilings;

    // The reaction model.
    class ReactionModel
    {
    public:
        ReactionModel(uint numberSpecies, uint numberReactions);
        virtual ~ReactionModel();
        virtual void build(const uint numberSpecies, const uint numberReactions, const uint * initialSpeciesCounts, const uint * reactionTypesA, const double * k, const int * S, const uint * D, const uint kCols=1);
        virtual void setPropensityFunction(uint reaction, double (*propensityFunction)(double time, uint * speciesCounts, void * args), void * propensityFunctionArg);

        uint numberSpecies;
        uint numberSpeciesToTrack;
        uint numberReactions;
        uint* initialSpeciesCounts;                    // numberSpecies
        uint* reactionTypes;                           // numberReactions
        int* S;                                        // Stoichiometric matrix: numberSpecies x numberReactions
        uint* D;                                       // Dependency matrix: numberSpecies x numberReactions
        void** propensityFunctions;
        void** propensityFunctionArgs;
        list<PropensityArgs*> propensityArgs;

        // Dependency tables.
        uint* numberDependentSpecies;
        uint** dependentSpecies;
        int** dependentSpeciesChange;
        uint* numberDependentReactions;
        uint** dependentReactions;
    };
    ReactionModel* reactionModel;

    // The current limits.
    double maxTime;
    uint numberSpeciesLimits;
    SpeciesLimit* speciesLimits;

    list<TrackedParameter> trackedParameters;

    // The current state.
    uint64_t trajectoryId;
    bool trajectoryStarted;
    uint* speciesCounts;
    uint* previousSpeciesCounts;
    double time;
    double timeStep;    // stores last time step calculated, used for building histogram
    int numberFptTrackedSpecies;
    FPTTracking* fptTrackedSpecies;
    uint numberTilingHists;
    TilingHist* tilingHists;

    // the type limit that stopped the trajectory. only has meaning after the trajectory's last step
    lm::io::TrajectoryLimits::LimitType finalLimitType;
};

}
}

#endif
