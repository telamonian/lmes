/*/*
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

#ifndef LM_MAIN_RESOURCECONTROLLER
#define LM_MAIN_RESOURCECONTROLLER

#include <pthread.h>
#include <list>
#include <map>
#include <string>
#include <vector>
#include "lm/Print.h"
#include "lm/main/WorkUnitRunner.h"
#include "lm/message/Communicator.h"
#include "lm/message/StartOutputWriter.pb.h"
#include "lm/message/StartWorkUnitRunner.pb.h"
#include "lm/message/StartedWorkUnitRunner.pb.h"
#include "lm/thread/Worker.h"
#include "lm/thread/Thread.h"

namespace lm {
namespace main {

class ResourceController : public lm::thread::Worker
{
public:
    static std::vector<int> getPhysicalCPUCores();
    static std::vector<int> getPhysicalGPUs();

public:
    ResourceController();
    virtual ~ResourceController();
    virtual void wake() throw(PthreadException);

protected:
    virtual int run();
    virtual void startWorkUnitRunner(const lm::message::StartWorkUnitRunner& msg);
    virtual void startOutputWriter(const lm::message::StartOutputWriter& msg);
    virtual void stopWorkers(bool abort);

protected:
    lm::message::Communicator communicator;
    std::list<lm::thread::Worker*> runnerWorkers;
    std::list<lm::thread::Worker*> writerWorkers;
};

}
}

#endif
