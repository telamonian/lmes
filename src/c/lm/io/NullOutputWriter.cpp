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
#include <pthread.h>
#include <sys/time.h>
#include <time.h>

#include "hrtime.h"
#include "lm/Print.h"
#include "lm/io/OutputWriter.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/main/SimulationSupervisor.h"
#include "lm/message/Communicator.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ProcessWorkUnitOutput.pb.h"
#include "lm/message/StartedOutputWriter.pb.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"
#include "lm/Types.h"

#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"


#include <lm/ClassFactory.h>
#include <lm/Print.h>
#include "lm/io/NullOutputWriter.h"
#include "lm/io/OutputWriter.h"


namespace lm {
namespace io {


bool NullOutputWriter::registered=NullOutputWriter::registerClass();

bool NullOutputWriter::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::io::OutputWriter","lm::io::NullOutputWriter",&NullOutputWriter::allocateObject);
    return true;
}

void* NullOutputWriter::allocateObject()
{
    return new NullOutputWriter();
}

NullOutputWriter::NullOutputWriter()
:secondsToDelay(0)
{
}

NullOutputWriter::~NullOutputWriter()
{
}

void NullOutputWriter::processFirstPassageTimes(const lm::io::FirstPassageTimes& data)
{
    if (secondsToDelay > 0)
        sleep(secondsToDelay);
}

void NullOutputWriter::processSpeciesCounts(const lm::io::SpeciesCounts& data)
{
    if (secondsToDelay > 0)
        sleep(secondsToDelay);
}

void NullOutputWriter::processSpeciesTimeSeries(const lm::io::SpeciesTimeSeries& data)
{
    if (secondsToDelay > 0)
        sleep(secondsToDelay);
}

void NullOutputWriter::processLatticeTimeSeries(const lm::io::LatticeTimeSeries& data)
{
    if (secondsToDelay > 0)
        sleep(secondsToDelay);
}

void NullOutputWriter::flush()
{

}

void NullOutputWriter::checkpoint()
{
}

}
}
