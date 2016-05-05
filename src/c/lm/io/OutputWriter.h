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

#ifndef LM_IO_OUTPUTWRITER
#define LM_IO_OUTPUTWRITER

#include <queue>
#include <string>

#include <pthread.h>

#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/LatticeTimeSeries.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/message/Communicator.h"
#include "lm/message/Message.pb.h"
#include "lm/message/ProcessWorkUnitOutput.pb.h"
#include "lm/thread/Thread.h"
#include "lm/thread/Worker.h"

using std::string;

namespace lm {
namespace io {

class OutputWriter : public lm::thread::Worker
{
public:
    OutputWriter();
    virtual ~OutputWriter();
    void setOutputFilename(string outputFilename) {this->outputFilename = outputFilename;}
    virtual void initialize();
    virtual void finalize();

    virtual void wake() throw(lm::thread::PthreadException);

protected:
    virtual void checkpoint()=0;
    virtual void flush()=0;

    virtual void processFFluxOutput(const lm::io::FFluxOutput& data) {}
    virtual void processFirstPassageTimes(const lm::io::FirstPassageTimes& data)=0;
    virtual void processLatticeTimeSeries(const lm::io::LatticeTimeSeries& data)=0;
    virtual void processOrderParameterFirstPassageTimes(const lm::io::OrderParameterFirstPassageTimes& data) {}
    virtual void processOrderParameterTimeSeries(const lm::io::OrderParameterTimeSeries& data) {}
    virtual void processSpeciesCounts(const lm::io::SpeciesCounts& data)=0;
    virtual void processSpeciesTimeSeries(const lm::io::SpeciesTimeSeries& data)=0;

    virtual int run();

private:
    static const int MESSAGE_QUEUE_MAX_SIZE=50*1024*1024;

protected:
    string outputFilename;

private:
    lm::message::Communicator communicator;
    std::queue<lm::message::Message*> messageQueue;
    volatile int messageQueueSize;
    pthread_mutex_t messageQueueMutex;
    pthread_cond_t messageQueueSignal;

private:
    class HelperThread : public lm::thread::Thread
    {
    public:
        HelperThread(OutputWriter* p);
        virtual ~HelperThread();
        virtual void wake() throw(lm::thread::PthreadException);
    protected:
        virtual int run();
    private:
        OutputWriter* p;
    };
};

}
}


#endif
