/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
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
#ifndef LM_PROTWRAP_FFLUXPHASEOUTPUT_H_
#define LM_PROTWRAP_FFLUXPHASEOUTPUT_H_

#include <algorithm>
#include <limits>
#include <map>
#include <vector>

#include "lm/EnumHelper.h"
#include "lm/fflux/FFluxPhaseZeroTrajectory.h"
#include "lm/fflux/io/FFluxPhaseOutput.pb.h"
#include "lm/limit/LimitCheckFunctions.h"
#include "lm/limit/LimitTrackingWrap.h"
#include "lm/io/LimitTracking.pb.h"
#include "lm/protowrap/NDArray.h"
#include "lm/protowrap/RepeatedMap.h"
#include "lm/rng/RandomGenerator.h"
#include "lm/rng/XORShift.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/Types.h"

#ifdef OPT_CUDA
#include "lm/rng/XORWow.h"
#endif

using lm::protowrap::RepeatedMap;

namespace lm {
namespace protowrap {

typedef lm::fflux::io::FFluxPhaseOutput FFluxPhaseOutputMsg;
typedef lm::fflux::io::StartPoint StartPointMsg;
typedef lm::fflux::io::EndPoint EndPointMsg;

typedef std::vector<int32_t> PointKey;

// map key getter and setter for the EndPointMap typedef
template <typename PointMsg>
PointKey getPointKey(const PointMsg& pointMsgConst)
{
    return PointKey(pointMsgConst.species_coordinates().begin(), pointMsgConst.species_coordinates().end());
}
template <typename PointMsg>
void setPointKey(PointMsg* pointMsg, const PointKey& pointKey)
{
    pointMsg->mutable_species_coordinates()->Clear();
    for (PointKey::const_iterator it=pointKey.begin();it!=pointKey.end();it++)
    {
        pointMsg->add_species_coordinates(*it);
    }
}
typedef RepeatedMap<EndPointMsg, PointKey, &getPointKey, &setPointKey> EndPointMap;
// each entry in an EndPointVector is a Pair of a pointer to an endpoint and an index. The index allows you to lookup the time
typedef PairVector<lm::fflux::io::EndPoint*, int> EndPointVector;

//class EndPoint
//{
//    EndPointMsg* wrappedMsgPtr;
//};
//
//class StartPoint
//{
//public:
//    StartPointMsg* wrappedMsgPtr;
//    EndPointMap successfulEndPointMap;
//    EndPointMap failedEndPointMap;
//};
//typedef std::map<PointKey, StartPoint> StartPointMap;

class FFluxPhaseOutputWrap
{
public:
    typedef FFluxPhaseOutputMsg Msg;

    FFluxPhaseOutputWrap(size_t randomCacheSize=10*KIBI)
    :msgPtr(NULL),rng(NULL),randomDoublesStart(NULL),randomDoubles(NULL),randomDoublesEnd(NULL),randomIndexesStart(NULL),
     randomIndexes(NULL),randomIndexesEnd(NULL),randomCacheSize(randomCacheSize),randomIndexesDirty(true)
    {
    }

    FFluxPhaseOutputWrap(Msg* msgMutablePtr, size_t randomCacheSize=10*KIBI)
    :msgPtr(NULL),rng(NULL),randomDoublesStart(NULL),randomDoubles(NULL),randomDoublesEnd(NULL),randomIndexesStart(NULL),
     randomIndexes(NULL),randomIndexesEnd(NULL),randomCacheSize(randomCacheSize),randomIndexesDirty(true)
    {
        setWrappedMsg(msgMutablePtr);
    }

    ~FFluxPhaseOutputWrap() {destructRng(); destructRandomIndexes();}

// mutators
    void addEndPointPhaseZero(const lm::io::TrajectoryState& trajectoryState, const lm::trajectory::Trajectory& trajectory, int burnInCount)
    {
        // TODO: fix burn in count
        // TODO: include consistency check constraining (phaseZeroSamples > burnInCount) somewhere

        // cast the Trajectory reference to a PhaseZeroTrajectory reference (its true type)
        const lm::fflux::FFluxPhaseZeroTrajectory& phaseZeroTrajectory = static_cast<const lm::fflux::FFluxPhaseZeroTrajectory&>(trajectory);

        // set wrapper on the limit_trackings field
        limitTrackingsWrap.setWrappedField(trajectoryState.limit_tracking_list().limit_trackings());

        // consistency checks
        if (limitTrackingsWrap.size()!=3)
        {
            throw ConsistencyException("Finished Forward Flux phase zero trajectories should have 3 tracked limits in their outputs; trajectory id %llu has %d", trajectoryState.trajectory_id(), limitTrackingsWrap.size());
        }
        for (int i=0;i<3;i++)
        {
            if (limitTrackingsWrap.Get(i).limit_id()!=i)
            {
                throw ConsistencyException("Finished Forward Flux phase zero trajectories should have 3 tracked limits in their outputs with limit_ids {0, 1, 2}; trajectory id %llu has limit tracking index %d with limit_id %d", trajectoryState.trajectory_id(), i, limitTrackingsWrap.Get(i).limit_id());
            }
        }

        // fetch forth some data from limit 0 (ie forward flux) tracking
        speciesCountWrap.setWrappedMsg(limitTrackingsWrap.Get(0).species_counts());
        timeWrapForwardFlux.setWrappedMsg(limitTrackingsWrap.Get(0).times());

        if (speciesCountWrap.shape(0) > 0)
        {
            int32_t* speciesCountDataForwardFlux = speciesCountWrap.get_data();
            double* timeDataForwardFlux = timeWrapForwardFlux.get_data();

            // get some metadata about the species counts
            uint rows = speciesCountWrap.shape(0);
            uint columns = speciesCountWrap.shape(1);

            // check that the shapes of times and species count ndarrays are consistent
            if (rows!=timeWrapForwardFlux.size())
            {
                throw ConsistencyException("In FFluxPhaseOutputWrapper::addEndPointPhaseZero, the number of rows in speciesCountDataForwardFlux: %d did not equal the number of rows in timeDataForwardFlux: %d.", rows, timeWrapForwardFlux.size());
            }

            // set total time, subtract out burn in time, and mark that we have "blocked" (ie accounted for) trajectory time up to the burn in time
            double burnInTime = burnInCount > 0 ? timeDataForwardFlux[burnInCount - 1] : 0.0;
            double workUnitStartTime = phaseZeroTrajectory.getSimTime();
            double workUnitEndTime = trajectoryState.cme_state().species_counts().time(trajectoryState.cme_state().species_counts().time_size() - 1);

            // load points from forward flux events into successful endpoints
            for (int i=burnInCount;i<rows;i++)
            {
                pointKey.assign(speciesCountDataForwardFlux + i*columns, speciesCountDataForwardFlux + (i + 1)*columns);
                EndPointMsg* endPointMsg = successfulEndPointMap[pointKey];
                endPointMsg->set_count(endPointMsg->count() + 1);
                endPointMsg->add_times(timeDataForwardFlux[i]);

                // TODO: decide if the creation of endPointVector should be done one at a time (as below) or all at once
                endPointVector.push_back(std::make_pair(endPointMsg, endPointMsg->count() - 1));
                randomIndexesDirty = true;
            }

            if (speciesCountWrap.compressed_deflate()) delete[] speciesCountDataForwardFlux;
            if (timeWrapForwardFlux.compressed_deflate()) delete[] timeDataForwardFlux;

            // add to the summary metrics
            msgPtr->set_successful_trajectories_launched_count(msgPtr->successful_trajectories_launched_count() + rows);

            // correct workUnitEndTime for burn in and for time spent outside of the region of the starting basin (see Valeriani 2007, Dinner 2010)
            msgPtr->set_successful_trajectories_launched_total_time(msgPtr->successful_trajectories_launched_total_time() + (workUnitEndTime - workUnitStartTime - phaseZeroTrajectory.timeInOtherBasinsLast));
        }
    }

//    // this function encapsulates part of addEndPointFromLimitTrackingsPhaseZero, and so relies on the consistency checks run at the begininng of that function
//    double getOtherBasinTimeCorrection(const lm::io::TrajectoryState& trajectoryState, double startTime, double endTime)
//    {
//        double timeCorrection = 0.0;
//
////        timeWrapBackwardFlux.setWrappedMsg(limitTrackingsWrap.Get(1).times());
////        timeWrapOtherBasinEntry.setWrappedMsg(limitTrackingsWrap.Get(2).times());
//
//        // If the trajectory ever passed into another basin, get the sum time of the intervals between entry into another basin and reentry into the starting basin
//        if (timeWrapOtherBasinEntry.size() > 0)
//        {
//            double *timeDataBackwardFlux, *timeDataBackwardFluxEnd, *timeDataOtherBasinEntry, *timeDataOtherBasinEntryEnd;
//            timeDataOtherBasinEntry = timeWrapOtherBasinEntry.get_data(true);
//            timeDataOtherBasinEntryEnd = timeDataOtherBasinEntry +  timeWrapOtherBasinEntry.size();
//            timeDataBackwardFlux = timeWrapBackwardFlux.get_data(true);
//            timeDataBackwardFluxEnd = timeDataBackwardFlux +  timeWrapBackwardFlux.size();
//
//            timeCorrection = sumTimeIntervals(timeDataOtherBasinEntry, timeDataOtherBasinEntryEnd, timeDataBackwardFlux, timeDataBackwardFluxEnd, startTime, endTime);
//
//            if (timeWrapBackwardFlux.compressed_deflate()) delete[] timeDataBackwardFlux;
//            if (timeWrapOtherBasinEntry.compressed_deflate()) delete[] timeDataOtherBasinEntry;
//        }
//
//        return timeCorrection;
//    }
//
//    // TODO: handle startTime and endTime in a more robust way and/or checked way
//    static double sumTimeIntervals(double* entryTimes, double* entryTimesEnd, double* exitTimes, double* exitTimesEnd, double startTime=0.0, double endTime=std::numeric_limits<double>::infinity())
//    {
//        double sumTime = 0.0;
//        checkLimitCurry<TrajLimEnums::MAX, false, double> greaterThanCurry(0.0);
//
//        // find the first interval entry time after the start time
////        entryTimes = checkLimitRangeAdapter<TrajLimEnums::MAX, false, double>(std::find_if, entryTimes, entryTimesEnd, startTime);
//        entryTimes = std::find_if(entryTimes, entryTimesEnd, greaterThanCurry.setLimitVal(startTime));
//
//        // if we didn't find an appropriate entry time, just return 0.0
//        if (entryTimes==entryTimesEnd) return sumTime;
//
//        while (true)
//        {
//            // try to find the next interval exit time
////            exitTimes = checkLimitRangeAdapter<TrajLimEnums::MAX, false, double>(std::find_if, exitTimes, exitTimesEnd, *entryTimes);
//            exitTimes = std::find_if(exitTimes, exitTimesEnd, greaterThanCurry.setLimitVal(*entryTimes));
//            if (exitTimes==exitTimesEnd)               // If have an entryTime with no exitTime, add the difference between the last entryTime and the endTime, and then break
//            {
//                if (endTime!=std::numeric_limits<double>::infinity()) sumTime += (endTime - *entryTimes);
//                return sumTime;
//            }
//            else                                       // Otherwise, we have found the next exit time. Add the length of this interval to the sumTime
//            {
//                sumTime += (*exitTimes - *entryTimes);
//            }
//
//            // try to find the next interval entry time
////            entryTimes = checkLimitRangeAdapter<TrajLimEnums::MAX, false, double>(std::find_if, entryTimes, entryTimesEnd, *exitTimes);
//            entryTimes = std::find_if(entryTimes, entryTimesEnd, greaterThanCurry.setLimitVal(*exitTimes));
//            if (entryTimes==entryTimesEnd)            // If we can't find another entryTime, there are no more intervals so break
//            {
//                return sumTime;
//            }
//        }
//    }

    void addEndPoint(const lm::io::TrajectoryState& trajectoryState, const lm::trajectory::Trajectory& trajectory)
    {
        // set wrapper on the limit_trackings field
        limitTrackingsWrap.setWrappedField(trajectoryState.limit_tracking_list().limit_trackings());

        // consistency checks
        if (limitTrackingsWrap.size()!=2) throw ConsistencyException("Finished Forward Flux phase n>0 trajectories should have 2 tracked limits in their outputs; trajectory id %llu has %d", trajectoryState.trajectory_id(), limitTrackingsWrap.size());
        for (int i=0;i<2;i++) {if (limitTrackingsWrap.Get(i).limit_id()!=i) throw ConsistencyException("Finished Forward Flux phase n>0 trajectories should have 2 tracked limits in their outputs with limit_ids {0, 1}; trajectory id %llu has limit tracking index %d with limit_id %d", trajectoryState.trajectory_id(), i, limitTrackingsWrap.Get(i).limit_id());}

        // fetch forth some time data from limit 0 (ie backward flux) and limit 1 (ie forward flux) tracking
        timeWrapBackwardFlux.setWrappedMsg(limitTrackingsWrap.Get(0).times());
        double* timeDataBackwardFlux = timeWrapBackwardFlux.get_data(true);
        timeWrapForwardFlux.setWrappedMsg(limitTrackingsWrap.Get(1).times());
        double* timeDataForwardFlux = timeWrapForwardFlux.get_data(true);

        // check if this trajectory fluxed backwards or forwards (and make sure it didn't somehow do both)
        if (timeWrapBackwardFlux.size()==1 and timeWrapForwardFlux.size()==0)       // branch for "failed" trajectories (ie ones that fluxed backward)
        {
            msgPtr->set_failed_trajectories_launched_count(msgPtr->failed_trajectories_launched_count() + 1);
            msgPtr->set_failed_trajectories_launched_total_time(msgPtr->failed_trajectories_launched_total_time() + timeDataBackwardFlux[0] - trajectory.getSimTime());
        }
        else if (timeWrapBackwardFlux.size()==0 and timeWrapForwardFlux.size()==1)  // branch for "successful" trajectories (ie ones that fluxed forward)
        {
            msgPtr->set_successful_trajectories_launched_count(msgPtr->successful_trajectories_launched_count() + 1);
            msgPtr->set_successful_trajectories_launched_total_time(msgPtr->successful_trajectories_launched_total_time() + timeDataForwardFlux[0] - trajectory.getSimTime());

            // since this is data from a "successful" trajectory (ie one that fluxed forward), add its endpoint to the list used to initialize the next phase
            speciesCountWrap.setWrappedMsg(limitTrackingsWrap.Get(1).species_counts());
            int32_t* speciesCountData = speciesCountWrap.get_data(true);

            uint columns = speciesCountWrap.shape(1);
            pointKey.assign(speciesCountData, speciesCountData + columns);
            EndPointMsg* endPointMsg = successfulEndPointMap[pointKey];
            endPointMsg->set_count(endPointMsg->count() + 1);
            endPointMsg->add_times(timeDataForwardFlux[0]);

            // TODO: decide if the creation of endPointVector should be done one at a time (as below) or all at once
            endPointVector.push_back(std::make_pair(endPointMsg, endPointMsg->count() - 1));
            randomIndexesDirty = true;

            if (speciesCountWrap.compressed_deflate()) delete[] speciesCountData;
        }
        else throw ConsistencyException("Finished Forward Flux phase n>0 trajectory %llu has recorded %d backward flux events and %d forward flux events; it should have either 1 forward or 1 backward flux event, and not both", trajectoryState.trajectory_id(), timeWrapBackwardFlux.size(), timeWrapForwardFlux.size());

        if (timeWrapBackwardFlux.compressed_deflate()) delete[] timeDataBackwardFlux;
        if (timeWrapForwardFlux.compressed_deflate()) delete[] timeDataForwardFlux;
    }

    const EndPointVector::Pair& getEndPointCyclic(size_t index) const
    {
        // take the modulus of index to wrap it back around to somewhere within the bounds of endPointVector
        index %= endPointVector.size();
        return endPointVector[index];
    }

    const EndPointVector::Pair& getEndPointUniformRandom() const
    {
        uint32_t ri = getRandomIndex();
        return endPointVector[ri];
    }

    Msg* wrappedMsg()
    {
        return msgPtr;
    }

    void setWrappedMsg(Msg* newMsgMutablePtr)
    {
        msgPtr = newMsgMutablePtr;

        successfulEndPointMap.setWrappedField(wrappedMsg()->mutable_successful_trajectory_end_points());
        rebuildEndPointVector();
    }

    void setWrappedMsgNull()
    {
        msgPtr = NULL;

        successfulEndPointMap.setWrappedFieldNull();
        endPointVector.clear();
    }

protected:
    void initRng() const {destructRng(); rng = new lm::rng::XORShift(0, 0);}
    void initRng(int cudaDevice) const
    {
        destructRng();

        #ifdef OPT_CUDA
        // Create the cuda based rng.
        rng = new lm::rng::XORWow(cudaDevice, 0, 0, lm::rng::RandomGenerator::UNIFORM);
        #endif

        if (rng == NULL)
        {
            rng = new lm::rng::XORShift(0, 0);
        }
    }
    void initRandomIndexes(size_t size) const
    {
        initRng();
        destructRandomIndexes();

        randomCacheSize = size;
        randomIndexesStart = new uint32_t[randomCacheSize];
        randomDoublesStart = new double[randomCacheSize];
        
        // set range pointers to the ends of the arrays, to match the start pointers
        randomIndexesEnd = randomIndexesStart + randomCacheSize;
        randomDoublesEnd = randomDoublesStart + randomCacheSize;
    }

    uint32_t getRandomIndex() const
    {
        // refill the cache of random numbers, if needed
        if (randomIndexes==randomIndexesEnd) fillRandomIndex();

        // if endPointVector has changed in size since the last time we ran .fillRandomIndex(), rescale a randomDouble on the fly to get a randomIndex
        if (randomIndexesDirty)
        {
            randomIndexes++;
            return getIndexFromDouble(*randomDoubles++);
        }
            // otherwise, our cache of randomIndexes is still good, so just take from that
        else
        {
            randomDoubles++;
            return *randomIndexes++;
        }
    }

    inline uint32_t getIndexFromDouble(double d) const
    {
        uint32_t ri = static_cast<uint32_t>(floor(d*endPointVector.size()));
        if (ri >= endPointVector.size())
        {
            throw ConsistencyException("FFluxPhaseOutputWrap generated a random index outside the bounds of its endPointVector. ri: %d, endPointVector.size(): %d", ri, endPointVector.size());
        }
        return ri;
    }

    void fillRandomIndex() const
    {
        // initialize the rng stuff for this instance of FFluxPhaseOutputWrap, if needed
        if (randomIndexes==NULL) {initRandomIndexes(randomCacheSize);}

        // get a large quantity of random doubles
        rng->getRandomDoubles(randomDoublesStart, randomCacheSize);

        // set both randomIndexes and randomDoubles to the front of their arrays
        randomIndexes = randomIndexesStart;
        randomDoubles = randomDoublesStart;
        
        // convert the random doubles into random uints that can be used to randomly lookup values in our table of successful trajectory endpoints
        for (;randomIndexes!=randomIndexesEnd and randomDoubles!=randomDoublesEnd;randomIndexes++,randomDoubles++)
        {
            *randomIndexes = getIndexFromDouble(*randomDoubles);
        }

        // randomIndexes has been rebuilt according the current endPointVector.size(), so mark that they match
        randomIndexesDirty = false;

        // reset the ptrs
        randomIndexes = randomIndexesStart;
        randomDoubles = randomDoublesStart;
    }

    void rebuildEndPointVector()
    {
        size_t oldSize = endPointVector.size();
        endPointVector.clear();

        // iterate over all of the endpoints in the successful_trajectory_end_point repeated field
        for (EndPointMap::iterator epit=successfulEndPointMap.begin();epit!=successfulEndPointMap.end();epit++)
        {
            // add an entry to endPointVector for every time a particular endpoint was "seen"
            for (int i=0;i<epit->count();i++)
            {
                endPointVector.push_back(std::make_pair(&*epit, i));
            }
        }

        if (endPointVector.size()!=oldSize) randomIndexesDirty = true;
    }

    void destructRng() const {if (rng!=NULL) delete rng; rng=NULL;}
    void destructRandomIndexes() const
    {
        if (randomIndexesStart!=NULL) delete[] randomIndexesStart; randomIndexesStart = NULL;
        randomIndexes = NULL;
        randomIndexesEnd = NULL;

        if (randomDoublesStart!=NULL) delete[] randomDoublesStart; randomDoublesStart = NULL;
        randomDoubles = NULL;
        randomDoublesEnd = NULL;

        randomCacheSize = 0;
    }

public:
    EndPointMap successfulEndPointMap;

protected:
    Msg* msgPtr;
    PointKey pointKey;
    lm::protowrap::NDArray<int32_t> speciesCountWrap;
    lm::protowrap::NDArray<double> timeWrapForwardFlux, timeWrapBackwardFlux;
    lm::protowrap::Repeated<lm::io::LimitTracking> limitTrackingsWrap;

    // list of pointers into the successful_trajectory_end_points field. Part of the system used to randomly choose some of them
    EndPointVector::T endPointVector;

    // rng used for randomly choosing points from one of the point lists. Caches large quantities of random numbers in an attempt to reduce the turnaround time of WorkUnitFinished messages on the supervisor
    mutable lm::rng::RandomGenerator * rng;
    mutable double *randomDoublesStart, *randomDoubles, *randomDoublesEnd;
    mutable uint32_t *randomIndexesStart, *randomIndexes, *randomIndexesEnd;
    mutable size_t randomCacheSize;

    // flag that indicates if the length of endPointVector has changed since we last refilled randomIndexes
    mutable bool randomIndexesDirty;
};

}
}


#endif /* LM_PROTOWRAP_FFLUXPHASEOUTPUT_H_ */
