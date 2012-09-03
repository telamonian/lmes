/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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

#ifndef LM_MAIN_RESOURCEALLOCATOR_H_
#define LM_MAIN_RESOURCEALLOCATOR_H_

#include <string>
#include <vector>
#include "lm/thread/Thread.h"

using std::string;
using std::vector;
using lm::thread::PthreadException;

namespace lm {
namespace main {

class ResourceAllocator
{
public:
    class ComputeResources
    {
    public:
    	int processNumber;
        vector<int> cpuCores;
        vector<int> cudaDevices;
        string toString();
    };

public:
    ResourceAllocator(int processNumber, int numberCpuCores, float cpuCoresPerReplicate) throw(Exception,PthreadException);
    ResourceAllocator(int processNumber, int numberCpuCores, float cpuCoresPerReplicate, vector<int> cudaDevices, float cudaDevicesPerReplicate) throw(Exception,PthreadException);
    virtual ~ResourceAllocator() throw(PthreadException);

    virtual int getMaxSimultaneousReplicates();
    virtual ComputeResources assignReplicate(int replicate) throw(Exception,PthreadException);
    virtual void removeReplicate(int replicate) throw(Exception,PthreadException);
    virtual int reserveCpuCore() throw(Exception,PthreadException);

private:
    void initialize(float cpuCoresPerReplicate, float cudaDevicesPerReplicate) throw(Exception,PthreadException);

protected:
    pthread_mutex_t mutex;
    int processNumber;
    int numberCpuCores;
    int reservedCpuCores;
    vector<int> cudaDevices;
    int cpuSlotsPerCore;
    int cpuSlotsPerReplicate;
    int cudaSlotsPerDevice;
    int cudaSlotsPerReplicate;

    int ** cpuSlots;
    int ** cudaSlots;
};

}
}

#endif
