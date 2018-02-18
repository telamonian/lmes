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

#include "lm/io/sfile/SFileRecord.h"

using std::string;

namespace lm {
namespace io {
namespace sfile {

// Need to assign integer literals to an array of unsigned char before converting to an array of char due to c++11 disallowing 'narrowing' conversions (i.e. cannot convert 243 to signed char)
const char SFileRecord::RECORD_SEPARATOR[] = {'S','F','R','X',1,65,static_cast<char>(243),72,36,static_cast<char>(217),55,18,static_cast<char>(134),11,static_cast<char>(234),83};

SFileRecord::SFileRecord()
:name(),type(),dataSize(0)
{
}

SFileRecord::SFileRecord(const string& name, const string& type, int64_t dataSize)
:name(name),type(type),dataSize(dataSize)
{
    normalizeName();
}

void SFileRecord::normalizeName()
{
    if (name.size() > NAME_BUFFER_MAX_SIZE) name.resize(NAME_BUFFER_MAX_SIZE);
}

}
}
}
