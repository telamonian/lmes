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
 * - Neither the names of the Roberts Group, Johns Hopkins University
 * nor the names of its contributors may be used to endorse or promote products
 * derived from this Software without specific prior written permission.
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

#ifndef LM_RESOURCE_RESOURCEMAP_H_
#define LM_RESOURCE_RESOURCEMAP_H_

#include <list>
#include <map>
#include <string>
#include <vector>

#include "lm/message/ResourcesAvailable.pb.h"

using std::list;
using std::map;
using std::string;
using std::vector;

namespace lm {
namespace resource {

class ResourceMap
{
public:
    class ComputeResources
    {
    public:
        ComputeResources():hostname(""),controller_process(-1),controller_thread(-1) {}
    	string hostname;
        int controller_process;
        int controller_thread;
        vector<int> cpuCores;
        vector<int> gpuDevices;
    };

public:
    ResourceMap(list<string>hostnames, int defaultCPUCores, int defaultGPUDevices, string resourceFilename);
    virtual ~ResourceMap();
    bool registerResources(const lm::message::ResourcesAvailable& msg);
    ComputeResources reserveCPUCores(int process, int numberCPUCores);
    map<int,ComputeResources> getAvailableResources();

protected:
    map<string,ComputeResources> parseResourceFile(string filename);
    map<string,ComputeResources> parsePBSNodeFile(string filename);

protected:
    int defaultCPUCores;
    int defaultGPUDevices;
    map<string,int> hostnameProcessMap;
    map<int,ComputeResources> allocatedResources;
    map<int,ComputeResources> registeredResources;

private:
    void parseIntList(vector<int>& list, string s);
};

}
}

#endif
