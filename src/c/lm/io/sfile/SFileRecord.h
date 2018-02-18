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

#ifndef LM_IO_SFILE_SFILERECORD_H
#define LM_IO_SFILE_SFILERECORD_H

#include <string>
#include "lm/Types.h"

namespace lm {
namespace io {
namespace sfile {

class SFileRecord
{
public:
    static const char RECORD_SEPARATOR[16];
    static const int NAME_BUFFER_MAX_SIZE=256;

public:
    SFileRecord();
    SFileRecord(const std::string& name, const std::string& type, int64_t dataSize);

    template <typename T>
    inline void setName(const T& newName)
    {
        name.assign(newName);
    }

    template <typename T>
    void setName(const char* fmt, const T arg0)
    {
        char buffer[NAME_BUFFER_MAX_SIZE+1];
        memset(buffer, 0, NAME_BUFFER_MAX_SIZE+1);
        snprintf(buffer, NAME_BUFFER_MAX_SIZE, fmt, arg0);

        name.assign(buffer);
    }

    template <typename T, typename U>
    void setName(const char* fmt, const T arg0, const U arg1)
    {
        char buffer[NAME_BUFFER_MAX_SIZE+1];
        memset(buffer, 0, NAME_BUFFER_MAX_SIZE+1);
        snprintf(buffer, NAME_BUFFER_MAX_SIZE, fmt, arg0, arg1);

        name.assign(buffer);
    }

    template <typename T, typename U, typename V>
    void setName(const char* fmt, const T arg0, const U arg1, const V arg2)
    {
        char buffer[NAME_BUFFER_MAX_SIZE+1];
        memset(buffer, 0, NAME_BUFFER_MAX_SIZE+1);
        snprintf(buffer, NAME_BUFFER_MAX_SIZE, fmt, arg0, arg1, arg2);

        name.assign(buffer);
    }

    std::string name;
    std::string type;
    int64_t dataSize;

protected:
    inline void normalizeName();
};

}
}
}

#endif
