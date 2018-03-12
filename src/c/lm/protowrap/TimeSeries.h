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
#ifndef LM_PROTOWRAP_TIMESERIES_H_
#define LM_PROTOWRAP_TIMESERIES_H_

#include <numeric>
#include <sstream>
#include <string>

#include "lm/io/DegreeAdvancementTimeSeries.pb.h"
#include "lm/io/OrderParameterTimeSeries.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/Print.h"
#include "lm/protowrap/NDArray.h"
#include "lm/Types.h"

namespace lm {
namespace protowrap {

template <typename WrappedMsg> struct TimeSeriesSpecializationBase
{
public:
    typedef lm::message::WorkUnitOutput WorkUnitOutputMsg;

    // typedefs for pointers to functions to get the time series message from the containing WorkUnitOutput message
    typedef WrappedMsg* (WorkUnitOutputMsg::*TimeArrayMsgGetter)();
    typedef const WrappedMsg& (WorkUnitOutputMsg::*TimeArrayMsgConstGetter)() const;

    // typedefs for pointers to getter functions in the time series protobuf
    typedef robertslab::pbuf::NDArray* (WrappedMsg::*ValueArrayMsgGetter)();
    typedef const robertslab::pbuf::NDArray& (WrappedMsg::*ValueArrayMsgConstGetter)() const;
};

// specializations to take care of the small differences between DegreeAdvancementTimesSeries, OrderParameterTimeSeries, etc
template <typename WrappedMsg> struct TimeSeriesSpecialization: public TimeSeriesSpecializationBase<WrappedMsg> {};

template <> struct TimeSeriesSpecialization<lm::io::DegreeAdvancementTimeSeries>: public TimeSeriesSpecializationBase<lm::io::DegreeAdvancementTimeSeries>
{
public:
    typedef uint64_t ValueT;

    static const TimeArrayMsgGetter timeArrayMsgGetter;
    static const TimeArrayMsgConstGetter timeArrayMsgConstGetter;

    static const ValueArrayMsgGetter valueArrayMsgGetter;
    static const ValueArrayMsgConstGetter valueArrayMsgConstGetter;

    static const char* mismatchErrorString;
};

template <> struct TimeSeriesSpecialization<lm::io::OrderParameterTimeSeries>: public TimeSeriesSpecializationBase<lm::io::OrderParameterTimeSeries>
{
public:
    typedef double ValueT;

    static const TimeArrayMsgGetter timeArrayMsgGetter;
    static const TimeArrayMsgConstGetter timeArrayMsgConstGetter;

    static const ValueArrayMsgGetter valueArrayMsgGetter;
    static const ValueArrayMsgConstGetter valueArrayMsgConstGetter;

    static const char* mismatchErrorString;
};

template <> struct TimeSeriesSpecialization<lm::io::SpeciesTimeSeries>: public TimeSeriesSpecializationBase<lm::io::SpeciesTimeSeries>
{
public:
    typedef int32_t ValueT;

    static const TimeArrayMsgGetter timeArrayMsgGetter;
    static const TimeArrayMsgConstGetter timeArrayMsgConstGetter;

    static const ValueArrayMsgGetter valueArrayMsgGetter;
    static const ValueArrayMsgConstGetter valueArrayMsgConstGetter;

    static const char* mismatchErrorString;
};

template <typename WrappedMsg>
class TimeSeries
{
public:
    typedef TimeSeriesSpecialization<WrappedMsg> TSS;
    typedef typename TSS::WorkUnitOutputMsg OutMsgT;
    typedef typename TSS::ValueT ValueT;
    typedef double TimeT;

//    static const TSS::TimeArrayMsgGetter timeArrayMsgGetter = TSS::timeArrayMsgGetter;
//    static const TSS::TimeArrayMsgConstGetter timeArrayMsgConstGetter = TSS::timeArrayMsgConstGetter;
//
//    static const TSS::ValueArrayMsgGetter valueArrayMsgGetter = TSS::valueArrayMsgGetter;
//    static const TSS::ValueArrayMsgConstGetter valueArrayMsgConstGetter = TSS::valueArrayMsgConstGetter;

    TimeSeries(): wrappedMsgPtr(NULL),wrappedMsgConstPtr(NULL) {};
    TimeSeries(const WrappedMsg& msgConstRef): wrappedMsgPtr(NULL),wrappedMsgConstPtr(NULL) {setMsg(msgConstRef);}
    TimeSeries(WrappedMsg* msgMutablePtr): wrappedMsgPtr(NULL),wrappedMsgConstPtr(NULL) {setMsg(msgMutablePtr);}
    ~TimeSeries() {}

// accessors
    template <typename ValueContainer, typename TimeContainer>
    inline bool inputCheck(const ValueContainer& valuesInput, const TimeContainer& timesInput, uint numberOfColumns) const
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

    inline bool wrappedIsNull() const {return wrappedMsgPtr==NULL;}
    inline const WrappedMsg* wrappedMsg() const {return wrappedMsgConstPtr;}

// mutators
    template <typename ValueContainer, typename TimeContainer>
    inline void get_arrays(ValueContainer& outputValueContainer, TimeContainer& outputTimeContainer) const
    {
        valuesWrap.get_data(outputValueContainer);
        timesWrap.get_data(outputTimeContainer);
    }

    WrappedMsg* wrappedMsg()
    {
        if (wrappedMsgPtr==NULL) throw Exception("Pointer to internal message (wrappedMsgPtr) set to NULL in lm::protowrap::TimeSeries instance");
        return wrappedMsgPtr;
    }

    template <typename ValueContainer, typename TimeContainer>
    inline void _set_arrays(const ValueContainer& valuesInput, const TimeContainer& timesInput, uint64_t trajectoryId, uint numberOfColumns, bool compress=false)
    {
        wrappedMsg()->set_trajectory_id(trajectoryId);

        // timesInput.size() is the number of "rows" in this time series
        valuesWrap.set_array(valuesInput, utuple(timesInput.size(), numberOfColumns), compress);
        timesWrap.set_array(timesInput, utuple(timesInput.size()), compress);
    }

    template <typename ValueContainer, typename TimeContainer>
    inline bool set_arrays(const ValueContainer& valuesInput, const TimeContainer& timesInput, uint64_t trajectoryId, uint numberOfColumns, bool compress=false)
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

    template <typename ValueContainer, typename TimeContainer>
    inline bool set_arrays_in_output_msg(OutMsgT* outMsg, const ValueContainer& valuesInput, const TimeContainer& timesInput, uint64_t trajectoryId, uint numberOfColumns, bool compress=false)
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

    inline TimeSeries* setMsg(WrappedMsg* newMsgMutablePtr)
    {
        wrappedMsgPtr = newMsgMutablePtr;
        wrappedMsgConstPtr = newMsgMutablePtr;

        valuesWrap.setWrappedMsg((wrappedMsgPtr->*TSS::valueArrayMsgGetter)());
        timesWrap.setWrappedMsg(wrappedMsgPtr->mutable_times());
        return this;
    }

    inline TimeSeries* setMsg(const WrappedMsg& newMsgConstRef)
    {
        wrappedMsgPtr = NULL;
        wrappedMsgConstPtr = &newMsgConstRef;

        valuesWrap.setWrappedMsg((wrappedMsgConstPtr->*TSS::valueArrayMsgConstGetter)());
        timesWrap.setWrappedMsg(wrappedMsgConstPtr->times());
        return this;
    }

    // versions of setWrappedMsg that work directly with the containing WorkUnitOutput msg
    inline TimeSeries* setMsg(OutMsgT* outMsg)
    {
        setMsg((outMsg->*TSS::timeArrayMsgGetter)());
        return this;
    }

    inline TimeSeries* setMsg(const OutMsgT& outMsgRef)
    {
        setMsg((outMsgRef.*TSS::timeArrayMsgConstGetter)());
        return this;
    }
    inline void setWrappedNull() {wrappedMsgPtr = NULL; wrappedMsgConstPtr = NULL;}

public:
    WrappedMsg* wrappedMsgPtr;
    const WrappedMsg* wrappedMsgConstPtr;

    mutable lm::protowrap::NDArray<ValueT> valuesWrap;
    mutable lm::protowrap::NDArray<TimeT> timesWrap;
};

}
}

// version of TimeSeries with both local container storage and an associated protobuf
// originally lived in CMESolver.h, not sure what to do with it now

//template <typename WrappedMsg>
//class TimeSeries
//{
//public:
//    typedef lm::protowrap::TimeSeries<WrappedMsg>::Element Element;
//    typedef double TimeT;
//
//    typedef std::vector<Element> ValueContainer;
//    typedef std::vector<TimeT> TimeContainer;
//
//    ValueContainer values;
//    TimeContainer times;
//
//    mutable lm::protowrap::TimeSeries<WrappedMsg> timeSeriesWrap;
//
//public:
//    void deserializeFrom(const WrappedMsg& msgRef)
//    {
//        timeSeriesWrap.setWrappedMsg(msgRef);
//        timeSeriesWrap.get_arrays(values, times);
//    }
//
//    void serializeTo(WrappedMsg* msg, uint64_t trajectoryId, uint numberOfColumns, bool compress) const
//    {
//        serializeTo(msg, trajectoryId, numberOfColumns, compress, values, times);
//    }
//
//    void serializeTo(WrappedMsg* msg, uint64_t trajectoryId, uint numberOfColumns, bool compress, const ValueContainer& valuesRef, const TimeContainer& timesRef) const
//    {
//        timeSeriesWrap.setWrappedMsg(msg);
//        timeSeriesWrap.set_arrays(valuesRef, timesRef, trajectoryId, numberOfColumns, compress);
//    }
//
//    // versions of the above functions overloaded to work directly with a WorkUnitOutput msg
//    void deserializeFrom(const lm::message::WorkUnitOutput& outputMsgRef)
//    {
//        timeSeriesWrap.setWrappedMsg(outputMsgRef);
//        timeSeriesWrap.get_arrays(values, times);
//    }
//
//    void serializeTo(lm::message::WorkUnitOutput* outputMsg, uint64_t trajectoryId, uint numberOfColumns, bool compress) const
//    {
//        serializeTo(outputMsg, trajectoryId, numberOfColumns, compress, values, times);
//    }
//
//    void serializeTo(lm::message::WorkUnitOutput* outputMsg, uint64_t trajectoryId, uint numberOfColumns, bool compress, const ValueContainer& valuesRef, const TimeContainer& timesRef) const
//    {
//        timeSeriesWrap.set_arrays_in_output_msg(outputMsg, valuesRef, timesRef, trajectoryId, numberOfColumns, compress);
//    }
//};


#endif /* LM_PROTOWRAP_TIMESERIES_H_ */
