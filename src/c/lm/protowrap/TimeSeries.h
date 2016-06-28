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
#ifndef LM_PROTOWRAP_TIMESERIES
#define LM_PROTOWRAP_TIMESERIES

#include <numeric>
#include <sstream>
#include <string>

#include "lm/array/Tuple.h"
#include "lm/io/DegreeAdvancementTimeSeries.pb.h"
#include "lm/io/OrderParameterTimeSeries.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/protowrap/NDArray.h"

namespace lm {
namespace protowrap {

template <typename MsgT> struct TimeSeriesSpecializationBase
{
public:
    typedef lm::message::WorkUnitOutput OutMsgT;

    // typedefs for pointers to functions to get the time series message from the containing WorkUnitOutput message
    typedef MsgT* (OutMsgT::*getTimeSeriesMsgT)();
    typedef const MsgT& (OutMsgT::*getTimeSeriesMsgConstT)() const;

    // typedefs for pointers to getter functions in the time series protobuf
    typedef robertslab::pbuf::NDArray* (MsgT::*getValMsgT)();
    typedef const robertslab::pbuf::NDArray& (MsgT::*getValMsgConstT)() const;
};

// specializations to take care of the small differences between DegreeAdvancementTimesSeries, OrderParameterTimeSeries, etc
template <typename MsgT> struct TimeSeriesSpecialization: public TimeSeriesSpecializationBase<MsgT> {};

template <> struct TimeSeriesSpecialization<lm::io::DegreeAdvancementTimeSeries>: public TimeSeriesSpecializationBase<lm::io::DegreeAdvancementTimeSeries>
{
public:
    typedef uint64_t ValT;

    static const getTimeSeriesMsgT getTimeSeriesMsgFunc;
    static const getTimeSeriesMsgConstT getTimeSeriesMsgConstFunc;

    static const getValMsgT getValMsgFunc;
    static const getValMsgConstT getValMsgConstFunc;

    static const char* mismatchErrorString;
};

template <> struct TimeSeriesSpecialization<lm::io::OrderParameterTimeSeries>: public TimeSeriesSpecializationBase<lm::io::OrderParameterTimeSeries>
{
public:
    typedef double ValT;

    static const getTimeSeriesMsgT getTimeSeriesMsgFunc;
    static const getTimeSeriesMsgConstT getTimeSeriesMsgConstFunc;

    static const getValMsgT getValMsgFunc;
    static const getValMsgConstT getValMsgConstFunc;

    static const char* mismatchErrorString;
};

template <> struct TimeSeriesSpecialization<lm::io::SpeciesTimeSeries>: public TimeSeriesSpecializationBase<lm::io::SpeciesTimeSeries>
{
public:
    typedef int32_t ValT;

    static const getTimeSeriesMsgT getTimeSeriesMsgFunc;
    static const getTimeSeriesMsgConstT getTimeSeriesMsgConstFunc;

    static const getValMsgT getValMsgFunc;
    static const getValMsgConstT getValMsgConstFunc;

    static const char* mismatchErrorString;
};

template <typename MsgT>
class TimeSeries
{
public:
    typedef TimeSeriesSpecialization<MsgT> TSS;
    typedef typename TSS::OutMsgT OutMsgT;
    typedef typename TSS::ValT ValT;
    typedef double TimeT;

//    static const TSS::getTimeSeriesMsgT getTimeSeriesMsgFunc = TSS::getTimeSeriesMsgFunc;
//    static const TSS::getTimeSeriesMsgConstT getTimeSeriesMsgConstFunc = TSS::getTimeSeriesMsgConstFunc;
//
//    static const TSS::getValMsgT getValMsgFunc = TSS::getValMsgFunc;
//    static const TSS::getValMsgConstT getValMsgConstFunc = TSS::getValMsgConstFunc;

    TimeSeries(): msgPtr(NULL),msgConstPtr(NULL) {};
    TimeSeries(const MsgT& msgConstRef): msgPtr(NULL),msgConstPtr(NULL) {setMsg(msgConstRef);}
    TimeSeries(MsgT* msgMutablePtr): msgPtr(NULL),msgConstPtr(NULL) {setMsg(msgMutablePtr);}
    ~TimeSeries() {}

// accessors
    inline const MsgT* getMsg() const {return msgConstPtr;}

    template <typename ValContainerT, typename TimeContainerT>
    inline bool inputCheck(const ValContainerT& valuesInput, const TimeContainerT& timesInput, uint numberOfColumns) const
    {

        // If we have any time series data, add them to the wrapped message.
        if (valuesInput.size() > 0 || timesInput.size() > 0)
        {
            // Make sure the arrays are of a consistent size.
            if (valuesInput.size() == timesInput.size()*numberOfColumns)
            {
                return true;
            }
            else
            {
                Print::printf(Print::ERROR, TSS::mismatchErrorString, valuesInput.size(), numberOfColumns, timesInput.size());
                return false;
            }
        }
        else
        {
            return false;
        }
    }

// mutators
    template <typename ValContainerT, typename TimeContainerT>
    inline void get_arrays(ValContainerT& outputValueContainer, TimeContainerT& outputTimeContainer) const
    {
        valuesWrap.get_data(outputValueContainer);
        timesWrap.get_data(outputTimeContainer);
    }

    MsgT* getMsg()
    {
        if (msgPtr==NULL) throw Exception("Pointer to internal message (msgPtr) set to NULL in lm::protowrap::TimeSeries instance");
        return msgPtr;
    }

    template <typename ValContainerT, typename TimeContainerT>
    inline bool _set_arrays(const ValContainerT& valuesInput, const TimeContainerT& timesInput, uint64_t trajectoryId, uint numberOfColumns, bool compress=false)
    {
        getMsg()->set_trajectory_id(trajectoryId);

        valuesWrap.set_array(valuesInput, utuple(valuesInput.size(), numberOfColumns), compress);
        timesWrap.set_array(timesInput, utuple(timesInput.size()), compress);
    }

    template <typename ValContainerT, typename TimeContainerT>
    inline bool set_arrays(const ValContainerT& valuesInput, const TimeContainerT& timesInput, uint64_t trajectoryId, uint numberOfColumns, bool compress=false)
    {
        if (inputCheck(valuesInput, timesInput, numberOfColumns))
        {
            _set_arrays(valuesInput, timesInput, trajectoryId, numberOfColumns, compress);
            return true;
        }
        else
        {
            return false;
        }
    }

    template <typename ValContainerT, typename TimeContainerT>
    inline bool set_arrays_in_output_msg(OutMsgT* outMsg, const ValContainerT& valuesInput, const TimeContainerT& timesInput, uint64_t trajectoryId, uint numberOfColumns, bool compress=false)
    {
        if (inputCheck(valuesInput, timesInput, numberOfColumns))
        {
            setMsg(outMsg);
            _set_arrays(valuesInput, timesInput, trajectoryId, numberOfColumns, compress);
            return true;
        }
        else
        {
            return false;
        }
    }

    inline TimeSeries* setMsg(MsgT* newMsgMutablePtr)
    {
        msgPtr = newMsgMutablePtr;
        msgConstPtr = newMsgMutablePtr;

        valuesWrap.setMsg((msgPtr->*TSS::getValMsgFunc)());
        timesWrap.setMsg(msgPtr->mutable_times());
        return this;
    }

    inline TimeSeries* setMsg(const MsgT& newMsgConstRef)
    {
        msgPtr = NULL;
        msgConstPtr = &newMsgConstRef;

        valuesWrap.setMsg((msgConstPtr->*TSS::getValMsgConstFunc)());
        timesWrap.setMsg(msgConstPtr->times());
        return this;
    }

    // versions of setMsg that work directly with the containing WorkUnitOutput msg
    inline TimeSeries* setMsg(OutMsgT* outMsg)
    {
        setMsg((outMsg->*TSS::getTimeSeriesMsgFunc)());
        return this;
    }

    inline TimeSeries* setMsg(const OutMsgT& outMsgRef)
    {
        setMsg((outMsgRef.*TSS::getTimeSeriesMsgConstFunc)());
        return this;
    }

public:
    MsgT* msgPtr;
    const MsgT* msgConstPtr;

    mutable lm::protowrap::NDArray<ValT> valuesWrap;
    mutable lm::protowrap::NDArray<TimeT> timesWrap;
    
};

}
}

// version of TimeSeries with both local container storage and an associated protobuf
// originally lived in CMESolver.h, not sure what to do with it now

//template <typename MsgT>
//class TimeSeries
//{
//public:
//    typedef lm::protowrap::TimeSeries<MsgT>::Element Element;
//    typedef double TimeT;
//
//    typedef std::vector<Element> ValContainerT;
//    typedef std::vector<TimeT> TimeContainerT;
//
//    ValContainerT values;
//    TimeContainerT times;
//
//    mutable lm::protowrap::TimeSeries<MsgT> timeSeriesWrap;
//
//public:
//    void deserializeFrom(const MsgT& msgRef)
//    {
//        timeSeriesWrap.setMsg(msgRef);
//        timeSeriesWrap.get_arrays(values, times);
//    }
//
//    void serializeTo(MsgT* msg, uint64_t trajectoryId, uint numberOfColumns, bool compress) const
//    {
//        serializeTo(msg, trajectoryId, numberOfColumns, compress, values, times);
//    }
//
//    void serializeTo(MsgT* msg, uint64_t trajectoryId, uint numberOfColumns, bool compress, const ValContainerT& valuesRef, const TimeContainerT& timesRef) const
//    {
//        timeSeriesWrap.setMsg(msg);
//        timeSeriesWrap.set_arrays(valuesRef, timesRef, trajectoryId, numberOfColumns, compress);
//    }
//
//    // versions of the above functions overloaded to work directly with a WorkUnitOutput msg
//    void deserializeFrom(const lm::message::WorkUnitOutput& outputMsgRef)
//    {
//        timeSeriesWrap.setMsg(outputMsgRef);
//        timeSeriesWrap.get_arrays(values, times);
//    }
//
//    void serializeTo(lm::message::WorkUnitOutput* outputMsg, uint64_t trajectoryId, uint numberOfColumns, bool compress) const
//    {
//        serializeTo(outputMsg, trajectoryId, numberOfColumns, compress, values, times);
//    }
//
//    void serializeTo(lm::message::WorkUnitOutput* outputMsg, uint64_t trajectoryId, uint numberOfColumns, bool compress, const ValContainerT& valuesRef, const TimeContainerT& timesRef) const
//    {
//        timeSeriesWrap.set_arrays_in_output_msg(outputMsg, valuesRef, timesRef, trajectoryId, numberOfColumns, compress);
//    }
//};


#endif /* LM_PROTOWRAP_TIMESERIES */
