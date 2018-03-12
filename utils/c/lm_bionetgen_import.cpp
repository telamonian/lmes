/*
 * Copyright 2017 Johns Hopkins University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
 *
 * Author(s): Elijah Roberts
 */

#include <iostream>
#include <string>
#include <sys/stat.h>

#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/Version.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lptf/Profile.h"
#include "robertslab/bngl/BNGLImporter.h"

using std::string;
using lm::Exception;
using lm::input::ReactionModel;
using lm::io::hdf5::Hdf5File;
using lm::Print;

void printCopyright(int argc, char** argv);
void parseArguments(int argc, char** argv);
void printUsage(int argc, char** argv);
void importBNGLModel(string lmFilename, string bnglFilename);

/**
 * The function being performed.
 */
string function = "";

/**
 * The lm file to output the model into.
 */
string outputFilename = "";

/**
 * The sbml file to import the model from.
 */
string inputFilename = "";

/**
 * Whether to print out more verbose messages.
 */
bool verbose = false;
bool reallyVerbose = false;

/**
 * If rate constants are given in terms of concentrations.
 */
bool constantsUseConcentrations = false;

/**
 * Parameters specified by the user.
 */
map<string,double> userParameters;


// Allocate the profile space.
PROF_ALLOC;

int main(int argc, char** argv)
{	
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
		else if (function == "import")
		{
            // Import the file.
            importBNGLModel(outputFilename, inputFilename);
        }
		else
		{
			throw lm::CommandLineArgumentException("unknown function.");
		}
		return 0;
	}
    catch (lm::CommandLineArgumentException e)
    {
        std::cout << "Invalid command line argument: " << e.what() << std::endl << std::endl;
        printUsage(argc, argv);
    }
    catch (Exception e)
    {
        std::cout << "Exception during execution: " << e.what() << std::endl;
    }
    catch (std::exception e)
    {
        std::cout << "Exception during execution: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "Unknown Exception during execution." << std::endl;
    }
    return -1;
}

void importBNGLModel(string lmFilename, string bnglFilename)
{
    // Make sure the input file exists.
    struct stat fileStats;
    if (stat(bnglFilename.c_str(), &fileStats) != 0)
    {
        throw Exception("The specified BNGL file does not exist", bnglFilename.c_str());
    }

    // Create the appropriate importer instance.
    robertslab::bngl::BNGLImporter* importer = new robertslab::bngl::BNGLImporter();

    // Import the document.
    importer->setOptions(constantsUseConcentrations, verbose, reallyVerbose);
    bool success = importer->import(bnglFilename, userParameters);
    if (success)
    {
        // If the output file doesn't exist, create it.
        if (stat(outputFilename.c_str(), &fileStats) != 0)
        {
            Hdf5File::create(outputFilename);
        }

        // Open the file.
        Hdf5File outputFile(outputFilename);

        // Set the reaction model.
        outputFile.setReactionModel(importer->getReactionModel());

        // Close the file.
        outputFile.close();

        Print::printf(Print::INFO, "Import completed successfully.");
    }
    else
    {
        Print::printf(Print::ERROR, "There were errors during the import process, the output file was not generated.");
    }

    // Free the importer.
    if (importer != NULL) delete importer;
}


/**
 * This function prints the copyright notice.
 */
void printCopyright(int argc, char** argv)
{
	std::cout << argv[0] << " v" << VERSION_NUM << " build " << BUILD_INFO << std::endl;
    std::cout << "Copyright (C) " << COPYRIGHT_DATE_JHU << " Roberts Group, Johns Hopkins University." << std::endl;
	std::cout << std::endl;
}

/**
 * Parses the command line arguments.
 */
void parseArguments(int argc, char** argv)
{
    function = "import";
    outputFilename = "";
    inputFilename = "";

    // Parse any arguments.
    for (int i=1; i<argc; i++)
    {
        char *option = argv[i];
        while (*option == ' ') option++;
        
        // See if the user is trying to get help.
        if (strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
        	function = "help";
        	break;
        }
        
        // See if the user is trying to get the version info.
        else if (strcmp(option, "-v") == 0 || strcmp(option, "--version") == 0) {
        	function = "version";
        	break;
        }

        // See if the user wants verbose messages.
        else if (strcmp(option, "--verbose") == 0)
        {
            if (!verbose)
                verbose = true;
            else
                reallyVerbose = true;
        }

        // See if the user is trying to import a sbml file that was originally exported by COPASI.
        else if (strcmp(option, "--constants-use-amounts") == 0)
        {
            constantsUseConcentrations = false;
        }

        // See if the user is trying to import a sbml file that was originally exported by COPASI.
        else if (strcmp(option, "--constants-use-concentration") == 0)
        {
            constantsUseConcentrations = true;
        }

        // See if the user is trying to specify a parameter key value pair.
        else if (strstr(option, "=") != NULL)
        {
            char * separator=strstr(option, "=");
            if (separator != NULL && separator > option && strlen(separator) > 1)
            {
                string key(option, separator-option);
                userParameters[key] = atof(separator+1);
            }
        }

        // See if the user is trying to specify an output filename.
        else if (outputFilename == "")
        {
            outputFilename = option;
        }

        // See if the user is trying to specify an input filename
        else if (inputFilename == "")
        {
            inputFilename = option;
        }

        // This must be an invalid option.
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
    std::cout << "Usage: " << argv[0] << " [OPTIONS] lm_filename bngl_filename" << std::endl;
	std::cout << std::endl;
    std::cout << "OPTIONS" << std::endl;
    std::cout << "  key=double_value                Specify a new or override an existing parameter in the BNGL file." << std::endl;
    std::cout << "  --verbose                       This option causes more detailed error information to be printed. Use twice for debugging messages." << std::endl;
    std::cout << "  --constants-use-amounts         Specify that rate constants are given in terms of particle counts." << std::endl;
    std::cout << "  --constants-use-concentration   Specify that rate constants are given in terms of concentrations." << std::endl;

    std::cout << std::endl;
}

