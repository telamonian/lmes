/*
 * University of Illinois Open Source License
 * Copyright 2008-2012 Luthey-Schulten Group,
 * Copyright 2012-2016 Roberts Group,
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

#include "lm/array/Tuple.h"
#include "lm/EnumHelper.h"
#include "lm/Math.h"
#include "lm/Types.h"
#include "lm/cme/ReactionModel.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/LimitTracking.pb.h"
#include "lm/io/OrderParameterFirstPassageTimes.pb.h"
#include "lm/io/ParameterValues.pb.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/main/Main.h"
#include "lm/me/MESolver.h"
#include "lm/me/PropensityFunction.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/oparam/OrderParameterFunction.h"
#include "lm/protowrap/NDArray.h"
#include "lm/protowrap/TimeSeries.h"
#include "lm/rng/RandomGenerator.h"
#include "lm/thread/Thread.h"
#include "lm/tiling/Tilings.h"
#include "lm/trajectory/TrajectoryLimits.h"

using std::list;
using std::map;
using std::pair;
using std::string;
using std::vector;
using lm::me::MESolver;
using lm::rng::RandomGenerator;
using lm::trajectory::TrajectoryLimit;

namespace lm {

namespace ioFirstOrderPropensity {
class ReactionModel;
}

namespace cme {

class CMESolver : public MESolver
{
protected:
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

    class OParamFPTTracking
    {
    public:
        typedef lm::io::OrderParameterFirstPassageTimes MsgT;
        typedef double ValueT;
        typedef std::deque<ValueT> ValueContainerT;
        typedef double TimeT;
        typedef std::deque<TimeT> TimeContainerT;

        uint oparamID;
        ValueT minValueAchieved;
        ValueT maxValueAchieved;
        ValueContainerT fptValues;
        TimeContainerT fptTimes;

        mutable lm::protowrap::NDArray<ValueT> fptValuesWrap;
        mutable lm::protowrap::NDArray<TimeT> fptTimesWrap;

        void deserializeFrom(const MsgT& opFPTMsgRef)
        {
            oparamID = opFPTMsgRef.order_parameter_id();

            fptValuesWrap.setMsg(opFPTMsgRef.order_parameter_value());
            fptValuesWrap.get_data(fptValues);

            fptTimesWrap.setMsg(opFPTMsgRef.first_passage_time());
            fptTimesWrap.get_data(fptTimes);

            minValueAchieved = fptValues.front();
            maxValueAchieved = fptValues.back();
        }
        
        void serializeTo(MsgT* opFPTMsg, uint64_t trajectoryId) const
        {
            serializeTo(opFPTMsg, trajectoryId, fptValues, fptTimes);
        }

        void serializeTo(MsgT* opFPTMsg, uint64_t trajectoryId, const ValueContainerT& fptValuesRef, const TimeContainerT& fptTimesRef) const
        {
            opFPTMsg->set_trajectory_id(trajectoryId);
            opFPTMsg->set_order_parameter_id(oparamID);

            fptValuesWrap.setMsg(opFPTMsg->mutable_order_parameter_value());
            fptValuesWrap.set_array(fptValuesRef, utuple(fptValuesRef.size()), false);

            fptTimesWrap.setMsg(opFPTMsg->mutable_first_passage_time());
            fptTimesWrap.set_array(fptTimesRef, utuple(fptTimesRef.size()), false);
        }
    };

    class LimitTracking
    {
    public:
        typedef lm::io::LimitTracking MsgT;
        typedef uint64_t DegreeAdvancementT;
        typedef double OrderParameterT;
        typedef int SpeciesT;
        typedef double TimeT;

        typedef std::vector<DegreeAdvancementT> DegreeAdvancementContainerT;
        typedef std::vector<OrderParameterT> OrderParameterContainerT;
        typedef std::vector<SpeciesT> SpeciesContainerT;
        typedef std::vector<TimeT> TimeContainerT;

        int limitID;
        MsgT::RecordingOption recordingOption;

        DegreeAdvancementContainerT degreeAdvancements;
        OrderParameterContainerT orderParameterValues;
        SpeciesContainerT speciesCounts;
        TimeContainerT times;

        mutable lm::protowrap::NDArray<DegreeAdvancementT> degreeAdvancmentsWrap;
        mutable lm::protowrap::NDArray<OrderParameterT> orderParameterWrap;
        mutable lm::protowrap::NDArray<SpeciesT> speciesWrap;
        mutable lm::protowrap::NDArray<TimeT> timesWrap;

    private:
        bool nonterminating;
        bool hasCountdown;
        int64_t countdown;

    public:
        void deserializeFrom(const MsgT& msgRef)
        {
            limitID = msgRef.limit_id();
            recordingOption = msgRef.recording_option();
            nonterminating = msgRef.nonterminating();
            hasCountdown = msgRef.has_countdown();
            countdown = msgRef.countdown();

            degreeAdvancmentsWrap.setMsg(msgRef.degree_advancements());
            degreeAdvancmentsWrap.get_data(degreeAdvancements);

            orderParameterWrap.setMsg(msgRef.order_parameter_values());
            orderParameterWrap.get_data(orderParameterValues);

            speciesWrap.setMsg(msgRef.species_counts());
            speciesWrap.get_data(speciesCounts);

            timesWrap.setMsg(msgRef.times());
            timesWrap.get_data(times);
        }

        bool getEnabled()
        {
            // if the limit tracking has a countdown, use this to determine if tracking is currently enabled
            if (hasCountdown)
            {
                // there are 3 cases for the value of countdown that we consider important
                if      (countdown > 0)  return true;     // case one: if countdown is greater than zero, enable tracking
                else if (countdown == 0) return true;     // case two: if countdown is exactly zero, enable tracking
                else                     return false;    // case three: if countdown is less than zero, disable tracking
            }
            // if the limit tracking has no countdown, by default tracking is enabled
            else
            {
                return true;
            }
        }

        bool getSignalTermination()
        {
            bool doSignal;
            // if the limit tracking has a countdown, use this to determinate if we should signal for termination of the trajectory
            if (hasCountdown)
            {
                // there are 3 cases for the value of countdown that we consider important
                if      (countdown > 0)  doSignal = false;    // case one: if countdown is greater than zero, don't signal for termination
                else if (countdown == 0) doSignal = true;     // case two: if countdown is exactly zero, signal for termination
                else                     doSignal = true;     // case three: if countdown is less than zero, signal for termination
            }
            // if the limit tracking does not have a countdown, by default we  signal for termination
            else
            {
                doSignal = true;
            }

            // skip signaling for termination if the limit tracking is marked nonterminating
            return doSignal and nonterminating;
        }

        // handle necessary tasks when the associated limit is triggered (eg decrement countdown)
        void limitTriggered()
        {
            if (hasCountdown)
            {
                // decrement countdown
                --countdown;
            }
        }

        void serializeTo(MsgT* msg, uint64_t trajectoryId) const
        {
            serializeTo(msg, trajectoryId, degreeAdvancements, orderParameterValues, speciesCounts, times);
        }

        void serializeTo(MsgT* msg, uint64_t trajectoryId, const DegreeAdvancementContainerT& degreeAdvancementsRef,
                         const OrderParameterContainerT& orderParameterValuesRef, const SpeciesContainerT& speciesCountsRef,
                         const TimeContainerT& timesRef) const
        {
            msg->set_trajectory_id(trajectoryId);
            msg->set_limit_id(limitID);
            if (hasCountdown) msg->set_countdown(countdown);

            degreeAdvancmentsWrap.setMsg(msg->mutable_degree_advancements());
            degreeAdvancmentsWrap.set_array(degreeAdvancementsRef, utuple(degreeAdvancementsRef.size()), false);

            orderParameterWrap.setMsg(msg->mutable_order_parameter_values());
            orderParameterWrap.set_array(orderParameterValuesRef, utuple(orderParameterValuesRef.size()), false);

            speciesWrap.setMsg(msg->mutable_species_counts());
            speciesWrap.set_array(speciesCountsRef, utuple(speciesCountsRef.size()), false);

            timesWrap.setMsg(msg->mutable_times());
            timesWrap.set_array(timesRef, utuple(timesRef.size()), false);
        }
    };
    typedef std::map<int, LimitTracking> TrackingMapT;

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

    template <typename MsgT>
    class TimeSeries
    {
    public:
        typedef lm::protowrap::TimeSeries<MsgT>::ValT ValT;
        typedef double TimeT;

        typedef std::vector<ValT> ValContainerT;
        typedef std::vector<TimeT> TimeContainerT;

        ValContainerT values;
        TimeContainerT times;

        mutable lm::protowrap::TimeSeries<MsgT> timeSeriesWrap;

    public:
        void deserializeFrom(const MsgT& msgRef)
        {
            timeSeriesWrap.setMsg(msgRef);
            timeSeriesWrap.get_arrays(values, times);
        }

        void serializeTo(MsgT* msg, uint64_t trajectoryId, uint numberOfColumns, bool compress) const
        {
            serializeTo(msg, trajectoryId, numberOfColumns, compress, values, times);
        }

        void serializeTo(MsgT* msg, uint64_t trajectoryId, uint numberOfColumns, bool compress, const ValContainerT& valuesRef, const TimeContainerT& timesRef) const
        {
            timeSeriesWrap.setMsg(msg);
            timeSeriesWrap.set_arrays(valuesRef, timesRef, trajectoryId, numberOfColumns, compress);
        }

    // versions of the above functions overloaded to work directly with a WorkUnitOutput msg
        void deserializeFrom(const lm::message::WorkUnitOutput& outputMsgRef)
        {
            timeSeriesWrap.setMsg(outputMsgRef);
            timeSeriesWrap.get_arrays(values, times);
        }

        void serializeTo(lm::message::WorkUnitOutput* outputMsg, uint64_t trajectoryId, uint numberOfColumns, bool compress) const
        {
            serializeTo(outputMsg, trajectoryId, numberOfColumns, compress, values, times);
        }

        void serializeTo(lm::message::WorkUnitOutput* outputMsg, uint64_t trajectoryId, uint numberOfColumns, bool compress, const ValContainerT& valuesRef, const TimeContainerT& timesRef) const
        {
            timeSeriesWrap.set_arrays_in_output_msg(outputMsg, valuesRef, timesRef, trajectoryId, numberOfColumns, compress);
        }
    };

public:
    CMESolver(RandomGenerator::Distributions neededDists);
    virtual ~CMESolver();
    virtual void setComputeResources(vector<int> cpus, vector<int> gpus);
    virtual bool needsReactionModel() {return true;}
    virtual void setReactionModel(const lm::input::ReactionModel& rm);
    virtual bool needsDiffusionModel() {return false;}
    virtual void setDiffusionModel(const lm::input::DiffusionModel& dm) {}
    virtual void setOrderParameters(const lm::input::OrderParameters& opsBuf);
    virtual void setTilings(const lm::input::Tilings& tilingsBuf);
    virtual void setLimits(const lm::input::TrajectoryLimits& limits);
    virtual void setOutputOptions(const lm::input::OutputOptions& outputOptions);
    virtual void reset();
    virtual void getState(lm::io::TrajectoryState* state, uint trajectoryNumber=0);
    virtual void setState(const lm::io::TrajectoryState& state, uint trajectoryNumber=0);
    virtual lm::message::WorkUnitStatus::Status getStatus(uint trajectoryNumber=0);

protected:
    inline void performReactionEvent(uint r)
    {
        // Update the counts according to the dependency tables.
        for (int i=0; i<(int)reactionModel->numberDependentSpecies[r]; i++)
        {
            speciesCounts[reactionModel->dependentSpecies[r][i]] += reactionModel->dependentSpeciesChange[r][i];
        }
        if (hasUpdateSpeciesCountsListeners) callUpdateSpeciesCountsListeners(r);
    }

    inline void callUpdateSpeciesCountsListeners(uint r)
    {
        // Update the degree advancement, if enabled
        if (numberDegreeAdvancements > 0)
        {
            degreeAdvancements[r]++;
        }

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

        // Update any order parameters.
        for (int i=0; i<numberOrderParameters; i++)
        {
            orderParameterPreviousValues[i] = orderParameterValues[i];
            orderParameterValues[i] = orderParameterFunctions[i]->calculate(time, speciesCounts, reactionModel->numberSpecies);
        }

        // Update the order parameter first passage time tables.
        for (int i=0; i<numberFptTrackedOrderParameters; i++)
        {
            // rounding version
            //double opVal = trunc(orderParameterValues[fptTrackedOrderParameters[i].oparamID]);
            double opVal = orderParameterValues[fptTrackedOrderParameters[i].oparamID];

            if (opVal < fptTrackedOrderParameters[i].minValueAchieved)
            {
//                double stepDown = floor(fptTrackedOrderParameters[i].minValueAchieved);
//                while (opVal < stepDown)
//                {
//                    fptTrackedOrderParameters[i].fptValues.push_front(--stepDown);
//                    fptTrackedOrderParameters[i].fptTimes.push_front(time);
//                }
                fptTrackedOrderParameters[i].minValueAchieved = opVal;
                fptTrackedOrderParameters[i].fptValues.push_front(opVal);
                fptTrackedOrderParameters[i].fptTimes.push_front(time);
            }
            if (opVal > fptTrackedOrderParameters[i].maxValueAchieved)
            {
//                double stepUp = ceil(fptTrackedOrderParameters[i].minValueAchieved);
//                while (opVal > stepUp)
//                {
//                    fptTrackedOrderParameters[i].fptValues.push_front(++stepUp);
//                    fptTrackedOrderParameters[i].fptTimes.push_front(time);
//                }
                fptTrackedOrderParameters[i].maxValueAchieved = opVal;
                fptTrackedOrderParameters[i].fptValues.push_back(opVal);
                fptTrackedOrderParameters[i].fptTimes.push_back(time);
            }
        }
        
//        // Update any tilingHists.
//        if (tilings != NULL)
//        {
//            for (int i=0;i<numberTilingHists;i++)
//            {
//                tilingHists[i].tileVals[(*tilings)[tilingHists[i].tilingID]->getTileIndex((*oparams)[(*tilings)[tilingHists[i].tilingID]->getOrderParameterID()]->get())] += timeStep;
//            }
//        }
    }
    bool isTrajectoryOutsideLimits();

protected:
    RandomGenerator::Distributions neededDists;
    RandomGenerator * rng;
    ReactionModel* reactionModel;
    bool hasUpdateSpeciesCountsListeners;
    lm::tiling::Tilings* tilings;

    // Degree advancement tracking
    int32_t numberDegreeAdvancements;

    // Order parameter function.
    int32_t numberOrderParameters;
    lm::oparam::OrderParameterFunction** orderParameterFunctions;

    // Trajectory status.
    lm::message::WorkUnitStatus::Status status;

    // Limits for the trajectory.
    lm::trajectory::TrajectoryLimits trajectoryLimits;
    double timeLimit;
    size_t numberLimits;
    TrajectoryLimit* limits;
    TrajectoryLimit* limitReached;
    int32_t limitIDReached;
    lm::input::TrajectoryLimit::LimitType limitTypeReached;

    // Output options.
    bool writeDegreeAdvancementTimeSeries, writeOrderParameterTimeSeries, writeSpeciesTimeSeries;
    double degreeAdvancementWriteInterval, orderParameterWriteInterval, speciesWriteInterval;

    // First passage time variables.
    int numberFptTrackedSpecies, numberFptTrackedOrderParameters;
    FPTTracking* fptTrackedSpecies;
    OParamFPTTracking* fptTrackedOrderParameters;

    // limit tracking variables
    TrackingMapT trackedLimits;

    // The current state.
    uint64_t* degreeAdvancements;
    double* orderParameterValues;
    double* orderParameterPreviousValues;
    int32_t* speciesCounts;
    double time;
    double timeStep;    // stores last time step calculated, used for building histogram
    uint64_t trajectoryId;
    bool trajectoryStarted;

    uint numberTilingHists;
    TilingHist* tilingHists;
};

}
}

#endif
