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
 * Author(s): Elijah Roberts
 */

#include <iostream>
#include <string>
#include <map>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include "hrtime.h"
#include "lm/Exceptions.h"
#include "lm/Version.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/LatticeTimeSeries.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/sfile/LocalSFile.h"
#include "lm/io/sfile/SFileRecord.h"
#include "lptf/Profile.h"

void printCopyright(int argc, char** argv);
void parseArguments(int argc, char** argv);
void printUsage(int argc, char** argv);

using std::string;
using std::map;
using lm::io::hdf5::Hdf5File;
using lm::io::sfile::LocalSFile;
using lm::io::sfile::SFileRecord;

/**
 * The function to perform.
 */
string function = "";

/**
 * The sfile filename.
 */
string sfileFilename = "";

/**
 * The lm filename.
 */
string lmFilename = "";

/**
 * The record type to convert.
 */
string recordType = "";


// Allocate the profile space.
PROF_ALLOC;

int main(int argc, char** argv)
{	
    PROF_INIT;
	try
	{
		printCopyright(argc, argv);
		parseArguments(argc, argv);
		
		if (function == "help")
		{
			printUsage(argc, argv);
		}
		else if (function == "version")
		{
		}
        else if (function == "convert")
		{
            // Make sure both files exists.
            struct stat fileStats;
            if (sfileFilename == "" || stat(sfileFilename.c_str(), &fileStats) != 0)
                throw lm::Exception("The specified sfile file does not exist", sfileFilename.c_str());
            if (lmFilename == "" || stat(lmFilename.c_str(), &fileStats) != 0)
                throw lm::Exception("The specified lm file does not exist", lmFilename.c_str());

            // Open the sfile.
            LocalSFile sfile(sfileFilename);
            sfile.openRead();

            // Open the lm file.
            Hdf5File lmfile(lmFilename);

            // Read each sfile record.
            hrtime startTime=getHrTime();
            int64_t recordsProcessed=0;
            int64_t recordsConverted=0;
            while (!sfile.isEof())
            {
                SFileRecord record = sfile.readNextSFileRecord();
                recordsProcessed++;

                // Process the record.
                if (recordType == "" || recordType == record.type)
                {
                    // Read the data.
                    unsigned char* data = new unsigned char[record.dataSize];
                    sfile.readFully(data, record.dataSize);

                    if (record.type == "protobuf:lm.io.FirstPassageTimes")
                    {
                        lm::io::FirstPassageTimes msg;
                        if (!msg.ParseFromArray(data, record.dataSize)) throw lm::Exception("Unable to deserialize FirstPassageTimes record");
                        lmfile.setFirstPassageTimes(msg.trajectory_id(), msg);
                    }
                    else if (record.type == "protobuf:lm.io.SpeciesCounts")
                    {
                        lm::io::SpeciesCounts msg;
                        if (!msg.ParseFromArray(data, record.dataSize)) throw lm::Exception("Unable to deserialize SpeciesCounts record");
                        lmfile.appendSpeciesCounts(msg.trajectory_id(), (lm::io::SpeciesCounts*)&msg);
                    }
                    else if (record.type == "protobuf:lm.io.LatticeTimeSeries")
                    {
                        lm::io::LatticeTimeSeries msg;
                        if (!msg.ParseFromArray(data, record.dataSize)) throw lm::Exception("Unable to deserialize LatticeTimeSeries record");
                        lmfile.appendLatticeTimeSeries(msg.trajectory_id(), msg);
                    }
                    else
                    {
                        printf("WARNING: Unknown record name=%s type=%s size=%lld\n",record.name.c_str(),record.type.c_str(),(long long int)record.dataSize);
                    }
                    delete[] data;
                    recordsConverted++;
                }
                else
                {
                    // Skip the data.
                    sfile.skip(record.dataSize);
                }

                if (recordsProcessed%1000 == 0)
                    printf("Record read: %lld, converted: %lld\n", (long long int)recordsProcessed, (long long int)recordsConverted);
            }

            // Close the files.
            lmfile.close();
            sfile.close();
            printf("Processed %lld record in %0.3f seconds.\n", (long long int)recordsProcessed, convertHrToSeconds(getHrTime()-startTime));
		}
		else
		{
			throw lm::CommandLineArgumentException("unknown function.");
		}
		return 0;
	}
    catch (lm::CommandLineArgumentException e)
    {
    	std::cerr << "Invalid command line argument: " << e.what() << std::endl << std::endl;
        printUsage(argc, argv);
    }
    catch (lm::Exception e)
    {
    	std::cerr << "Exception during execution: " << e.what() << std::endl;
    }
    catch (std::exception e)
    {
    	std::cerr << "Exception during execution: " << e.what() << std::endl;
    }
    catch (...)
    {
    	std::cerr << "Unknown Exception during execution." << std::endl;
    }
    return -1;
}

/**
 * This function prints the copyright notice.
 */
void printCopyright(int argc, char** argv) {

	std::cout << argv[0] << " v" << VERSION_NUM << " build " << BUILD_INFO << std::endl;
    std::cout << "Copyright (C) " << COPYRIGHT_DATE_JHU << " Roberts Group, Johns Hopkins University." << std::endl << std::endl;
	std::cout << std::endl;
}

/**
 * Parses the command line arguments.
 */
void parseArguments(int argc, char** argv)
{
    //Parse any arguments.
    for (int i=1; i<argc; i++)
    {
        char *option = argv[i];
        while (*option == ' ') option++;
        
        //See if the user is trying to get help.
        if (strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
        	function = "help";
        	break;
        }
        
        //See if the user is trying to get the version info.
        else if (strcmp(option, "-v") == 0 || strcmp(option, "--version") == 0) {
        	function = "version";
        	break;
        }
            
        //See if the user is trying to specify a filename.
        else if (i == 1 && argc >= 3)
        {
            function = "convert";
            sfileFilename = option;
        }
        
        //See if the user is trying to specify a species count.
        else if (i == 2 && argc >= 3)
        {
            lmFilename = option;
        }

        //See if the user is trying to specify a record type.
        else if (i == 3)
        {
            recordType = option;
        }


             
        //This must be an invalid option.
        else {
            throw lm::CommandLineArgumentException(option);
        }
    }
}

/**
 * Prints the usage for the program.
 */
void printUsage(int argc, char** argv)
{
	std::cout << "Usage: " << argv[0] << " (-h|--help)" << std::endl;
	std::cout << "Usage: " << argv[0] << " (-v|--version)" << std::endl;
    std::cout << "Usage: " << argv[0] << " sfile_filename lm_filename [record_type]" << std::endl;
	std::cout << std::endl;
}
