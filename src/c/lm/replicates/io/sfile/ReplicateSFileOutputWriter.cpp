/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
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
 * Author(s): Elijah Roberts, Max Klein
 */
#include <google/protobuf/message.h>
#include <iomanip>
#include <sstream>
#include <string>

#include "lm/ClassFactory.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/FFluxOutput.pb.h"
#include "lm/io/OrderParameterFirstPassageTimes.pb.h"
#include "lm/io/OrderParameterTimeSeries.pb.h"
#include "lm/io/OutputWriter.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/io/sfile/LocalSFile.h"
#include "lm/Print.h"
#include "lm/replicates/io/sfile/ReplicateSFileOutputWriter.h"

using std::stringstream;
using std::string;

namespace lm {
namespace replicates {
namespace io {
namespace sfile {


bool ReplicateSFileOutputWriter::registered=ReplicateSFileOutputWriter::registerClass();

bool ReplicateSFileOutputWriter::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::io::OutputWriter","lm::io::sfile::ReplicateSFileOutputWriter",&ReplicateSFileOutputWriter::allocateObject);
    return true;
}

void* ReplicateSFileOutputWriter::allocateObject()
{
    return new ReplicateSFileOutputWriter();
}

void ReplicateSFileOutputWriter::processGenericMessage(const google::protobuf::Message& data)
{
    const google::protobuf::Descriptor* descriptor = data.GetDescriptor();

    stringstream nameSS;

    // add the trajectory id to the record name
    string trajectoryID(getMessageTrajectoryID(data));
    if (not trajectoryID.empty())
        nameSS << "/Simulations/" << trajectoryID;

    // add the message type name to the record name
    nameSS << "/" << descriptor->name();

    stringstream typeSS;
    typeSS << "protobuf:" << descriptor->full_name();

    processMessage(descriptor->name(), typeSS.str(), data);
}

void ReplicateSFileOutputWriter::processDegreeAdvancementTimeSeries(const lm::io::DegreeAdvancementTimeSeries& data)
{
    processMessageReplicate("DegreeAdvancementTimeSeries", "protobuf:lm.io.DegreeAdvancementTimeSeries", data);
}

void ReplicateSFileOutputWriter::processFirstPassageTimes(const lm::io::FirstPassageTimes& data)
{
    processMessageReplicate("FirstPassageTimes", "protobuf:lm.io.FirstPassageTimes", data);
}

void ReplicateSFileOutputWriter::processLatticeTimeSeries(const lm::io::LatticeTimeSeries& data)
{
    processMessageReplicate("LatticeTimeSeries", "protobuf:lm.io.LatticeTimeSeries", data);
}

void ReplicateSFileOutputWriter::processLimitTracking(const lm::io::LimitTracking& data)
{
    processMessageReplicate("LimitTracking", "protobuf:lm.io.LimitTracking", data);
}

void ReplicateSFileOutputWriter::processOrderParameterFirstPassageTimes(const lm::io::OrderParameterFirstPassageTimes& data)
{
    processMessageReplicate("OrderParameterFirstPassageTimes", "protobuf:lm.io.OrderParameterFirstPassageTimes", data);
}

void ReplicateSFileOutputWriter::processOrderParameterTimeSeries(const lm::io::OrderParameterTimeSeries& data)
{
    processMessageReplicate("OrderParameterTimeSeries", "protobuf:lm.io.OrderParameterTimeSeries", data);
}

void ReplicateSFileOutputWriter::processSpeciesCounts(const lm::io::SpeciesCounts& data)
{
    processMessageReplicate("SpeciesCounts", "protobuf:lm.io.SpeciesCounts", data);
}

void ReplicateSFileOutputWriter::processSpeciesTimeSeries(const lm::io::SpeciesTimeSeries& data)
{
    processMessageReplicate("SpeciesTimeSeries", "protobuf:lm.io.SpeciesTimeSeries", data);
}

}
}
}
}
