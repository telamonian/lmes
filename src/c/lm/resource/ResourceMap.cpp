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

ResourceMap::ResourceMap(list<string>hostnames, int defaultCPUCores, int defaultGPUDevices, string resourceFilename)
    :defaultCPUCores(defaultCPUCores),defaultGPUDevices(defaultGPUDevices)
{
    // Create the initial allocation map from the hostnames.
    int i=0;
    for (list<string>::iterator it=hostnames.begin(); it != hostnames.end(); it++, i++)
    {
        // Add the hostname and process to the map.
        if (!hostnameProcessMap.count(*it)) hostnameProcessMap[*it] = i;

        // Create an entry in the allocation map.
        ComputeResources resources;
        resources.hostname = *it;
        resources.controller_process = i;
        allocatedResources[i] = resources;
    }

    // If we can find the resource file, parse it.
    map<string,ComputeResources> fileResources;
    struct stat fileStats;
	char * pbsNodeFile = getenv("PBS_NODEFILE");
    if (resourceFilename != "" && stat(resourceFilename.c_str(), &fileStats) == 0 && S_ISREG(fileStats.st_mode))
    {
        // Parse the resource file.
        fileResources = parseResourceFile(resourceFilename);
        Print::printf(Print::INFO, "Read resource allocations from file %s: %d hosts.", resourceFilename.c_str(), fileResources.size());

    }

    // If we didn't get a resource file, try to find a PBS nodefile.
    else if (pbsNodeFile != NULL && stat(pbsNodeFile, &fileStats) == 0 && S_ISREG(fileStats.st_mode))
	{
		// Parse the pbs node file.
        fileResources = parsePBSNodeFile(pbsNodeFile);
        Print::printf(Print::INFO, "Read resource allocations from PBS node file %s: %d hosts.", pbsNodeFile, fileResources.size());
    }

    // If we found a resource list, parse it into the allocation map.
    if (fileResources.size() > 0)
    {
        // Match the resources from the file with the hostname map.
        for (map<string,ComputeResources>::iterator it=fileResources.begin(); it != fileResources.end(); it++)
        {
            ComputeResources resources=it->second;
            if (!hostnameProcessMap.count(resources.hostname) || !allocatedResources.count(hostnameProcessMap[resources.hostname]))
            {
                Print::printf(Print::WARNING, "Host %s in resource file was not a valid host.", resources.hostname.c_str());
            }
            else
            {
                allocatedResources[hostnameProcessMap[resources.hostname]].cpuCores = resources.cpuCores;
                allocatedResources[hostnameProcessMap[resources.hostname]].gpuDevices = resources.gpuDevices;
            }
        }

        // See if there are any hosts without a resource allocation in the file.
        for (map<int,ComputeResources>::iterator it=allocatedResources.begin(); it != allocatedResources.end(); it++)
        {
            ComputeResources resources=it->second;
            if (resources.cpuCores.size() == 0)
                Print::printf(Print::WARNING, "Host %d (%s) had NO resources allocated in nodefile.", resources.controller_process, resources.hostname.c_str());
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
                  resources.cpuCores.push_back(resources.cpuCores.size());
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
    if (allocatedResources.count(msg.controller_process()))
    {
        // Make sure the hostname matches.
        ComputeResources resources = allocatedResources[msg.controller_process()];
        if (resources.hostname != msg.hostname())
            throw Exception("Host reporting resources available had a mismatched hostname.", msg.hostname().c_str(), msg.controller_process());

        // Set the controller thread.
        resources.controller_thread = (int)msg.controller_thread();

        // If there were no cpus specified, use the default cpu cores and gpu devices.
        if (resources.cpuCores.size() == 0)
        {
            int num=msg.cpu_size();
            if (defaultCPUCores >= 0 && defaultCPUCores < num) num = defaultCPUCores;
            for (int i=0; i<num; i++)
            {
                resources.cpuCores.push_back(msg.cpu(i));
            }

            // If there were no gpu devices specified, also use the default gpu devices.
            if (resources.gpuDevices.size() == 0)
            {
                int num=msg.gpu_size();
                if (defaultGPUDevices >= 0 && defaultGPUDevices < num) num = defaultGPUDevices;
                for (int i=0; i<num; i++)
                {
                    resources.gpuDevices.push_back(msg.gpu(i));
                }
            }
        }

        // Move the resources from the allocated list to the registered list.
        registeredResources[resources.controller_process] = resources;
        allocatedResources.erase(resources.controller_process);

        Print::printf(Print::INFO, "Registered resources for host %s: %d cpu cores, %d gpu devices", resources.hostname.c_str(), resources.cpuCores.size(), resources.gpuDevices.size());
    }
    else
    {
        throw Exception("Host reporting resources available was not in the allocation list.", msg.hostname().c_str());
    }
    return (allocatedResources.size() == 0);
}

ComputeResources ResourceMap::reserveCPUCores(int process, int numberCPUCores, bool Exclusive)
{
    ComputeResources resources = registeredResources[process];
    if ((int)resources.cpuCores.size() >= numberCPUCores)
    {
        ComputeResources reservedResources;
        reservedResources.hostname = resources.hostname;
        reservedResources.controller_process = resources.controller_process;
        reservedResources.controller_thread = resources.controller_thread;
        for (int i=0; i<numberCPUCores; i++)
        {
            reservedResources.cpuCores.push_back(resources.cpuCores[0]);
            if (Exclusive)
            {
                resources.cpuCores.erase(resources.cpuCores.begin());
            }
        }
        registeredResources[process] = resources;
        return reservedResources;
    }
    throw Exception("Insufficient resource on the specified process to reserve a CPU core", process, resources.cpuCores.size(), numberCPUCores);
}

map<int,ComputeResources> ResourceMap::getAvailableResources()
{
    return registeredResources;
}

}
}
