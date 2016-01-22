/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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
#include "hrtime.h"

using std::string;
using std::vector;

/**
 * The function being performed.
 */
string functionOption = "interpreter";

/**
 * The name of the file containing the simulation input.
 */
string simulationInputFilename;

/**
 * The name of the file containing the simulation output.
 */
string simulationOutputFilename;

/**
 * The output writer to use for the simulations.
 */
string outputWriterClassName;

/**
 * The number of replicates of the simulation that should be performed.
 */
vector<uint64_t> replicates;

/**
 * The interval at which the results file should be checkpointed.
 */
time_t checkpointInterval = 0;

/**
 * If a global abort signal has been received.
 */
volatile bool globalAbort = false;

/**
 * The supervisor to use for the simulations.
 */
string supervisorClassName;

/**
 * The solver to use for the simulations.
 */
string solverClassName;

/**
 * The filename for the resource list.
 */
string resourceFilename;

/**
 * The number of cpu cores assigned to each process.
 */
int cpuCores;

/**
 * The number of cpu cores to assign per runner (can be a fraction, e.g., 1/2, 1/4, etc).
 */
double cpuCoresPerRunner;

/**
 * Whether we should use CPU affinity.
 */
bool useCPUAffinity;

/**
 * The number gpu devices assigned to each process.
 */
int gpuDevices;

/**
 * The number of gpu devices to assign per runner (can be a fraction, e.g., 1/2, 1/4, etc).
 */
double gpuDevicesPerRunner;

/**
 * Whether we should print the cuda device capabilities on startup.
 */
bool shouldPrintGPUCapabilities;

/**
 * Whether we should reserve a core for the output thread.
 */
bool shouldReserveOutputCore;

/**
 * Flag to indicate that forward flux simulation is in use.
 */
bool ffluxFlag;

/*
 * Flag to indicate that we want intermediate output related to simulation results
 */
bool intermediateOutputFlag;

/**
 * Flag to run input output testing
 */
bool ioTestFlag;

/**
 * Prints the copyright notice.
 */

void printCopyright(int argc, char** argv)
{
    std::cout << "Lattice Microbe ES v" << VERSION_NUM << " build " << BUILD_INFO << " in " << (sizeof(uintv_t)*8) << "-bit mode with options";
#ifdef OPT_CUDA
    std::cout << " CUDA";
#endif
    std::cout << " MPI";
    std::cout << "." << std::endl;
    std::cout << "Copyright (C) " << COPYRIGHT_DATE << " Luthey-Schulten Group, University of Illinois at Urbana-Champaign." << std::endl;
    std::cout << "Copyright (C) " << COPYRIGHT_DATE_JHU << " Roberts Group, Johns Hopkins University." << std::endl << std::endl;
}

/**
 * Parses the command line arguments.
 */
void parseArguments(int argc, char** argv)
{
    // Set any default options.
    replicates.clear();
    replicates.push_back(1);

    cpuCores = -1;
    cpuCoresPerRunner = 1.0;
    useCPUAffinity = false;
    gpuDevices = -1;
    gpuDevicesPerRunner = 1.0;
    shouldPrintGPUCapabilities = true;

    simulationInputFilename = "";
    simulationOutputFilename = "";
    outputWriterClassName = "lm::io::hdf5::Hdf5OutputWriter";
    supervisorClassName = "lm::replicates::ReplicateSupervisor";
    solverClassName = "lm::cme::GillespieDSolver";

    shouldReserveOutputCore = true;
    ffluxFlag = false;
    intermediateOutputFlag = false;
    ioTestFlag = false;

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

        //See if the user is trying to perfrom a debug test.
        else if (strcmp(option, "--debug") == 0) {
            functionOption = "debug";
            break;
        }

        //See if the user is trying to execute an iotest.
        else if (strcmp(option, "-iotest") == 0 || strcmp(option, "--input-ouput-test") == 0)
        {
            functionOption = "iotest";

            // Get the filename.
            if (i < argc-1)
                simulationInputFilename = argv[++i];
            else
                throw lm::CommandLineArgumentException("missing simulation input file.");
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
                simulationInputFilename = argv[++i];
            else
                throw lm::CommandLineArgumentException("missing simulation input file.");
        }

        //See if the user is trying to set the output filename.
        else if ((strcmp(option, "-fo") == 0 || strcmp(option, "--output-file") == 0) && i < (argc-1))
        {
            simulationOutputFilename=argv[++i];
        }
        else if (strncmp(option, "--output-file=", strlen("--output-file=")) == 0)
        {
            simulationOutputFilename=option+strlen("--output-file=");
        }

        //See if the user is trying to set the output format.
        else if ((strcmp(option, "-ff") == 0 || strcmp(option, "--output-format") == 0) && i < (argc-1))
        {
            outputWriterClassName=parseOutputFormatArg(argv[++i]);
        }
        else if (strncmp(option, "--output-format=", strlen("--output-format=")) == 0)
        {
            outputWriterClassName=parseOutputFormatArg(option+strlen("--output-format="));
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
            solverClassName = "lm::rdme::NextSubvolumeSolver";
        }
		#else
        else if ((strcmp(option, "-sp") == 0 || strcmp(option, "--spatially-resolved") == 0))
        {
            solverClassName = "lm::rdme::NextSubvolumeSolver";
        }
		#endif
        else if ((strcmp(option, "-ws") == 0 || strcmp(option, "--well-stirred") == 0))
        {
            solverClassName = "lm::cme::GillespieDSolver";
        }
        else if ((strcmp(option, "-m") == 0 || strcmp(option, "--model") == 0) && i < (argc-1))
        {
            solverClassName = argv[++i];
        }
        else if (strncmp(option, "--model=", strlen("--model=")) == 0)
        {
            solverClassName = option+strlen("--model=");
        }
        else if ((strcmp(option, "-sl") == 0 || strcmp(option, "--solver") == 0) && i < (argc-1))
        {
            solverClassName = argv[++i];
        }
        else if (strncmp(option, "--solver=", strlen("--solver=")) == 0)
        {
            solverClassName = option+strlen("--solver=");
        }

        //See if the user is trying to set the node list.
        else if ((strcmp(option, "-n") == 0 || strcmp(option, "--nodelist") == 0) && i < (argc-1))
        {
            resourceFilename=argv[++i];
        }
        else if (strncmp(option, "--nodelist=", strlen("--nodelist=")) == 0)
        {
            resourceFilename=option+strlen("--nodelist=");
        }

        //See if the user is trying to set the resource map.
        else if ((strcmp(option, "-m") == 0 || strcmp(option, "--resource-map") == 0) && i < (argc-1))
        {
            resourceFilename=argv[++i];
        }
        else if (strncmp(option, "--resource-map=", strlen("--resource-map=")) == 0)
        {
            resourceFilename=option+strlen("--resource-map=");
        }

        //See if the user is trying to set the number of cpus.
        else if ((strcmp(option, "-c") == 0 || strcmp(option, "--cpu") == 0) && i < (argc-1))
        {
            cpuCores=atoi(argv[++i]);
        }
        else if (strncmp(option, "--cpu=", strlen("--cpu=")) == 0)
        {
            cpuCores=atoi(option+strlen("--cpu="));
        }

         //See if the user is trying to set the number of gpu devices per runner.
         else if ((strcmp(option, "-cr") == 0 || strcmp(option, "--cpus-per-runner") == 0 || strcmp(option, "--cpus-per-replicate") == 0) && i < (argc-1))
         {
             cpuCoresPerRunner=parseIntReciprocalArg(argv[++i]);
         }
        else if (strncmp(option, "--cpus-per-runner=", strlen("--cpus-per-runner=")) == 0)
        {
            cpuCoresPerRunner=parseIntReciprocalArg(option+strlen("--cpus-per-runner="));
        }
         else if (strncmp(option, "--cpus-per-replicate=", strlen("--cpus-per-replicate=")) == 0)
         {
             cpuCoresPerRunner=parseIntReciprocalArg(option+strlen("--cpus-per-replicate="));
         }


        //See if the user is trying to turn on cpu affinity.
         else if ((strcmp(option, "-ca") == 0 || strcmp(option, "--cpu-affinity") == 0))
         {
             useCPUAffinity = true;
         }

         //See if the user is trying to set the gpu devices.
         else if ((strcmp(option, "-g") == 0 || strcmp(option, "--gpu") == 0) && i < (argc-1))
         {
             gpuDevices=atoi(argv[++i]);
         }
         else if (strncmp(option, "--gpu=", strlen("--gpu=")) == 0)
         {
             gpuDevices=atoi(option+strlen("--gpu="));
         }

         //See if the user is trying to set the number of gpu devices per runner.
         else if ((strcmp(option, "-gr") == 0 || strcmp(option, "--gpus-per-runner") == 0 || strcmp(option, "--gpus-per-replicate") == 0) && i < (argc-1))
         {
             gpuDevicesPerRunner=parseIntReciprocalArg(argv[++i]);
         }
         else if (strncmp(option, "--gpus-per-runner=", strlen("--gpus-per-runner=")) == 0)
         {
             gpuDevicesPerRunner=parseIntReciprocalArg(option+strlen("--gpus-per-runner="));
         }
        else if (strncmp(option, "--gpus-per-replicate=", strlen("--gpus-per-replicate=")) == 0)
        {
            gpuDevicesPerRunner=parseIntReciprocalArg(option+strlen("--gpus-per-replicate="));
        }

        //See if the user is trying to turn off cuda capability printing.
         else if ((strcmp(option, "-nc") == 0 || strcmp(option, "--no-capabilities") == 0))
         {
             shouldPrintGPUCapabilities = false;
         }

        //See if the user is trying to turn off cuda capability printing.
         else if ((strcmp(option, "-nr") == 0 || strcmp(option, "--no-reserve-core") == 0))
         {
        	 shouldReserveOutputCore = false;
         }

        //See if the user is trying to use forward flux sampling.
        else if ((strcmp(option, "-fflux") == 0 || strcmp(option, "--use-forward-flux") == 0))
		{
        	 ffluxFlag = true;
        	 supervisorClassName = "lm::fflux::FFluxSupervisor";
		}

        //See if the user is trying to use forward flux sampling.
        else if ((strcmp(option, "-intout") == 0 || strcmp(option, "--intermediate-output") == 0))
        {
             intermediateOutputFlag = true;
        }

        //See if the user is trying to do an input output test.
        else if ((strcmp(option, "-ioflag") == 0 || strcmp(option, "--do-io-test") == 0))
        {
             ioTestFlag = true;
        }

        //This must be an invalid option.
        else {
            throw lm::CommandLineArgumentException(option);
        }
    }

    // Perform some validation.
    if (outputWriterClassName == "lm::io::hdf5::Hdf5OutputWriter" && simulationOutputFilename == "")
        simulationOutputFilename = simulationInputFilename;
    else if (outputWriterClassName == "lm::io::hdf5::Hdf5OutputWriter" && simulationOutputFilename != simulationInputFilename)
        throw lm::CommandLineArgumentException("cannot specify separate input and output files with the hdf5 format.");
    if (outputWriterClassName == "lm::io::sfile::SFileOutputWriter" && simulationOutputFilename == "")
        throw lm::CommandLineArgumentException("missing simulation output file.");
}

string parseOutputFormatArg(char* option)
{
    if (strcmp(option, "hdf5") == 0)
        return "lm::io::hdf5::Hdf5OutputWriter";
    else if (strcmp(option, "sfile") == 0)
        return "lm::io::sfile::SFileOutputWriter";
    else if (strcmp(option, "log") == 0)
        return "lm::io::ConsoleOutputWriter";
    else if (strcmp(option, "null") == 0)
        return "lm::io::NullOutputWriter";
    throw lm::CommandLineArgumentException(option);
}

void parseIntListArg(vector<uint64_t> & list, char* arg)
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

double parseIntReciprocalArg(char * arg)
{
    if (strlen(arg) >= 3 && arg[0] == '1' && arg[1] == '/')
    {
        return 1.0/(double)atoi(arg+2);
    }
    else
    {
        return (double)atoi(arg);
    }
}

/**
 * Prints the usage for the program.
 */
void printUsage(int argc, char** argv)
{
    std::cout << "Usage: mpirun lm (-h|--help)" << std::endl;
    std::cout << "Usage: mpirun lm (-v|--version)" << std::endl;
    std::cout << "Usage: mpirun lm (-l|--list-devices)" << std::endl;
    std::cout << "Usage: mpirun lm [OPTIONS] [SIM_OPTIONS] (-f|--file) input_filename" << std::endl;
    std::cout << std::endl;
    std::cout << "OPTIONS" << std::endl;
    std::cout << "  -fo output_file   --output-file=output_filename The file for the simulation output, if different than the input file. Required for sfile, invalid for hdf5." << std::endl;
    std::cout << "  -ff format        --output-format=format        The file format for the simulation output. Valid values are \"hdf5\" (default)|\"sfile\"|\"log\"|\"null\"." << std::endl;
    std::cout << "  -n node_file      --nodelist=node_file          A file containing the list of nodes on which to run, one line per available CPU core." << std::endl;
    std::cout << "  -m map_file       --resource-map=map_file       A file containing the map of resources to use: hostname processor_id_list gpu_id_list." << std::endl;
    std::cout << "  -c num_cpus       --cpu=num_cpus                The number of CPUs on which to execute (default all)." << std::endl;
    std::cout << "  -cr num           --cpus-per-runner=num         The number of CPUs (possibly fractional) to assign per runner, e.g. \"2\", \"1/4\" (default 1)." << std::endl;
    std::cout << "  -ca               --cpu-affinity                Turn on CPU affinity." << std::endl;
    std::cout << "  -g num_gpus       --gpu=num_gpus                The number of GPUs on which to execute (default all)." << std::endl;
    std::cout << "  -gr num           --gpus-per-runner=num         The number of GPUs (possibly fractional) to assign per runner, e.g. \"2\", \"1/4\" (default 1)." << std::endl;
    std::cout << "  -nc               --no-capabilities             Don't print the capabilities of the GPU devices." << std::endl;
    std::cout << "  -nr               --no-reserve-core             Don't reserve a CPU core for the output thread." << std::endl;
    std::cout << std::endl;
    std::cout << "SIM_OPTIONS" << std::endl;
    std::cout << "  -r replicates     --replicates=replicates       A list of replicates to run, e.g. \"0-9\", \"0,11,21\" (default 0)." << std::endl;
    std::cout << "  -sp               --spatially-resolved          The simulations should use the spatially resolved reaction model (default)." << std::endl;
    std::cout << "  -ws               --well-stirred                The simulations should use the well-stirred reaction model." << std::endl;
    std::cout << "  -sl solver        --solver=solver               The specific solver class to use for the simulations." << std::endl;
    std::cout << "  -ck               --checkpoint=interval         Enable checkpointing with the given interval as hh:mm:ss (default 00:00:00 -- disabled)." << std::endl;
    std::cout << "  -fflux            --use-forward-flux			Enable forward flux sampling (default disabled)." << std::endl;
    std::cout << "  -intout           --intermediate-output         More verbose output. Consists of intermediate values used to calculate standard output.";
}

#include "lm/Types.h"
#include "lm/me/PropensityFunction.h"

void mainDebug(int argc, char** argv)
{
    tuple<uint> t1(3, (uint[]){1,2,3});
    tuple<uint> t2(3, (uint[]){4,5,6});
    t1.print("\n");
    t2.print("\n");
    t2=t1;
    t1.print("\n");
    t2.print("\n");

    tuple<uint> t3(10,5,3);
    ndarray<double> a1(t3);
    a1.print("\n");
    for (uint r=0; r<a1.shape[0]; r++)
        for (uint c=0; c<a1.shape[1]; c++)
            a1[tuple<uint>(r,c,0)] = (double)r;
    a1.print("\n");
    for (uint r=0; r<a1.shape[0]; r++)
        for (uint c=0; c<a1.shape[1]; c++)
            a1[tuple<uint>(r,c,0)] = (double)c;
    a1.print("\n");
    for (uint r=0; r<a1.shape[0]; r++)
        for (uint c=0; c<a1.shape[1]; c++)
            for (uint d=0; d<a1.shape[2]; d++)
                a1[tuple<uint>(r,c,d)] = (double)d;
    a1.print("\n");

    lm::me::PropensityFunctions f;
}










