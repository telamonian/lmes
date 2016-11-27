/*
 * University of Illinois Open Source License
 * Copyright 2012 Roberts Group,
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

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <list>
#include <string>
#include <sys/stat.h>
#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/message/ResourcesAvailable.pb.h"
#include "lm/resource/ResourceMap.h"

using std::ifstream;

namespace lm {
namespace resource {

ResourceMap::ResourceMap()
:defaultCPUCores(-1),defaultGPUDevices(-1)
{
}

ResourceMap::ResourceMap(list<string>hostnames, int defaultCPUCores, int defaultGPUDevices)
:defaultCPUCores(defaultCPUCores),defaultGPUDevices(defaultGPUDevices)
{
    // Create the initial allocation map from the hostnames.
    for (list<string>::iterator it=hostnames.begin(); it != hostnames.end(); it++)
    {
        // Create an entry in the allocation map.
        ComputeResources resources;
        resources.hostname = *it;
        resources.useDefaultResources = true;
        allocatedResources[*it] = resources;
    }
}

ResourceMap::ResourceMap(string resourceFilename, int defaultCPUCores, int defaultGPUDevices)
:defaultCPUCores(defaultCPUCores),defaultGPUDevices(defaultGPUDevices)
{
    // If we can find the resource file, parse it.
    struct stat fileStats;
    if (resourceFilename != "" && stat(resourceFilename.c_str(), &fileStats) == 0 && S_ISREG(fileStats.st_mode))
    {
        // Parse the resource file.
        allocatedResources = parseResourceFile(resourceFilename);
        Print::printf(Print::INFO, "Read resource allocations from file %s: %d hosts.", resourceFilename.c_str(), allocatedResources.size());
    }
}

ResourceMap::ResourceMap(QueueingSystem queueingSystem, int defaultCPUCores, int defaultGPUDevices)
:defaultCPUCores(defaultCPUCores),defaultGPUDevices(defaultGPUDevices)
{
    // See what queueing system we are using.
    if (queueingSystem == PBS)
    {
        // Try to find a PBS nodefile.
        char * pbsNodeFile = getenv("PBS_NODEFILE");
        struct stat fileStats;
        if (pbsNodeFile != NULL && stat(pbsNodeFile, &fileStats) == 0 && S_ISREG(fileStats.st_mode))
        {
            // Parse the pbs node file.
            allocatedResources = parsePBSNodeFile(pbsNodeFile);
            Print::printf(Print::INFO, "Read resource allocations from PBS node file %s: %d hosts.", pbsNodeFile, allocatedResources.size());
        }
    }
}

ResourceMap::~ResourceMap()
{
}

/**
 * @brief ResourceMap::parsePBSNodeFile
 * @param filename
 * @return
 */
map<string,ComputeResources> ResourceMap::parseResourceFile(string filename)
{
      map<string,ComputeResources> fileResources;

      ifstream file(filename.c_str());
      string line;

      while (std::getline(file, line))
      {
          if (line.length() > 0)
          {
              string hostname = "";
              string cpuCores = "";
              string gpuDevices = "";
              size_t i = line.find(' ');
              if (i != string::npos)
              {
                  hostname = line.substr(0, i);
                  line = line.substr(i+1, string::npos);
                  i = line.find(' ');
                  if (i != string::npos)
                  {
                      cpuCores = line.substr(0, i);
                      gpuDevices = line.substr(i+1, string::npos);
                  }
                  else
                  {
                      cpuCores = line;
                  }
              }
              else
              {
                  hostname = line;
              }

              // See if this is the first entry for the host.
              if (!fileResources.count(hostname))
              {
                  ComputeResources resources;
                  resources.hostname = hostname;
                  if (cpuCores.length() > 0)
                      parseIntList(resources.cpuCores, cpuCores);
                  else
                      resources.cpuCores.push_back(resources.cpuCores.size());

                  if (gpuDevices.length() > 0)
                      parseIntList(resources.gpuDevices, gpuDevices);

                  fileResources[hostname] = resources;
              }
              else
              {
                  if (cpuCores.length() > 0)
                      parseIntList(fileResources[hostname].cpuCores, cpuCores);
                  else
                      fileResources[hostname].cpuCores.push_back(fileResources[hostname].cpuCores.size());

                  if (gpuDevices.length() > 0)
                      parseIntList(fileResources[hostname].gpuDevices, gpuDevices);
              }
          }
      }

      return fileResources;
}


/**
 * @brief ResourceMap::parsePBSNodeFile
 * @param filename
 * @return
 *
 * PBS node files have a single field per line with the name of a host allocated. Hosts can
 * be listed multiple times in which case one core for each entry should be assigned.
 */
map<string,ComputeResources> ResourceMap::parsePBSNodeFile(string filename)
{
      map<string,ComputeResources> fileResources;

	  ifstream file(filename.c_str());
      string hostname;

      while (std::getline(file, hostname))
	  {
          if (hostname != "")
          {
              // See if this is the first entry for the host.
              if (!fileResources.count(hostname))
              {
                  ComputeResources resources;
                  resources.hostname = hostname;
                  resources.cpuCores.push_back(0);
                  fileResources[hostname] = resources;
              }
              else
              {
                  // Add the next cpu core to the list.
                  fileResources[hostname].cpuCores.push_back(fileResources[hostname].cpuCores.size());
              }
          }
	  }

      return fileResources;
}

/**
 * @brief ResourceMap::registerResources
 * @param msg
 * @return True if all allocated resource have been registered, otherwise false.
 */
bool ResourceMap::registerResources(const lm::message::ResourcesAvailable& msg)
{
    // See if we can find the process in the map of allocated resources.
    if (allocatedResources.count(msg.hostname()))
    {
        // Make sure the hostname matches.
        ComputeResources resources = allocatedResources[msg.hostname()];

        // Set the controller thread.
        resources.controllerAddress = msg.controller_address();

        // If there were no resources explicitly assigned, use the default cpu cores and gpu devices.
        if (resources.useDefaultResources)
        {
            // Add the cpu cores.
            int numCPUCores=msg.cpu_cores_size();
            if (defaultCPUCores >= 0 && defaultCPUCores < numCPUCores)
            {
                numCPUCores = defaultCPUCores;
            }
            for (int i=0; i<numCPUCores; i++)
            {
                resources.cpuCores.push_back(msg.cpu_cores(i));
            }

            // Add the gpu devices.
            int numGPUDevices=msg.gpu_devices_size();
            if (defaultGPUDevices >= 0 && defaultGPUDevices < numGPUDevices)
            {
                numGPUDevices = defaultGPUDevices;
            }
            for (int i=0; i<numGPUDevices; i++)
            {
                resources.gpuDevices.push_back(msg.gpu_devices(i));
            }
        }

        // Move the resources from the allocated list to the registered list.
        registeredResources[resources.hostname] = resources;
        allocatedResources.erase(resources.hostname);

        Print::printf(Print::INFO, "Registered resources for host %s (defaults=%d): %d cpu cores, %d gpu devices", resources.hostname.c_str(), resources.useDefaultResources, resources.cpuCores.size(), resources.gpuDevices.size());
    }
    else
    {
        throw Exception("a host reporting resources available was not in the allocation list", msg.hostname().c_str());
    }
    return (allocatedResources.size() == 0);
}

ComputeResources ResourceMap::reserveCPUCores(string hostname, int numberCPUCores)
{
    ComputeResources resources = registeredResources[hostname];
    if (resources.cpuCores.size() >= numberCPUCores)
    {
        ComputeResources reservedResources;
        reservedResources.hostname = resources.hostname;
        reservedResources.controllerAddress = resources.controllerAddress;
        for (int i=0; i<numberCPUCores; i++)
        {
            reservedResources.cpuCores.push_back(resources.cpuCores[0]);
            resources.cpuCores.erase(resources.cpuCores.begin());
        }
        registeredResources[hostname] = resources;
        return reservedResources;
    }
    throw lm::Exception("Insufficient resource on specified host to reserve a CPU core", hostname.c_str(), resources.cpuCores.size(), numberCPUCores);
}

map<string,ComputeResources> ResourceMap::getAvailableResources()
{
    return registeredResources;
}

void ResourceMap::parseIntList(vector<int>& list, string s)
{
    char * argbuf = new char[s.length()+1];
    strcpy(argbuf,s.c_str());
    char * pch = strtok(argbuf,",;:");
    while (pch != NULL)
    {
        char * rangeDelimiter;
        if ((rangeDelimiter=strstr(pch,"-")) != NULL)
        {
            *rangeDelimiter='\0';
            int begin=atoi(pch);
            int end=atoi(rangeDelimiter+1);
            for (int i=begin; i<=end; i++)
                list.push_back(i);
        }
        else
        {
            if (strlen(pch) > 0) list.push_back(atoi(pch));
        }
        pch = strtok(NULL," ,;:");
    }
    delete[] argbuf;
}

}
}
