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

#ifndef LM_IO_SFILE_LOCALSFFILE_H
#define LM_IO_SFILE_LOCALSFFILE_H

#include <cstdio>
#include <string>
#include <google/protobuf/message.h>
#include <sys/stat.h>

#include "lm/io/sfile/SFile.h"

using std::string;

namespace lm {
namespace io {
namespace sfile {

class LocalSFile : public SFile
{
public:
    LocalSFile(const string filename);
    virtual ~LocalSFile();
    virtual bool isFile();
    virtual bool isDir();
    virtual bool exists();
    virtual string getFilename();
    virtual int64_t getSize();
    virtual void openTruncate();
    virtual void openAppend();
    virtual void openRead();
    virtual size_t read(void* buffer, size_t length);
    virtual void skip(uint64_t length);
    virtual bool isEof();
    virtual size_t write(void* buffer, size_t length);
    virtual void writeMessage(const google::protobuf::Message& message);
    virtual void flush();
    virtual void close();

protected:
    string filename;
    struct stat filestats;
    int filestatsRet;
    FILE* fp;
};

}
}
}

#endif
