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
#include "lm/io/DegreeAdvancementTimeSeries.pb.h"
#include "lm/io/OrderParameterTimeSeries.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/protowrap/TimeSeries.h"

namespace lm {
namespace protowrap {

// initialization for static members of TimeSeriesSpecialization

// initialization for Degree Advancment specializations
typedef TimeSeriesSpecialization<lm::io::DegreeAdvancementTimeSeries> DATSS;

const DATSS::getTimeSeriesMsgT DATSS::getTimeSeriesMsgFunc = &DATSS::OutMsgT::mutable_degree_advancement_time_series;
const DATSS::getTimeSeriesMsgConstT DATSS::getTimeSeriesMsgConstFunc = &DATSS::OutMsgT::degree_advancement_time_series;

const DATSS::getValMsgT DATSS::getValMsgFunc = &lm::io::DegreeAdvancementTimeSeries::mutable_counts;
const DATSS::getValMsgConstT DATSS::getValMsgConstFunc = &lm::io::DegreeAdvancementTimeSeries::counts;

const char* DATSS::mismatchErrorString = "Array shape mismatch when serializing a degree advancements time series (check counts.size==times.size*numberOfColumns failed, %d,%d,%d)";

// initialization for Order Parameter specializations
typedef TimeSeriesSpecialization<lm::io::OrderParameterTimeSeries> OPTSS;

const OPTSS::getTimeSeriesMsgT OPTSS::getTimeSeriesMsgFunc = &OPTSS::OutMsgT::mutable_order_parameter_time_series;
const OPTSS::getTimeSeriesMsgConstT OPTSS::getTimeSeriesMsgConstFunc = &OPTSS::OutMsgT::order_parameter_time_series;

const OPTSS::getValMsgT OPTSS::getValMsgFunc = &lm::io::OrderParameterTimeSeries::mutable_values;
const OPTSS::getValMsgConstT OPTSS::getValMsgConstFunc = &lm::io::OrderParameterTimeSeries::values;

const char* OPTSS::mismatchErrorString = "Array shape mismatch when serializing an order parameters time series (check values.size==times.size*numberOfColumns failed, %d,%d,%d)";

// initialization for Species specializations
typedef TimeSeriesSpecialization<lm::io::SpeciesTimeSeries> STSS;

const STSS::getTimeSeriesMsgT STSS::getTimeSeriesMsgFunc = &STSS::OutMsgT::mutable_species_time_series;
const STSS::getTimeSeriesMsgConstT STSS::getTimeSeriesMsgConstFunc = &STSS::OutMsgT::species_time_series;

const STSS::getValMsgT STSS::getValMsgFunc = &lm::io::SpeciesTimeSeries::mutable_counts;
const STSS::getValMsgConstT STSS::getValMsgConstFunc = &lm::io::SpeciesTimeSeries::counts;

const char* STSS::mismatchErrorString = "Array shape mismatch when serializing a species time series (check counts.size==times.size*numberOfColumns failed, %d,%d,%d)";
