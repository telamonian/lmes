/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
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

#include <string>
#include <list>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <sys/stat.h>
#if defined(MACOSX)
#include <sys/sysctl.h>
#elif defined(LINUX)
#include <sys/sysinfo.h>
#endif
#ifdef OPT_CUDA
#include "lm/Cuda.h"
#endif
#include "lm/Version.h"
#include "lm/Types.h"
#include "lm/Exceptions.h"
#include "lm/main/Main.h"
#include "lm/me/MESolverFactory.h"

using std::string;
using std::vector;

/**
 * The function being performed.
 */
string functionOption = "interpreter";

/**
 * The name of the file containing the simulation.
 */
string simulationFilename;

/**
 * The number of replicates of the simulation that should be performed.
 */
vector<int> replicates;

/**
 * The interval at which the results file should be checkpointed.
 */
time_t checkpointInterval = 0;

/**
 * If a global abort signal has been received.
 */
volatile bool globalAbort = false;

/**
 * The solver to use for the simulations.
 */
lm::me::MESolverFactory solverFactory;

/**
 * The number of cpu cores assigned to each process.
 */
int numberCpuCores;

/**
 * The number of cpu cores to assign per replicate (can be a fraction, e.g., 1/2, 1/4, etc).
 */
float cpuCoresPerReplicate;

#ifdef OPT_CUDA

/**
 * The cuda devices assigned to each process.
 */
vector<int> cudaDevices;

/**
 * The number of cuda devices to assign per replicate (can be a fraction, e.g., 1/2, 1/4, etc).
 */
float cudaDevicesPerReplicate;

/**
 * Whether we should print the cuda device capabilities on startup.
 */
bool shouldPrintCudaCapabilities;

#endif

/**
 * Whether we should reserve a core for the output thread.
 */
bool shouldReserveOutputCore;



/**
 * Prints the copyright notice.
 */

void printCopyright(int argc, char** argv)
{
    std::cout << "Lattice Microbe v" << VERSION_NUM << " build " << BUILD_INFO << " in " << (sizeof(uintv_t)*8) << "-bit mode with options";
#ifdef OPT_CUDA
    std::cout << " CUDA";
#endif
#ifdef OPT_MPI
    std::cout << " MPI";
#endif
    std::cout << "." << std::endl;
    std::cout << "Copyright (C) " << COPYRIGHT_DATE << " Luthey-Schulten Group, University of Illinois at Urbana-Champaign." << std::endl;
    std::cout << "Copyright (C) " << COPYRIGHT_DATE_JHU << " Roberts Group, Johns Hopkins University." << std::endl << std::endl;
}

/**
 * Gets the number of physical cpu cores on the system.
 */
int getPhysicalCpuCores()
{
    // Get the number of processors.
    #if defined(MACOSX)
    uint physicalCpuCores;
    size_t  physicalCpuCoresSize=sizeof(physicalCpuCores);
    sysctlbyname("hw.activecpu",&physicalCpuCores,&physicalCpuCoresSize,NULL,0);
    return physicalCpuCores;
    #elif defined(LINUX)
    return get_nprocs();
    #else
    #error "Unsupported architecture."
    #endif
}

/**
 * Parses the command line arguments.
 */
void parseArguments(int argc, char** argv)
{
    // Set any default options.
    replicates.clear();
    replicates.push_back(1);

    numberCpuCores = getPhysicalCpuCores();
    cpuCoresPerReplicate = 1.0;

    solverFactory.setSolver("lm::rdme::MpdRdmeSolver");

    #ifdef OPT_CUDA
    cudaDevices.clear();
    for (int i=0; i<lm::CUDA::getNumberDevices(); i++)
        cudaDevices.push_back(i);
    cudaDevicesPerReplicate = 1.0;
    shouldPrintCudaCapabilities = true;
    #endif
    shouldReserveOutputCore = true;

    // Parse any arguments.
    for (int i=1; i<argc; i++)
    {
        char *option = argv[i];
        while (*option == ' ') option++;
        
        //See if the user is trying to get help.
        if (strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
        	functionOption = "help";
        	break;
        }
        
        //See if the user is trying to get the version info.
        else if (strcmp(option, "-v") == 0 || strcmp(option, "--version") == 0) {
        	functionOption = "version";
            break;
        }
            
        //See if the user is trying to get the device info.
        else if (strcmp(option, "-l") == 0 || strcmp(option, "--list-devices") == 0) {
            functionOption = "devices";
        }

        //See if the user is trying to execute a simulation.
        else if (strcmp(option, "-f") == 0 || strcmp(option, "--file") == 0)
        {
            functionOption = "simulation";

            // Get the filename.
            if (i < argc-1)
                simulationFilename = argv[++i];
            else
                throw lm::CommandLineArgumentException("missing simulation filename.");
        }

        //See if the user is trying to set the replicates.
        else if ((strcmp(option, "-r") == 0 || strcmp(option, "--replicates") == 0) && i < (argc-1))
        {
            parseIntListArg(replicates, argv[++i]);
        }
        else if (strncmp(option, "--replicates=", strlen("--replicates=")) == 0)
        {
            parseIntListArg(replicates, option+strlen("--replicates="));
        }

        //See if the user is trying to set the checkpoint interval.
        else if ((strcmp(option, "-ck") == 0 || strcmp(option, "--checkpoint") == 0) && i < (argc-1))
        {
            checkpointInterval=parseTimeArg(argv[++i]);
        }
        else if (strncmp(option, "--checkpoint=", strlen("--checkpoint=")) == 0)
        {
            checkpointInterval=parseTimeArg(option+strlen("--checkpoint="));
        }

        //See if the user is trying to set the solver.
		#ifdef OPT_CUDA
        else if ((strcmp(option, "-sp") == 0 || strcmp(option, "--spatially-resolved") == 0))
        {
            solverFactory.setSolver("lm::rdme::MpdRdmeSolver");
        }
		#else
        else if ((strcmp(option, "-sp") == 0 || strcmp(option, "--spatially-resolved") == 0))
        {
            solverFactory.setSolver("lm::rdme::NextSubvolumeSolver");
        }
		#endif
        else if ((strcmp(option, "-ws") == 0 || strcmp(option, "--well-stirred") == 0))
        {
            solverFactory.setSolver("lm::cme::GillespieDSolver");
        }
        else if ((strcmp(option, "-m") == 0 || strcmp(option, "--model") == 0) && i < (argc-1))
        {
            solverFactory.setSolver(argv[++i]);
        }
        else if (strncmp(option, "--model=", strlen("--model=")) == 0)
        {
            solverFactory.setSolver(option+strlen("--model="));
        }
        else if ((strcmp(option, "-sl") == 0 || strcmp(option, "--solver") == 0) && i < (argc-1))
        {
            solverFactory.setSolver(argv[++i]);
        }
        else if (strncmp(option, "--solver=", strlen("--solver=")) == 0)
        {
            solverFactory.setSolver(option+strlen("--solver="));
        }

        //See if the user is trying to set the number of cpus.
        else if ((strcmp(option, "-c") == 0 || strcmp(option, "--cpu") == 0) && i < (argc-1))
        {
            numberCpuCores=atoi(argv[++i]);
        }
        else if (strncmp(option, "--cpu=", strlen("--cpu=")) == 0)
        {
            numberCpuCores=atoi(option+strlen("--cpu="));
        }

        //See if the user is trying to set the number of cuda devices per replicate.
         else if ((strcmp(option, "-cr") == 0 || strcmp(option, "--cpus-per-replicate") == 0) && i < (argc-1))
         {
             cpuCoresPerReplicate=parseIntReciprocalArg(argv[++i]);
         }
         else if (strncmp(option, "--cpus-per-replicate=", strlen("--cpus-per-replicate=")) == 0)
         {
             cpuCoresPerReplicate=parseIntReciprocalArg(option+strlen("--cpus-per-replicate="));
         }

#ifdef OPT_CUDA
        //See if the user is trying to set the cuda devices.
         else if ((strcmp(option, "-g") == 0 || strcmp(option, "--gpu") == 0) && i < (argc-1))
         {
             parseIntListArg(cudaDevices, argv[++i]);
         }
         else if (strncmp(option, "--gpu=", strlen("--gpu=")) == 0)
         {
             parseIntListArg(cudaDevices, option+strlen("--gpu="));
         }

        //See if the user is trying to set the number of cuda devices per replicate.
         else if ((strcmp(option, "-gr") == 0 || strcmp(option, "--gpus-per-replicate") == 0) && i < (argc-1))
         {
             cudaDevicesPerReplicate=parseIntReciprocalArg(argv[++i]);
         }
         else if (strncmp(option, "--gpus-per-replicate=", strlen("--gpus-per-replicate=")) == 0)
         {
             cudaDevicesPerReplicate=parseIntReciprocalArg(option+strlen("--gpus-per-replicate="));
         }
             
        //See if the user is trying to turn off cuda capability printing.
         else if ((strcmp(option, "-nc") == 0 || strcmp(option, "--no-capabilities") == 0))
         {
             shouldPrintCudaCapabilities = false;
         }
#endif
        //See if the user is trying to turn off cuda capability printing.
         else if ((strcmp(option, "-nr") == 0 || strcmp(option, "--no-reserve-core") == 0))
         {
        	 shouldReserveOutputCore = false;
         }

        //This must be an invalid option.
        else {
            throw lm::CommandLineArgumentException(option);
        }
    }
}

void parseIntListArg(vector<int> & list, char * arg)
{
    list.clear();
    char * argbuf = new char[strlen(arg)+1];
    strcpy(argbuf,arg);
    char * pch = strtok(argbuf," ,;:\"");
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

time_t parseTimeArg(char * arg)
{
    char * argbuf = new char[strlen(arg)+1];
    strcpy(argbuf,arg);
    char * pch = strtok(argbuf,":");

    // Parse the arguments into tokens.
    int tokenNumber=0;
    int tokens[3];
    while (pch != NULL)
    {
        if (tokenNumber < 3)
        {
            tokens[tokenNumber++] = atoi(pch);
        }
        else
        {
            delete[] argbuf;
            throw lm::CommandLineArgumentException(arg);
        }
        pch = strtok(NULL,":");
    }
    delete[] argbuf;

    // Calculate the time from the tokens.
    time_t time=0;
    if (tokenNumber == 1)
        time = tokens[0];
    else if (tokenNumber == 2)
        time = (60*tokens[0])+tokens[1];
    else if (tokenNumber == 3)
        time = (3600*tokens[0])+(60*tokens[1])+tokens[2];

    return time;
}

float parseIntReciprocalArg(char * arg)
{
    if (strlen(arg) >= 3 && arg[0] == '1' && arg[1] == '/')
    {
        return 1.0f/(float)atoi(arg+2);
    }
    else
    {
        return (float)atoi(arg);
    }
}

/**
 * Prints the usage for the program.
 */
void printUsage(int argc, char** argv)
{
#ifndef OPT_MPI
	std::cout << "Usage: lm (-h|--help)" << std::endl;
	std::cout << "Usage: lm (-v|--version)" << std::endl;
	std::cout << "Usage: lm [OPTIONS] (-l|--list-devices)" << std::endl;
	std::cout << "Usage: lm [OPTIONS]" << std::endl;
	std::cout << "Usage: lm [OPTIONS] (-s|--script) script_filename [(-sa|--script-args) script_arguments+]" << std::endl;
    std::cout << "Usage: lm [OPTIONS] [SIM_OPTIONS] (-f|--file) simulation_filename " << std::endl;
#else
    std::cout << "Usage: mpirun lm (-h|--help)" << std::endl;
    std::cout << "Usage: mpirun lm (-v|--version)" << std::endl;
    std::cout << "Usage: mpirun lm (-l|--list-devices)" << std::endl;
    std::cout << "Usage: mpirun lm [OPTIONS] [SIM_OPTIONS] (-f|--file) simulation_filename" << std::endl;
#endif
    std::cout << std::endl;
    std::cout << "OPTIONS" << std::endl;
    std::cout << "  -c num_cpus       --cpu=num_cpus               The number of CPUs on which to execute (default all)." << std::endl;
    std::cout << "  -cr num           --cpus-per-replicate=num     The number of CPUs (possibly fractional) to assign per replicate, e.g. \"2\", \"1/4\" (default 1)." << std::endl;
#ifdef OPT_CUDA
    std::cout << "  -g cuda_devices   --gpu=cuda_devices           A list of cuda devices on which to execute, e.g. \"0-3\", \"0,2\" (default 0)." << std::endl;
    std::cout << "  -gr num           --gpus-per-replicate=num     The number of cuda devices (possibly fractional) to assign per replicate, e.g. \"2\", \"1/4\" (default 1)." << std::endl;
    std::cout << "  -nc               --no-capabilities            Don't print the capabilities of the CUDA devices." << std::endl;
#endif
    std::cout << "  -nr               --no-reserve-core            Don't reserve a CPU core for the output thread." << std::endl;
    std::cout << std::endl;
    std::cout << "SIM_OPTIONS" << std::endl;
    std::cout << "  -r replicates     --replicates=replicates      A list of replicates to run, e.g. \"0-9\", \"0,11,21\" (default 0)." << std::endl;
    std::cout << "  -sp               --spatially-resolved         The simulations should use the spatially resolved reaction model (default)." << std::endl;
    std::cout << "  -ws               --well-stirred               The simulations should use the well-stirred reaction model." << std::endl;
    std::cout << "  -sl solver        --solver=solver              The specific solver class to use for the simulations." << std::endl;
    std::cout << "  -ck               --checkpoint=interval        Enable checkpointing with the given interval as hh:mm:ss (default 00:00:00 -- disabled)." << std::endl;
}

