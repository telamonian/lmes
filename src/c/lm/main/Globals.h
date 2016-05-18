/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
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
#ifndef LM_MAIN_GLOBALS_H
#define LM_MAIN_GLOBALS_H

#include <ctime>
#include <list>
#include <string>
#include <vector>
#include "lm/Types.h"
#include "hrtime.h"

using std::string;
using std::vector;

/**
 * The function being performed.
 */
extern string functionOption;

/**
 * The name of the file containing the simulation input.
 */
extern string simulationInputFilename;

/**
 * The name of the file containing the simulation output.
 */
extern string simulationOutputFilename;

/**
 * The output writer to use for the simulations.
 */
extern string outputWriterClassName;

/**
 * The number of replicates of the simulation that should be performed.
 */
extern vector<uint64_t> replicates;

/**
 * The interval at which the results file should be checkpointed.
 */
extern time_t checkpointInterval;

/**
 * If a global abort signal has been received.
 */
extern volatile bool globalAbort;

/**
 * The supervisor to use for the simulations.
 */
extern string supervisorClassName;

/**
 * The solver to use for the simulations.
 */
extern string solverClassName;

/**
 * The filename for the resource list.
 */
extern string resourceFilename;

/**
 * The number of cpu cores assigned to each process.
 */
extern int cpuCores;

/**
 * The number of cpu cores to assign per runner (can be a fraction, e.g., 1/2, 1/4, etc).
 */
extern double cpuCoresPerRunner;

/**
 * Whether we should use CPU affinity.
 */
extern bool useCPUAffinity;

/**
 * The number gpu devices assigned to each process.
 */
extern int gpuDevices;

/**
 * The number of gpu devices to assign per runner (can be a fraction, e.g., 1/2, 1/4, etc).
 */
extern double gpuDevicesPerRunner;

/**
 * Whether we should print the cuda device capabilities on startup.
 */
extern bool shouldPrintGPUCapabilities;

/**
 * Whether we should reserve a core for the output thread.
 */
extern bool shouldReserveOutputCore;

/**
 * Flag to indicate that forward flux simulation is in use.
 */
extern bool ffluxFlag;

/*
 * Flag to indicate that we want intermediate output related to simulation results
 */
extern bool intermediateOutputFlag;

/*
 * Flag that determines whether or not to track degree advancement in addition to species count
 */
extern bool daFlag;

/*
 * Flag to indicate that we need to initialize the order parameters and update them at every simulation step
 */
extern bool opActivatedFlag;

/*
 * Flag that determines whether or not to track order parameter values in addition to species counts
 */
extern bool opTrackingFlag;

/*
 * Flag to indicate that we're running a test of the program's input and output
 */
extern bool ioTestFlag;

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

#endif
