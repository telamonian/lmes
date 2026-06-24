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
 * Author(s): Elijah Roberts
 */

#include <cstdlib>
#include <string>

#include "lm/Exceptions.h"
#include "lm/io/sfile/SFile.h"
#include "lm/io/sfile/SFileRecord.h"

using std::string;

namespace lm {
namespace io {
namespace sfile {

SFile::SFile()
{
}

SFile::~SFile()
{
}

void SFile::readFully(void* vbuffer, size_t length)
{
    unsigned char* buffer = (unsigned char*)vbuffer;
    size_t bytesRead;
    do
    {
        bytesRead=read(buffer, length);
        buffer += bytesRead;
        length -= bytesRead;
    }
    while (length > 0 && bytesRead != 0);
}

bool SFile::isSFile()
{
    bool ret=false;
    unsigned char header[sizeof(SFileRecord::RECORD_SEPARATOR)];
    openRead();
    size_t bytesRead=read(header, sizeof(SFileRecord::RECORD_SEPARATOR));
    if (bytesRead == sizeof(SFileRecord::RECORD_SEPARATOR) && memcmp(header, SFileRecord::RECORD_SEPARATOR, sizeof(SFileRecord::RECORD_SEPARATOR)) == 0)
        ret=true;
    close();
    return ret;
}

SFileRecord SFile::readNextSFileRecord()
{
    // Make sure that a separator is next.
    unsigned char separator[sizeof(SFileRecord::RECORD_SEPARATOR)];
    readFully(separator, sizeof(SFileRecord::RECORD_SEPARATOR));
    if (memcmp(separator, SFileRecord::RECORD_SEPARATOR, sizeof(SFileRecord::RECORD_SEPARATOR)) != 0)
    {
        throw lm::IOException("Error reading next sfile separator.");
    }

    // Read the name.
    uint32_t nameSize;
    readFully(&nameSize, sizeof(nameSize));
    char* name = new char[nameSize+1];
    readFully(name, nameSize);
    name[nameSize] = 0;

    // Read the type.
    uint32_t typeSize;
    readFully(&typeSize, sizeof(typeSize));
    char* type = new char[typeSize+1];
    readFully(type, typeSize);
    type[typeSize] = 0;

    // Read the data size
    uint64_t dataSize;
    readFully(&dataSize, sizeof(dataSize));

    return SFileRecord(string(name), string(type), dataSize);
}

void SFile::writeSFileRecord(SFileRecord record)
{
    write((void*)SFileRecord::RECORD_SEPARATOR, sizeof(SFileRecord::RECORD_SEPARATOR));
    uint32_t nameLength = record.name.size();
    write(&nameLength, sizeof(nameLength));
    write((void*)record.name.c_str(), nameLength);
    uint32_t typeLength = record.type.size();
    write(&typeLength, sizeof(typeLength));
    write((void*)record.type.c_str(), typeLength);
    write(&record.dataSize, sizeof(record.dataSize));
}

}
}
}
