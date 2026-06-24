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

#include <cstdio>
#include <stdexcept>
#include <string>
#include <google/protobuf/message.h>
#include <sys/stat.h>

#include "lm/Exceptions.h"
#include "lm/io/sfile/LocalSFile.h"

using std::string;

namespace lm {
namespace io {
namespace sfile {

LocalSFile::LocalSFile(const string filename)
:filename(filename),fp(NULL)
{
    filestatsRet=stat(filename.c_str(), &filestats);
}

LocalSFile::~LocalSFile()
{
    close();
}

bool LocalSFile::isFile()
{
    return S_ISREG(filestats.st_mode);

}

bool LocalSFile::isDir()
{
    return S_ISDIR(filestats.st_mode);
}

bool LocalSFile::exists()
{
    return (filestatsRet==0);
}

string LocalSFile::getFilename()
{
    return filename;
}

int64_t LocalSFile::getSize()
{
    return (int64_t)filestats.st_size;
}

void LocalSFile::openTruncate()
{
    if (fp == NULL)
    {
        fp = fopen(filename.c_str(), "w");
        if (fp == NULL) throw lm::IOException(string("Could not open file for writing truncate: ")+filename);
    }
}

void LocalSFile::openAppend()
{
    if (fp == NULL)
    {
        fp = fopen(filename.c_str(), "a");
        if (fp == NULL) throw lm::IOException(string("Could not open file for writing append: ")+filename);
    }
}

void LocalSFile::openRead()
{
    if (fp == NULL)
    {
        fp = fopen(filename.c_str(), "r");
        if (fp == NULL) throw lm::IOException(string("Could not open file for reading: ")+filename);
    }
}

size_t LocalSFile::read(void* buffer, size_t count)
{
    if (fp != NULL)
    {
        size_t ret=fread(buffer, sizeof(unsigned char), count, fp);
        if (count != ret && ferror(fp) != 0) throw lm::IOException(string("Error reading from file: ")+filename);
        return ret;
    }
    return 0;
}

void LocalSFile::skip(uint64_t length)
{
    if (fp != NULL)
    {
        if (fseek(fp, length, SEEK_CUR) != 0) throw lm::IOException(string("Error seeking in file: ")+filename);
    }
}

bool LocalSFile::isEof()
{
    if (fp != NULL)
    {
        return (ftell(fp) == getSize());
    }
    return true;
}

size_t LocalSFile::write(void* buffer, size_t count)
{
    if (fp != NULL)
    {
        size_t ret=fwrite(buffer, sizeof(unsigned char), count, fp);
        if (count != ret) throw lm::IOException(string("Error writing to file: ")+filename);
        return ret;
    }
    return 0;
}

void LocalSFile::writeMessage(const google::protobuf::Message& message)
{
    if (fp != NULL)
    {
        fflush(fp);
        message.SerializeToFileDescriptor(fileno(fp));
    }
}

void LocalSFile::flush()
{
    if (fp != NULL)
    {
        int ret=fflush(fp);
        if (ret != 0) throw lm::IOException(string("Could not flush file: ")+filename);
    }
}

void LocalSFile::close()
{
    if (fp != NULL)
    {
        int ret=fclose(fp);
        fp = NULL;
        if (ret != 0) throw lm::IOException(string("Could not close file: ")+filename);
    }
}

}
}
}
