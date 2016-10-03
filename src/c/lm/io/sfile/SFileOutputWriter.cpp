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
#include <sys/stat.h>

#include "lm/ClassFactory.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/FFluxOutput.pb.h"
#include "lm/io/OrderParameterFirstPassageTimes.pb.h"
#include "lm/io/OrderParameterTimeSeries.pb.h"
#include "lm/io/OutputWriter.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/io/sfile/LocalSFile.h"
#include "lm/io/sfile/SFileOutputWriter.h"
#include "lm/io/sfile/SFile.h"
#include "lm/main/Globals.h"
#include "lm/Print.h"

using std::stringstream;
using std::string;

namespace lm {
namespace io {
namespace sfile {


bool SFileOutputWriter::registered=SFileOutputWriter::registerClass();

bool SFileOutputWriter::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::io::OutputWriter","lm::io::sfile::SFileOutputWriter",&SFileOutputWriter::allocateObject);
    return true;
}

void* SFileOutputWriter::allocateObject()
{
    return new SFileOutputWriter();
}

SFileOutputWriter::SFileOutputWriter(): file(NULL) {}

SFileOutputWriter::~SFileOutputWriter()
{
    if (file != NULL) delete file; file = NULL;
}

void SFileOutputWriter::initialize()
{
    OutputWriter::initialize();

    // Make sure we have an output filename.
    if (outputFilename == "") throw Exception("Invalid output filename",outputFilename.c_str());

    // Open the file.
    file = new LocalSFile(outputFilename);
    file->openAppend();
}

void SFileOutputWriter::finalize()
{
    OutputWriter::finalize();

    file->close();
    delete file;
    file = NULL;
}

void SFileOutputWriter::checkpoint()
{
}

void SFileOutputWriter::flush()
{
    file->flush();
}

void SFileOutputWriter::processDegreeAdvancementTimeSeries(const lm::io::DegreeAdvancementTimeSeries& data)
{
    stringstream nameSS;
    nameSS << "/Simulations" << "/" << data.trajectory_id() << "/DegreeAdvancementTimeSeries";

    processMessage(nameSS.str(), "protobuf:lm.io.DegreeAdvancementTimeSeries", data);
}

void SFileOutputWriter::processFFluxOutput(const lm::io::FFluxOutput& data)
{
    processMessage("/FFluxOutput", "protobuf:lm.io.FFluxOutput", data);
}

void SFileOutputWriter::processFirstPassageTimes(const lm::io::FirstPassageTimes& data)
{
    char buffer[RECORD_NAME_BUFFER_MAX_SIZE+1];
    memset(buffer, 0, RECORD_NAME_BUFFER_MAX_SIZE+1);
    snprintf(buffer,RECORD_NAME_BUFFER_MAX_SIZE,"%s/Simulations/%llu/FirstPassageTimes/%d", recordNamePrefix.c_str(), data.trajectory_id(), data.species());
    SFileRecord record(string(buffer), string("protobuf:lm.io.FirstPassageTimes"), data.ByteSize());
    file->writeSFileRecord(record);
    file->writeMessage(data);
}

void SFileOutputWriter::processLatticeTimeSeries(const lm::io::LatticeTimeSeries& data)
{
    char buffer[RECORD_NAME_BUFFER_MAX_SIZE+1];
    memset(buffer, 0, RECORD_NAME_BUFFER_MAX_SIZE+1);
    snprintf(buffer,RECORD_NAME_BUFFER_MAX_SIZE,"%s/Simulations/%llu/LatticeTimeSeries", recordNamePrefix.c_str(), data.trajectory_id());
    SFileRecord record(string(buffer), string("protobuf:lm.io.LatticeTimeSeries"), data.ByteSize());
    file->writeSFileRecord(record);
    file->writeMessage(data);
}

void SFileOutputWriter::processLimitTracking(const lm::io::LimitTracking& data)
{
    stringstream nameSS;
    nameSS << "/Simulations/" << data.trajectory_id();
    nameSS << "/Limit/" << std::setfill('0') << std::setw(2) << data.limit_id();
    nameSS << "/LimitTracking";

    processMessage(nameSS.str(), "protobuf:lm.io.LimitTracking", data);
}

void SFileOutputWriter::processOrderParameterFirstPassageTimes(const lm::io::OrderParameterFirstPassageTimes& data)
{
    stringstream nameSS;
    nameSS << "/Simulations/" << data.trajectory_id();
    nameSS << "/OrderParameter/" << std::setfill('0') << std::setw(2) << data.order_parameter_id();
    nameSS << "/OrderParameterFirstPassageTimes/";

    processMessage(nameSS.str(), "protobuf:lm.io.OrderParameterFirstPassageTimes", data);
}

void SFileOutputWriter::processOrderParameterTimeSeries(const lm::io::OrderParameterTimeSeries& data)
{
    char buffer[RECORD_NAME_BUFFER_MAX_SIZE+1];
    memset(buffer, 0, RECORD_NAME_BUFFER_MAX_SIZE+1);
    snprintf(buffer,RECORD_NAME_BUFFER_MAX_SIZE,"%s/Simulations/%llu/OrderParameterTimeSeries", recordNamePrefix.c_str(), data.trajectory_id());
    SFileRecord record(string(buffer), string("protobuf:lm.io.OrderParameterTimeSeries"), data.ByteSize());
    file->writeSFileRecord(record);
    file->writeMessage(data);
}

void SFileOutputWriter::processSpeciesCounts(const lm::io::SpeciesCounts& data)
{
    char buffer[RECORD_NAME_BUFFER_MAX_SIZE+1];
    memset(buffer, 0, RECORD_NAME_BUFFER_MAX_SIZE+1);
    snprintf(buffer,RECORD_NAME_BUFFER_MAX_SIZE,"%s/Simulations/%llu/SpeciesCounts", recordNamePrefix.c_str(), data.trajectory_id());
    SFileRecord record(string(buffer), string("protobuf:lm.io.SpeciesCounts"), data.ByteSize());
    file->writeSFileRecord(record);
    file->writeMessage(data);
}

void SFileOutputWriter::processSpeciesTimeSeries(const lm::io::SpeciesTimeSeries& data)
{
    char buffer[RECORD_NAME_BUFFER_MAX_SIZE+1];
    memset(buffer, 0, RECORD_NAME_BUFFER_MAX_SIZE+1);
    snprintf(buffer,RECORD_NAME_BUFFER_MAX_SIZE,"%s/Simulations/%llu/SpeciesTimeSeries", recordNamePrefix.c_str(), data.trajectory_id());
    SFileRecord record(string(buffer), string("protobuf:lm.io.SpeciesTimeSeries"), data.ByteSize());
    file->writeSFileRecord(record);
    file->writeMessage(data);
}

void SFileOutputWriter::processGenericMessage(const google::protobuf::Message& data)
{
    const google::protobuf::Reflection* reflection = data.GetReflection();
    const google::protobuf::Descriptor* descriptor = data.GetDescriptor();

    stringstream nameSS;
    // if your message has a trajectory_id, file it away under "Simulations"
    const google::protobuf::FieldDescriptor* trajIDDescriptor = descriptor->FindFieldByName("trajectory_id");
    if (trajIDDescriptor!=NULL and (trajIDDescriptor->label()!=google::protobuf::FieldDescriptor::LABEL_OPTIONAL or reflection->HasField(data, trajIDDescriptor)))
    {
        // this will cause a runtime error if your trajectory_id field is not of type uint64. Alternatively, you could check, ie if (trajIDDescriptor->type()==google::protobuf::FieldDescriptor::TYPE_UINT64)
        nameSS << "/Simulations" << "/" << reflection->GetUInt64(data, trajIDDescriptor);
    }
    nameSS << "/" << descriptor->name();

    stringstream typeSS;
    typeSS << "protobuf:" << descriptor->full_name();

    processMessage(nameSS.str(), typeSS.str(), data);
}

void SFileOutputWriter::processMessage(const string& nameString, const string& typeString, const google::protobuf::Message& data)
{
    // copy namestring from the const ref to a new mutable string
    string prefixedNameString(nameString);
    prefixedNameString.insert(0, recordNamePrefix);
    if (prefixedNameString.size() > RECORD_NAME_BUFFER_MAX_SIZE) prefixedNameString.resize(RECORD_NAME_BUFFER_MAX_SIZE);

    SFileRecord record(prefixedNameString, typeString, data.ByteSize());
    file->writeSFileRecord(record);
    file->writeMessage(data);
}

}
}
}
