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

void SFileOutputWriter::processMessage(const string& nameString, const string& typeString, const google::protobuf::Message& data)
{
    SFileRecord record(nameString, typeString, data.ByteSize());
    file->writeSFileRecord(record);
    file->writeMessage(data);
}

void SFileOutputWriter::processGenericMessage(const google::protobuf::Message& data)
{
    const google::protobuf::Descriptor* descriptor = data.GetDescriptor();

    stringstream typeSS;
    typeSS << "protobuf:" << descriptor->full_name();

    processMessage(recordNamePrefix, typeSS.str(), data);
}

}
}
}
