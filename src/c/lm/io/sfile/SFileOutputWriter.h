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

#ifndef LM_IO_SFILEOutputWriter
#define LM_IO_SFILEOutputWriter

#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/OutputWriter.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/io/sfile/SFile.h"

namespace lm {
namespace io {
namespace sfile {

class SFileOutputWriter : public OutputWriter
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

public:
    SFileOutputWriter();
    virtual ~SFileOutputWriter();
    virtual void initialize();
    virtual void finalize();

protected:
    virtual void processFirstPassageTimes(const lm::io::FirstPassageTimes& data);
    virtual void processSpeciesCounts(const lm::io::SpeciesCounts& data);
    virtual void processSpeciesTimeSeries(const lm::io::SpeciesTimeSeries& data);
    virtual void processLatticeTimeSeries(const lm::io::LatticeTimeSeries& data);
    virtual void flush();
    virtual void checkpoint();

private:
    SFile* file;
};

}
}
}


#endif
