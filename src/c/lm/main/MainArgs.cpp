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
#include "hrtime.h"
#include "lm/ClassFactory.h"
#ifdef OPT_CUDA
#include "lm/Cuda.h"
#endif
#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/Version.h"
#include "lm/main/Globals.h"
#include "lm/main/MainArgs.h"

using std::string;
using std::vector;


void printCopyright(int argc, char** argv)
{
    std::cout << "Lattice Microbe ES v" << VERSION_NUM << " build " << BUILD_INFO << " in " << (sizeof(uintv_t)*8) << "-bit mode with options";
#ifdef OPT_AVX
    std::cout << " AVX";
#endif
#ifdef OPT_FMA
    std::cout << " FMA";
#endif
#ifdef OPT_CUDA
    std::cout << " CUDA";
#endif
#ifdef OPT_MPI
    std::cout << " MPI";
#endif
#ifdef OPT_SBML
    std::cout << " SBML";
#endif
#ifdef OPT_SNAPPY
    std::cout << " SNAPPY";
#endif
#ifdef OPT_SVML
    std::cout << " SVML";
#endif
    std::cout << "." << std::endl;
    std::cout << "Copyright (C) " << COPYRIGHT_DATE_JHU << " Roberts Group, Johns Hopkins University." << std::endl;
    std::cout << "Copyright (C) " << COPYRIGHT_DATE << " Luthey-Schulten Group, University of Illinois at Urbana-Champaign." << std::endl << std::endl;
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
#ifdef OPT_CUDA
    gpuDevicesPerRunner = 1.0;
#else
    gpuDevicesPerRunner = 0.0;
#endif
    shouldPrintGPUCapabilities = true;

    simulationInputFilenames.clear();
    simulationOutputFilename = "";
    outputWriterClassName = "lm::io::hdf5::Hdf5OutputWriter";
    supervisorClassName = "lm::replicates::ReplicateSupervisor";

#ifdef OPT_AVX
    solverClassName = "lm::avx::GillespieDSolverAVX";
#else
    solverClassName = "lm::cme::GillespieDSolver";
#endif

    // Set the default communicator.
#ifdef OPT_MPI
    communicatorClassName = "lm::mpi::MPICommunicator";
#else
    communicatorClassName = "lm::message::LocalCommunicator";
#endif

    shouldReserveOutputCore = true;
    ffluxFlag = false;
    intermediateOutputFlag = false;
    daFlag = false;
    opActivatedFlag = false;
    opTrackingFlag = false;

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

        //See if the user is trying to get the device info.
        else if (strcmp(option, "-l") == 0 || strcmp(option, "--list-devices") == 0) {
            functionOption = "devices";
        }

        //See if the user is trying to execute a simulation.
        else if ((strcmp(option, "-f") == 0 || strcmp(option, "--file") == 0) && i < (argc-1))
        {
            functionOption = "simulation";
            parseStringListArg(simulationInputFilenames, argv[++i]);
        }
        else if (strncmp(option, "--file=", strlen("--file=")) == 0)
        {
            functionOption = "simulation";
            parseStringListArg(simulationInputFilenames, option+strlen("--file="));
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

        //See if the user is trying to set the output filename.
        else if ((strcmp(option, "-fo") == 0 || strcmp(option, "--output-file") == 0) && i < (argc-1))
        {
            simulationOutputFilename=argv[++i];
        }
        else if (strncmp(option, "--output-file=", strlen("--output-file=")) == 0)
        {
            simulationOutputFilename=option+strlen("--output-file=");
        }

        //See if the user is trying to set the output record prefix.
        else if ((strcmp(option, "-fp") == 0 || strcmp(option, "--output-prefix") == 0) && i < (argc-1))
        {
            sfileRecordNamePrefix=argv[++i];
        }
        else if (strncmp(option, "--output-prefix=", strlen("--output-prefix=")) == 0)
        {
            sfileRecordNamePrefix=option+strlen("--output-prefix=");
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
            resourceFileFormat = lm::resource::ResourceMap::NODELIST;
        }
        else if (strncmp(option, "--nodelist=", strlen("--nodelist=")) == 0)
        {
            resourceFilename=option+strlen("--nodelist=");
            resourceFileFormat = lm::resource::ResourceMap::NODELIST;
        }

        //See if the user is trying to set the resource map.
        else if ((strcmp(option, "-m") == 0 || strcmp(option, "--resource-map") == 0) && i < (argc-1))
        {
            resourceFilename=argv[++i];
            resourceFileFormat = lm::resource::ResourceMap::RESOURCE_MAP;
        }
        else if (strncmp(option, "--resource-map=", strlen("--resource-map=")) == 0)
        {
            resourceFilename=option+strlen("--resource-map=");
            resourceFileFormat = lm::resource::ResourceMap::RESOURCE_MAP;
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

        //
        // Simulation type arguments.
        //

        //See if the user is trying to perform a replicate sampling simulation.
        else if ((strcmp(option, "-rs") == 0 || strcmp(option, "--replicate-sampling") == 0))
        {
             supervisorClassName = "lm::replicates::ReplicateSupervisor";
        }

        //See if the user is trying to use forward flux sampling.
        else if ((strcmp(option, "-fflux") == 0 || strcmp(option, "--use-forward-flux") == 0))
		{
        	 ffluxFlag = true;
        	 opActivatedFlag = true;
        	 supervisorClassName = "lm::fflux::FFluxSupervisor";
		}

        //See if the user is trying to use forward flux sampling.
        else if ((strcmp(option, "-intout") == 0 || strcmp(option, "--intermediate-output") == 0))
        {
             intermediateOutputFlag = true;
        }

        //See if the user is trying to perform a microenvironment simulation.
        else if ((strcmp(option, "-me") == 0 || strcmp(option, "--microenvironment") == 0))
        {
             supervisorClassName = "lm::microenv::MicroenvironmentSupervisor";
        }

        //See if the user is trying to set the supervisor directly.
        else if ((strcmp(option, "-su") == 0 || strcmp(option, "--supervisor") == 0) && i < (argc-1))
        {
            supervisorClassName = argv[++i];
        }
        else if (strncmp(option, "--supervisor=", strlen("--supervisor=")) == 0)
        {
            supervisorClassName = option+strlen("--supervisor=");
        }


        //See if the user is trying to set the gpu devices.
        else if ((strcmp(option, "-so") == 0 || strcmp(option, "--shared-libraries") == 0) && i < (argc-1))
        {
            vector<string> sharedLibraries;
            parseStringListArg(sharedLibraries, argv[++i]);
            for (vector<string>::iterator it=sharedLibraries.begin(); it!=sharedLibraries.end(); it++)
                lm::ClassFactory::getInstance().registerClassesFromExternalLibrary(*it);
        }
        else if (strncmp(option, "--shared-libraries=", strlen("--shared-libraries=")) == 0)
        {
            vector<string> sharedLibraries;
            parseStringListArg(sharedLibraries, option+strlen("--shared-libraries="));
            for (vector<string>::iterator it=sharedLibraries.begin(); it!=sharedLibraries.end(); it++)
                lm::ClassFactory::getInstance().registerClassesFromExternalLibrary(*it);
        }

        // See if the user is trying to set the communicator.
        else if (strcmp(option, "--local") == 0)
        {
            communicatorClassName = "lm::message::LocalCommunicator";
        }
        else if (strcmp(option, "--mpi") == 0)
        {
            communicatorClassName = "lm::mpi::MPICommunicator";
        }
        else if (strcmp(option, "--mpi-async") == 0)
        {
            communicatorClassName = "lm::mpi::AsyncMPICommunicator";
        }

        //This must be an invalid option.
        else {
            throw lm::CommandLineArgumentException(option);
        }
    }

    // Perform some validation.
    if (functionOption == "simulation" && simulationInputFilenames.size() == 0)
        throw lm::CommandLineArgumentException("missing simulation input file.");

    if (outputWriterClassName == "lm::io::hdf5::Hdf5OutputWriter" && simulationOutputFilename == "")
        simulationOutputFilename = simulationInputFilenames[0];
    else if (outputWriterClassName == "lm::io::hdf5::Hdf5OutputWriter" && simulationOutputFilename != simulationInputFilenames[0])
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

void parseStringListArg(vector<string>& list, char* arg)
{
    list.clear();
    char * argbuf = new char[strlen(arg)+1];
    strcpy(argbuf,arg);
    char * pch = strtok(argbuf," ,;:\"");
    while (pch != NULL)
    {
        if (strlen(pch) > 0) list.push_back(string(pch));
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
    std::cout << "Usage: lm (-h|--help)" << std::endl;
    std::cout << "Usage: lm (-v|--version)" << std::endl;
    std::cout << "Usage: lm (-l|--list-devices)" << std::endl;
    std::cout << "Usage: lm [OPTIONS] [SIM_OPTIONS] (-f input_filename_list | --file=input_filename_list)" << std::endl;
    std::cout << std::endl;
    std::cout << "OPTIONS" << std::endl;
    std::cout << "  -ff format        --output-format=format        The file format for the simulation output. Valid values are \"hdf5\" (default)|\"sfile\"|\"log\"|\"null\"." << std::endl;
    std::cout << "  -fo output_file   --output-file=output_filename The file for the simulation output, if different than the input file. Required for sfile, invalid for hdf5." << std::endl;
    std::cout << "  -fp record_prefix --output-prefix=record_prefix The prefix to use for the record names. Optional for sfile output, invalid for hdf5." << std::endl;
    std::cout << "  -n node_file      --nodelist=node_file          A file containing the list of nodes on which to run." << std::endl;
    std::cout << "  -m map_file       --resource-map=map_file       A file containing the map of resources to use: hostname processor_id_list gpu_id_list." << std::endl;
    std::cout << "  -c num_cpus       --cpu=num_cpus                The number of CPUs on which to execute (default all)." << std::endl;
    std::cout << "  -cr num           --cpus-per-runner=num         The number of CPUs (possibly fractional) to assign per runner, e.g. \"2\", \"1/4\" (default 1)." << std::endl;
    std::cout << "  -ca               --cpu-affinity                Turn on CPU affinity." << std::endl;
    std::cout << "  -g num_gpus       --gpu=num_gpus                The number of GPUs on which to execute (default all)." << std::endl;
    std::cout << "  -gr num           --gpus-per-runner=num         The number of GPUs (possibly fractional) to assign per runner, e.g. \"2\", \"1/4\" (default 1)." << std::endl;
    std::cout << "  -nc               --no-capabilities             Don't print the capabilities of the GPU devices." << std::endl;
    std::cout << "  -nr               --no-reserve-core             Don't reserve a CPU core for the output thread." << std::endl;
    std::cout << "  -so               --shared-libraries=libs       A comma delimited list of shared library to load." << std::endl;
    std::cout << "                    --local                       Use a local communicator on only this process." << std::endl;
    std::cout << "                    --mpi                         Use an MPI communicator across multiple processes." << std::endl;
    std::cout << "                    --mpi-async                   Use an anstnchronous MPI communicator across multiple processes." << std::endl;
    std::cout << std::endl;
    std::cout << "SIM_OPTIONS" << std::endl;
    std::cout << "  -r replicates     --replicates=replicates       A list of replicates to run, e.g. \"0-9\", \"0,11,21\" (default 0)." << std::endl;
    std::cout << "  -sp               --spatially-resolved          The simulations should use the spatially resolved reaction model (default)." << std::endl;
    std::cout << "  -ws               --well-stirred                The simulations should use the well-stirred reaction model." << std::endl;
    std::cout << "  -sl solver        --solver=solver               The specific solver class to use for the simulations." << std::endl;
    std::cout << "  -rs               --replicate-sampling          Perform a replicate sampling simulation (default)." << std::endl;
    std::cout << "  -fflux            --use-forward-flux            Perform a forward-flux simulation." << std::endl;
    std::cout << "  -me               --microenvironment            Perform a microenvironment simulation." << std::endl;
    std::cout << "  -su supervisor    --supervisor=classname        Perform a simulation using the specified supervisor." << std::endl;
    std::cout << "  -ck               --checkpoint=interval         Enable checkpointing with the given interval as hh:mm:ss (default 00:00:00 -- disabled)." << std::endl;
    std::cout << "  -intout           --intermediate-output         More verbose output. Consists of intermediate values used to calculate standard output." << std::endl;
}

#include "hrtime.h"
#include "lm/Exceptions.h"
#include "lm/Types.h"
#include "lm/me/PropensityFunction.h"
#include "lm/rng/XORShift.h"

#include <limits>
#include <immintrin.h>
#include "lm/cme/GillespieDSolver.h"
#include "lm/avx/GillespieDSolverAVX.h"

void mainDebug(int argc, char** argv)
{
    /*
    tuple<uint> t3(10,5,1);
    ndarray<double> a1(t3);
    for (uint r=0; r<a1.shape[0]; r++)
        for (uint c=0; c<a1.shape[1]; c++)
            a1[tuple<uint>(r,c,0)] = (double)r;
    a1.print("\n");
    for (uint r=0; r<a1.shape[0]; r++)
        for (uint c=0; c<a1.shape[1]; c++)
            a1[tuple<uint>(r,c,0)] = (double)c;
    a1.print("\n");

    uint numberSpecies=2;
    uint numberReactions=1;
    ndarray<int> S(tuple<uint>(numberSpecies,numberReactions));
    ndarray<uint> D(tuple<uint>(numberSpecies,numberReactions));

    uint reactionIndex=0;
    S[tuple<uint>(0,reactionIndex)] = -1;
    S[tuple<uint>(1,reactionIndex)] = 1;
    D[tuple<uint>(0,reactionIndex)] = 1;
    D[tuple<uint>(1,reactionIndex)] = 0;
    tuple<double> k(0.1);
    S.print("\n");
    D.print("\n");
    k.print("\n");

    int id=1;
    lm::me::PropensityFunctionFactory fs;
    lm::me::PropensityFunction* f = fs.createPropensityFunction(id, reactionIndex, S, D, k);

    double time=10.0;
    int* speciesCounts=new int[numberSpecies];
    speciesCounts[0] = 10;
    speciesCounts[1] = 3;
    double a = f->calculate(time, speciesCounts);
    S.print("\n");
    D.print("\n");
    k.print("\n");
    printf("%f: a=%f\n",time,a);
    delete[] speciesCounts;
    */

    /*__m256d time = _mm256_setzero_pd();
    __m256d maxTime = _mm256_set1_pd(2.0);
    __m256d totalPropensity = _mm256_set1_pd(2.0);
    __m256d expR = _mm256_setr_pd(1.0, 2.0, 3.0, 4.0);
    __m256d timestep = _mm256_div_pd(expR, totalPropensity);
    time = _mm256_add_pd(time,timestep);

    __m256d comp = _mm256_cmp_pd(time, maxTime, _CMP_GE_OQ);

    double* res = (double*)&comp;
    printf("%lf %lf %lf %lf\n", res[0], res[1], res[2], res[3]);


     // If any new time is past the end time, we are done.
    int allFalse = _mm256_testz_pd(comp,comp);
    printf("%d\n",allFalse);
    */

    /**
      Generate a trajectory.
      */
    /*
    //lm::avx::GillespieDSolverAVX s = new lm::avx::GillespieDSolverAVX();
    lm::cme::GillespieDSolver s;

    vector<int> cpus;
    cpus.push_back(0);
    s.setComputeResources(cpus, vector<int>());

    // First order decay model.
//    lm::input::ReactionModel rm;
//    rm.set_number_species(1);
//    rm.set_number_reactions(1);
//    rm.add_initial_species_count(100);
//    rm.add_reaction();
//    rm.mutable_reaction(0)->set_type(1);
//    rm.mutable_reaction(0)->add_rate_constant(0.5);
//    rm.add_dependency_matrix(1);
//    rm.add_stoichiometric_matrix(-1);

    // First order birth death model.
    lm::input::ReactionModel rm;
    rm.set_number_species(1);
    rm.set_number_reactions(2);
    rm.add_initial_species_count(1000);
    rm.add_reaction();
    rm.mutable_reaction(0)->set_type(0);
    rm.mutable_reaction(0)->add_rate_constant(1000.0);
    rm.add_reaction();
    rm.mutable_reaction(1)->set_type(1);
    rm.mutable_reaction(1)->add_rate_constant(1.0);
    rm.add_dependency_matrix(0);
    rm.add_dependency_matrix(1);
    rm.add_stoichiometric_matrix(1);
    rm.add_stoichiometric_matrix(-1);

    // Three reaction birth death.
//    lm::input::ReactionModel rm;
//    rm.set_number_species(1);
//    rm.set_number_reactions(3);
//    rm.add_initial_species_count(100);
//    rm.add_reaction();
//    rm.mutable_reaction(0)->set_type(1);
//    rm.mutable_reaction(0)->add_rate_constant(0.5);
//    rm.add_reaction();
//    rm.mutable_reaction(1)->set_type(0);
//    rm.mutable_reaction(1)->add_rate_constant(100.0);
//    rm.add_reaction();
//    rm.mutable_reaction(2)->set_type(1);
//    rm.mutable_reaction(2)->add_rate_constant(0.5);
//    rm.add_dependency_matrix(1);
//    rm.add_dependency_matrix(0);
//    rm.add_dependency_matrix(1);
//    rm.add_stoichiometric_matrix(-1);
//    rm.add_stoichiometric_matrix(1);
//    rm.add_stoichiometric_matrix(-1);

    // Two species parallel three reaction birth death.
//    lm::input::ReactionModel rm;
//    rm.set_number_species(2);
//    rm.set_number_reactions(6);
//    rm.add_initial_species_count(100);
//    rm.add_reaction();
//    rm.mutable_reaction(0)->set_type(1);
//    rm.mutable_reaction(0)->add_rate_constant(0.5);
//    rm.add_reaction();
//    rm.mutable_reaction(1)->set_type(0);
//    rm.mutable_reaction(1)->add_rate_constant(100.0);
//    rm.add_reaction();
//    rm.mutable_reaction(2)->set_type(1);
//    rm.mutable_reaction(2)->add_rate_constant(0.5);
//    rm.add_reaction();
//    rm.mutable_reaction(3)->set_type(1);
//    rm.mutable_reaction(3)->add_rate_constant(0.5);
//    rm.add_reaction();
//    rm.mutable_reaction(4)->set_type(0);
//    rm.mutable_reaction(4)->add_rate_constant(10.0);
//    rm.add_reaction();
//    rm.mutable_reaction(5)->set_type(1);
//    rm.mutable_reaction(5)->add_rate_constant(0.5);
//    rm.add_dependency_matrix(1);
//    rm.add_dependency_matrix(0);
//    rm.add_dependency_matrix(1);
//    rm.add_dependency_matrix(0);
//    rm.add_dependency_matrix(0);
//    rm.add_dependency_matrix(0);
//    rm.add_dependency_matrix(0);
//    rm.add_dependency_matrix(0);
//    rm.add_dependency_matrix(0);
//    rm.add_dependency_matrix(1);
//    rm.add_dependency_matrix(0);
//    rm.add_dependency_matrix(1);
//    rm.add_stoichiometric_matrix(-1);
//    rm.add_stoichiometric_matrix(1);
//    rm.add_stoichiometric_matrix(-1);
//    rm.add_stoichiometric_matrix(0);
//    rm.add_stoichiometric_matrix(0);
//    rm.add_stoichiometric_matrix(0);
//    rm.add_stoichiometric_matrix(0);
//    rm.add_stoichiometric_matrix(0);
//    rm.add_stoichiometric_matrix(0);
//    rm.add_stoichiometric_matrix(-1);
//    rm.add_stoichiometric_matrix(1);
//    rm.add_stoichiometric_matrix(-1);

    // Set the reaction model.
    s.setReactionModel(rm);

    // Set the limits.
    lm::io::TrajectoryLimits limits;
    //limits.set_max_time_limit(100.0);
    s.setLimits(limits);

    // Reset the solver.
    s.reset();

    // Set the initial state.
    lm::io::TrajectoryState state;
    state.set_trajectory_id(1);
    state.mutable_cme_state();
    state.mutable_cme_state()->mutable_species_counts()->set_trajectory_id(state.trajectory_id());
    state.mutable_cme_state()->mutable_species_counts()->set_number_species(rm.number_species());
    state.mutable_cme_state()->mutable_species_counts()->set_number_entries(1);
    for (int i=0; i<rm.number_species(); i++)
        state.mutable_cme_state()->mutable_species_counts()->add_species_count(rm.initial_species_count(i));
    state.mutable_cme_state()->mutable_species_counts()->add_time(0.0);
    s.setState(state);

    hrtime start = getHrTime();
    long long steps = s.generateTrajectory(100000000);
    hrtime stop = getHrTime();
    printf("Performed %lld steps in %0.3f seconds (%0.4e steps/second)\n",steps,convertHrToSeconds(stop-start),double(steps)/convertHrToSeconds(stop-start));
   */


    /**
      * Write out a bunch of random numbers.
      */
    /**/
    {
        lm::rng::XORShift rng(0,0);
        double* rngValues = NULL;
        double* expRngValues = NULL;
        int rngCount=10000000;

#ifdef OPT_AVX
        POSIX_EXCEPTION_CHECK(posix_memalign((void**)&rngValues, DOUBLES_PER_AVX*sizeof(double), rngCount*sizeof(double)));
        POSIX_EXCEPTION_CHECK(posix_memalign((void**)&expRngValues, DOUBLES_PER_AVX*sizeof(double), rngCount*sizeof(double)));
#endif

        // Warmup.
        rng.getRandomDoubles(rngValues,rngCount);
        rng.getExpRandomDoubles(expRngValues,rngCount);
        rng.getRandomDoubles(rngValues,rngCount, true);
        rng.getExpRandomDoubles(expRngValues,rngCount, true);

#ifdef OPT_AVX
        // Test with avx.
        {
        hrtime start = getHrTime();
        rng.getRandomDoubles(rngValues,rngCount, true);
        hrtime stop = getHrTime();
        printf("Calculated %d norm rngs with avx in %0.3f seconds (%0.4e rngs/second)\n",rngCount,convertHrToSeconds(stop-start),double(rngCount)/convertHrToSeconds(stop-start));
        start = getHrTime();
        rng.getExpRandomDoubles(expRngValues,rngCount, true);
        stop = getHrTime();
        printf("Calculated %d exp rngs with avx in %0.3f seconds (%0.4e rngs/second)\n",rngCount,convertHrToSeconds(stop-start),double(rngCount)/convertHrToSeconds(stop-start));
        FILE* f = fopen("rng-avx.txt", "w");
        for (int i=0; i<rngCount; i++)
            fprintf(f, "%e %e\n", rngValues[i], expRngValues[i]);
        fclose(f);
        }
#endif


        // Test without avx.
        hrtime start = getHrTime();
        rng.getRandomDoubles(rngValues,rngCount);
        hrtime stop = getHrTime();
        printf("Calculated %d norm rngs in %0.3f seconds (%0.4e rngs/second)\n",rngCount,convertHrToSeconds(stop-start),double(rngCount)/convertHrToSeconds(stop-start));
        start = getHrTime();
        rng.getExpRandomDoubles(expRngValues,rngCount);
        stop = getHrTime();
        printf("Calculated %d exp rngs in %0.3f seconds (%0.4e rngs/second)\n",rngCount,convertHrToSeconds(stop-start),double(rngCount)/convertHrToSeconds(stop-start));
        FILE* f = fopen("rng.txt", "w");
        for (int i=0; i<rngCount; i++)
            fprintf(f, "%e %e\n", rngValues[i], expRngValues[i]);
        fclose(f);

        free(rngValues);
        rngValues = NULL;
        free(expRngValues);
        expRngValues = NULL;
    }
    /**/

#ifdef OPT_AVX
    /**
      * Test the RNG limits using avx.
      */
    /**/
    {
        // Convert to double and normalize using avx.
        //long long denom = std::numeric_limits<uint32_t>::max();
        //denom += 2;
        //double newNorm = 1.0/double(denom);
        //const avxd norm = _mm256_set1_pd(newNorm);
        const avxd norm1 = _mm256_set1_pd(2.328306436538696289062500000000e-10); // 1/(2^32)
        const avxd norm2 = _mm256_set1_pd(2.328306435996595202819747782996e-10);// 1/(2^32+1)
        const avxd half = _mm256_set1_pd(0.5);
        avxi irng;
        for (int j=0; j<INT32S_PER_AVX; j++)
            if (j%2 == 0)
                ((int32_t*)&irng)[j] = std::numeric_limits<int32_t>::min();
            else
                ((int32_t*)&irng)[j] = std::numeric_limits<int32_t>::max();
        printf("Min: %d, Max: %d, Norm %0.30e\n",std::numeric_limits<int32_t>::min(),std::numeric_limits<int32_t>::max(), ((double*)&norm1)[0]);

        // Process the four lo rngs.
        __m128i irngHalf = _mm256_extractf128_si256(irng, 0);
        avxd rng = _mm256_fmadd_pd(_mm256_cvtepi32_pd(irngHalf), norm1, half);    // Range (-0.5-0.5)+0.5

        double* res = (double*)&rng;
        printf("RNG: %18.12e %18.12e %18.12e %18.12e\n", res[0], res[1], res[2], res[3]);
        printf("CMP: %d %d %d %d\n", res[0]==0.0, res[1]==1.0, res[2]>0.0, res[3]<1.0);

        // Process the four hi rngs.
        irngHalf = _mm256_extractf128_si256(irng, 1);
        rng = _mm256_fmadd_pd(_mm256_cvtepi32_pd(irngHalf), norm2, half);    // Range (-0.5-0.5)+0.5
        res = (double*)&rng;
        printf("RNG: %18.12e %18.12e %18.12e %18.12e\n", res[0], res[1], res[2], res[3]);
        printf("CMP: %d %d %d %d\n", res[0]==0.0, res[1]==1.0, res[2]>0.0, res[3]<1.0);
    }
    /**/
#endif

}










