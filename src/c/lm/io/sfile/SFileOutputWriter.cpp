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
#include <sstream>
#include <string>
#include <sys/stat.h>

#include <lm/ClassFactory.h>
#include <lm/main/Globals.h>
#include <lm/Print.h>
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/FFluxOutput.pb.h"
#include "lm/io/OrderParameterFirstPassageTimes.pb.h"
#include "lm/io/OrderParameterTimeSeries.pb.h"
#include "lm/io/OutputWriter.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/io/sfile/LocalSFile.h"
#include "lm/io/sfile/SFileOutputWriter.h"
#include "lm/io/sfile/SFile.h"

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
    lm::Print::printf(Print::INFO, "Using sfile record prefix: %s",sfileRecordNamePrefix.c_str());
    return new SFileOutputWriter(sfileRecordNamePrefix);
}

SFileOutputWriter::SFileOutputWriter(string recordNamePrefix)
:recordNamePrefix(recordNamePrefix),file(NULL)
{
}

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

void SFileOutputWriter::processMessage(const google::protobuf::Message& data, std::string& nameString, std::string& typeString)
{
    SFileRecord record(nameString, typeString, data.ByteSize());
    file->writeSFileRecord(record);
    file->writeMessage(data);
}

void SFileOutputWriter::processFFluxOutput(const lm::io::FFluxOutput& data)
{
    stringstream ss;
    ss << "/FFluxOutput";
    string nameString(ss.str()), typeString("protobuf:lm.io.FFluxOutput");
    processMessage(data, nameString, typeString);
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

void SFileOutputWriter::processOrderParameterFirstPassageTimes(const lm::io::OrderParameterFirstPassageTimes& data)
{
    stringstream ss;
    ss << "/Simulations/" << data.trajectory_id() << "/OrderParameterFirstPassageTimes";
    string nameString(ss.str()), typeString("protobuf:lm.io.OrderParameterFirstPassageTimes");
    processMessage(data, nameString, typeString);
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

void SFileOutputWriter::flush()
{
    file->flush();
}

void SFileOutputWriter::checkpoint()
{
}

void SFileOutputWriter::finalize()
{
    OutputWriter::finalize();

    file->close();
    delete file;
    file = NULL;
}

}
}
}
