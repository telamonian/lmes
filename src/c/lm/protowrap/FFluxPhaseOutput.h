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

#include <map>
#include <vector>
#include <lm/io/LimitTracking.pb.h>

#include "lm/fflux/io/FFluxPhaseOutput.pb.h"
#include "lm/protowrap/NDArray.h"
#include "lm/protowrap/RepeatedMap.h"
#include "lm/Types.h"

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
    pointMsg->mutable_species_coordinates()->clear();
    for (PointKey::const_iterator it=pointKey.begin();it!=pointKey.end();it++)
    {
        pointMsg->add_species_coordinates(*it);
    }
}
typedef RepeatedMap<EndPointMsg, PointKey, &getPointKey, &setPointKey> EndPointMap;
typedef std::map<PointKey, EndPointMap> EndPointMapMap;

//class EndPoint
//{
//    EndPointMsg* msgPtr;
//};
//
//class StartPoint
//{
//public:
//    StartPointMsg* msgPtr;
//    EndPointMap sucessfulEndPointMap;
//    EndPointMap failedEndPointMap;
//};
//typedef std::map<PointKey, StartPoint> StartPointMap;

class FFluxPhaseOutput
{
public:
    typedef FFluxPhaseOutputMsg Msg;

    FFluxPhaseOutput(): msgPtr(NULL) {}
    FFluxPhaseOutput(Msg* msgMutablePtr): msgPtr(NULL) {setMsg(msgMutablePtr);}

// mutators
    Msg* getMsg()
    {
        return msgPtr;
    }

    void setMsg(Msg* newMsgMutablePtr)
    {
        msgPtr = newMsgMutablePtr;

        sucessfulEndPointMap.setRepFieldPtr(getMsg()->mutable_sucessful_trajectory_end_points());
    }

    void addEndpointFromLimitTracking(const lm::io::LimitTracking& limitTracking)
    {
        speciesCountWrap.setMsg(limitTracking.species_counts());
        timeWrap.setMsg(limitTracking.times());
        int32_t* speciesCountData = speciesCountWrap.get_data(true);
        double* timeData = timeWrap.get_data(true);

        uint rows = speciesCountWrap.shape(0);
        uint columns = speciesCountWrap.shape(1);
        for (int i=0;i<rows;i++)
        {
            pointKey.assign(speciesCountData[i*columns], speciesCountData[i*columns+rows]);
            EndPointMsg* endPointMsg = sucessfulEndPointMap[pointKey];
            endPointMsg->set_count(endPointMsg->count() + 1);
            endPointMsg->add_times(timeData[i]);
        }

        if (speciesCountWrap.compressed_deflate()) delete[] speciesCountData;
        if (timeWrap.compressed_deflate()) delete[] timeData;
    }

public:
    Msg* msgPtr;
    EndPointMap sucessfulEndPointMap;

    PointKey pointKey;
    lm::protowrap::NDArray<int32_t> speciesCountWrap;
    lm::protowrap::NDArray<double> timeWrap;
};

}
}


#endif /* LM_PROTOWRAP_FFLUXPHASEOUTPUT_H_ */
