/*
 * University of Illinois Open Source License
 * Copyright 2008 Luthey-Schulten Group, 
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

#ifndef LKMAIN_H_
#define LKMAIN_H_

#include <string>
#include <list>
#include <vector>
#include <ctime>
#include "lm/me/MESolverFactory.h"

using std::string;
using std::vector;

/**
 * The function being performed.
 */
extern string functionOption;

/**
 * The name of the file containing the simulation.
 */
extern string simulationFilename;

/**
 * The number of replicates of the simulation that should be performed.
 */
extern vector<int> replicates;

/**
 * The interval at which the results file should be checkpointed.
 */
extern time_t checkpointInterval;

/**
 * If a global abort signal has been received.
 */
extern volatile bool globalAbort;

/**
 * The solver to use for the simulations.
 */
extern lm::me::MESolverFactory solverFactory;

#ifdef OPT_MPI
/**
 * The filename for the node list.
 */
extern string nodelistFilename;
#endif

/**
 * The number of cpu cores assigned to each process.
 */
extern int numberCpuCores;

/**
 * The number of cpu cores to assign per replicate (can be a fraction, e.g., 1/2, 1/4, etc).
 */
extern float cpuCoresPerReplicate;

#ifdef OPT_CUDA

/**
 * The cuda devices assigned to each process.
 */
extern vector<int> cudaDevices;

/**
 * The number of cuda devices to assign per replicate (can be a fraction, e.g., 1/2, 1/4, etc).
 */
extern float cudaDevicesPerReplicate;

/**
 * Whether we should print the cuda device capabilities on startup.
 */
extern bool shouldPrintCudaCapabilities;

#endif

/**
 * Whether we should reserve a core for the output thread.
 */
extern bool shouldReserveOutputCore;

#ifdef OPT_PYTHON
/**
 * The directory containing the supporting files.
 */
extern string libDir;

/**
 * The directory containing the supporting files.
 */
extern string userLibDir;

/**
 * The path of directories containing user scripts to execute at startup.
 */
extern string scriptPath;

/**
 * The script filename being executed, if applicable.
 */
extern string scriptFilename;

/**
 * The arguments for the script, if applicable.
 */
extern vector<string> scriptArguments;

#endif

void printCopyright(int argc, char** argv);
int getPhysicalCpuCores();
void parseArguments(int argc, char** argv);
void parseIntListArg(vector<int> & list, char * option);
time_t parseTimeArg(char * option);
float parseIntReciprocalArg(char * option);
void printUsage(int argc, char** argv);
void discoverEnvironment();
void initPython();
void finalizePython();
void startInterpreter();
void executeScript(std::string filename, std::list<std::string> arguments, int replicate=0);

#endif
