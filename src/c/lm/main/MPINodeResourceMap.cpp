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
#include "lm/main/MPINodeResourceMap.h"

using std::ifstream;

namespace lm {
namespace main {

MPINodeResourceMap::MPINodeResourceMap(list<string>hostnames, string nodelistFilename, int defaultNumberCpuCores)
:numberNodes(hostnames.size()),cpuCoresTable(NULL)
{
	// Create the initial maps.
	int i=0;
	for (list<string>::iterator it=hostnames.begin(); it != hostnames.end(); it++,i++)
	{
		resourceMap[i] = ComputeResources();
		resourceMap[i].hostname = *it;
		hostnameMap[*it] = i;
	}

	// Create the accessory tables.
	cpuCoresTable = new int[numberNodes];
	memset(cpuCoresTable, 0, sizeof(int)*numberNodes);


	// Check to see if we can find a hosts file.
	struct stat fileStats;
	char * pbsNodeFile = getenv("PBS_NODEFILE");
	if (nodelistFilename != "" && stat(nodelistFilename.c_str(), &fileStats) == 0 && S_ISREG(fileStats.st_mode))
	{
		// Parse the node file.
		list<string> nodes = parseNodeFile(nodelistFilename);
		Print::printf(Print::INFO, "Read resource allocations from node file %s: %d entries.", nodelistFilename.c_str(), nodes.size());

		// Extract the resources to be assigned from the node file.
		int i=0;
		for (list<string>::iterator it=nodes.begin(); it != nodes.end(); it++,i++)
		{
			string node=*it;
			if (!hostnameMap.count(node)) throw lm::Exception("Invalid node in nodefile", node.c_str());
			resourceMap[hostnameMap[node]].cpuCores.push_back(resourceMap[hostnameMap[node]].cpuCores.size());
		}

		// See if there are any hosts without a resource allocation.
		for (int i=0; i<numberNodes; i++)
			if (resourceMap[i].cpuCores.size() == 0)
				Print::printf(Print::WARNING, "Host %s had NO resources allocated in nodefile.", resourceMap[i].hostname.c_str());
	}
	else if (pbsNodeFile != NULL && stat(pbsNodeFile, &fileStats) == 0 && S_ISREG(fileStats.st_mode))
	{
		// Parse the pbs node file.
		list<string> nodes = parseNodeFile(string(pbsNodeFile));
		Print::printf(Print::INFO, "Read resource allocations from PBS node file %s: %d entries.", pbsNodeFile, nodes.size());

		// Extract the resources to be assigned from the node file.
		int i=0;
		for (list<string>::iterator it=nodes.begin(); it != nodes.end(); it++,i++)
		{
			string node=*it;
			if (!hostnameMap.count(node)) throw lm::Exception("Invalid node in PBS_NODEFILE", node.c_str());
			resourceMap[hostnameMap[node]].cpuCores.push_back(resourceMap[hostnameMap[node]].cpuCores.size());
		}

		// See if there are any hosts without a resource allocation.
		for (int i=0; i<numberNodes; i++)
			if (resourceMap[i].cpuCores.size() == 0)
				Print::printf(Print::WARNING, "Host %s had NO resources allocated in PBS_NODEFILE.", resourceMap[i].hostname.c_str());

	}
	else
	{
		// Fill in the defaults.
		for (int i=0; i<numberNodes; i++)
		{
			for (int j=0; j<defaultNumberCpuCores; j++)
				resourceMap[i].cpuCores.push_back(j);
		}
	}

	// Fill in the accessory tables.
	for (int i=0; i<numberNodes; i++)
	{
		cpuCoresTable[i] = resourceMap[i].cpuCores.size();
	}


}

MPINodeResourceMap::~MPINodeResourceMap()
{
	if (cpuCoresTable != NULL) delete[] cpuCoresTable; cpuCoresTable = NULL;
}

list<string> MPINodeResourceMap::parseNodeFile(string filename)
{
	  list<string> nodes;

	  ifstream file(filename.c_str());
	  string node;

	  while (getline(file, node))
	  {
		  nodes.push_back(node);
	  }

	  return nodes;
}

}
}
