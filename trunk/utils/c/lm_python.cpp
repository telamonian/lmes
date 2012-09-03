/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 *               University of Illinois at Urbana-Champaign
 *               http://www.scs.uiuc.edu/~schulten
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

#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <google/protobuf/stubs/common.h>
#include <Python.h>
#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/Version.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lptf/Profile.h"

using std::string;
using std::list;
using lm::Print;
using lm::io::hdf5::SimulationFile;

/**
 * The function being performed.
 */
string function = "";

/**
 * The script filename being executed, if applicable.
 */
string scriptFilename = "";

/**
 * The arguments for the script, if applicable.
 */
list<string> scriptArguments;

/**
 * The directory containing the supporting files.
 */
string libDir;

/**
 * The directory containing the supporting files.
 */
string userLibDir;

void printCopyright(int argc, char** argv);
void parseArguments(int argc, char** argv);
void printUsage(int argc, char** argv);
void discoverEnvironment();
void initPython();
void finalizePython();
void startInterpreter();
void executeScript(string filename, list<string> arguments);


// Allocate the profile space.
PROF_ALLOC;

int main(int argc, char** argv)
{
    // Make sure we are using the correct protocol buffers library.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

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
        else if (function == "script")
        {
            discoverEnvironment();
            executeScript(scriptFilename, scriptArguments);
        }
        else if (function == "interpreter")
        {
            discoverEnvironment();
            startInterpreter();
        }
        else
        {
            throw lm::CommandLineArgumentException("unknown function.");
        }
        return 0;
    }
    catch (lm::CommandLineArgumentException e)
    {
        Print::printf(Print::FATAL, "Invalid command line argument: %s\n", e.what());
        printUsage(argc, argv);
    }
    catch (lm::Exception e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s", e.what());
    }
    catch (std::exception e)
    {
        Print::printf(Print::FATAL, "Exception during execution: %s", e.what());
    }
    catch (...)
    {
        Print::printf(Print::FATAL, "Unknown exception during execution.");
    }
    return -1;
}

/**
 * This function prints the copyright notice.
 */
void printCopyright(int argc, char** argv) {

    std::cout << argv[0] << " v" << VERSION_NUM << " build " << BUILD_INFO << std::endl;
    std::cout << "Copyright (C) " << COPYRIGHT_DATE << " Luthey-Schulten Group," << std::endl;
    std::cout << "University of Illinois at Urbana-Champaign." << std::endl;
    std::cout << std::endl;
}

/**
 * Parses the command line arguments.
 */
void parseArguments(int argc, char** argv)
{
    // Set any default options.
    function = "interpreter";
    scriptFilename = "";
    bool parsingScriptArgs = false;
    scriptArguments.clear();

    //Parse any arguments.
    for (int i=1; i<argc; i++)
    {
        char *option = argv[i];
        while (*option == ' ') option++;

        //See if the user is trying to get help.
        if (!parsingScriptArgs && strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
            function = "help";
            break;
        }

        //See if the user is trying to get the version info.
        else if (!parsingScriptArgs && strcmp(option, "-v") == 0 || strcmp(option, "--version") == 0) {
            function = "version";
            break;
        }

        //See if the user is trying to execute a script.
        else if (!parsingScriptArgs && strcmp(option, "-s") == 0 || strcmp(option, "--script") == 0)
        {
            function = "script";

            // Get the filename.
            if (i < argc-1)
                scriptFilename = argv[++i];
            else
                throw lm::CommandLineArgumentException("missing script filename.");
        }
        else if (!parsingScriptArgs && function == "script" && (strcmp(option, "-sa") == 0 || strcmp(option, "--script-args") == 0))
        {
            parsingScriptArgs = true;
        }
        else if (parsingScriptArgs)
        {
            scriptArguments.push_back(option);
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
    std::cout << "Usage: " << argv[0] << " [OPTIONS]" << std::endl;
    std::cout << "Usage: " << argv[0] << " [OPTIONS] (-s|--script) script_filename [(-sa|--script-args) script_arguments+]" << std::endl;
    std::cout << std::endl;
}


/**
 * Figures out the environment for the program (directories, files, etc).
 */
void discoverEnvironment()
{
    char* env;
    struct stat fileStats;

    // See if we have a lib directory as an environment variable.
    if ((env=getenv("LMLIBDIR")) != NULL && stat((string(env)+"/"+"lm.py").c_str(), &fileStats) == 0 && S_ISREG(fileStats.st_mode))
    {
        libDir = env;
    }

    // Otherwise, see if we can find the lib directory based a a few guesses.
    else if (stat("/usr/local/lib/lm/lm.py", &fileStats) == 0 && S_ISREG(fileStats.st_mode))
    {
        libDir = "/usr/local/lib/lm/";
    }
    else if (stat("/usr/lib/lm/lm.py", &fileStats) == 0 && S_ISREG(fileStats.st_mode))
    {
        libDir = "/usr/lib/lm/";
    }

    // Without a lib directory we can't continue.
    else
    {
        throw lm::Exception("Could not find installation lib directory, please set the LMLIBDIR environment variable appropriately.");
    }

    // See if we have any user paths as an environment variable.
    if ((env=getenv("LMPATH")) != NULL)
    {
        userLibDir = env;
    }
}

/**
 * Starts the python interpreter.
 */
extern "C" void init_lm(void);

void initPython()
{
	Py_Initialize();

	// Add the python modules to the search path.
	string pythonPath=libDir+":"+Py_GetPath();
	if (userLibDir != "")
		pythonPath += ":"+userLibDir;
	pythonPath += ":.";
	
	char* pythonPathCopy = new char[pythonPath.length()+1];
	memset(pythonPathCopy, 0, pythonPath.length()+1);
	pythonPath.copy(pythonPathCopy, pythonPath.length()+1);
	PySys_SetPath(pythonPathCopy);
	delete[] pythonPathCopy;
	
	// Initialize the swig module.
	init_lm();
	FILE* fp = fopen((libDir+"/"+"lm.py").c_str(),"r");
	if (fp == NULL)
		throw lm::Exception("Failed to open lm.py file.");
	if (PyRun_SimpleFile(fp, "lm.py"))
		throw lm::Exception("Failed to run lm.py file.");
	if (fclose(fp))
		throw lm::Exception("Failed to close lm.py file.");
	fp = NULL;
}

void finalizePython()
{
	Py_Finalize();	
}

void startInterpreter()
{
	initPython();
	
	// Start the interpreter.
	if (PyRun_InteractiveLoop(stdout, "Lattice Microbe"))
		throw lm::Exception("Python failed");
	
	finalizePython();
}

void executeScript(string filename, std::list<string> arguments)
{
	initPython();
	Print::printf(Print::DEBUG, "Init python.");
	
	// Allocate a buffer for the arguments.
	int argc = arguments.size()+1;
	char** argv = new char*[argc];
	if (argv == NULL)
		throw lm::Exception("Failed to allocate memory for the script arguments.");
	
	// Copy the script name into the first argument.
	int argIndex=0;
	argv[argIndex] = new char[filename.size()+1];
	if (argv[argIndex] == NULL) throw lm::Exception("Failed to allocate memory for the script arguments.");
	strcpy(argv[argIndex], filename.c_str());
	argIndex++;
		
	// Copy the rest of the arguments into the buffer.
	for (std::list<string>::iterator it=arguments.begin(); it != arguments.end(); it++, argIndex++)
	{
		string arg = *it;
		argv[argIndex] = new char[arg.size()+1];
		if (argv[argIndex] == NULL) throw lm::Exception("Failed to allocate memory for the script arguments.");
		strcpy(argv[argIndex], arg.c_str());
	}
    Print::printf(Print::DEBUG, "Copied args.");
		
	// Set the arguments.
	PySys_SetArgv(argc, argv);
	Print::printf(Print::DEBUG, "Set args.");
	
	// Open the script.
	std::ifstream scriptFile(filename.c_str());
	scriptFile.exceptions(std::ifstream::eofbit|std::ifstream::failbit|std::ifstream::badbit);
	Print::printf(Print::DEBUG, "Opened script.");
	
	// Figure out how long the script is.
	scriptFile.seekg(0, std::ios::end);
	std::streampos length = scriptFile.tellg();
	scriptFile.seekg(0, std::ios::beg);
	Print::printf(Print::DEBUG, "Got script length %d.", (int)length);
	
	// Allocate a buffer for the script data.
	char * scriptData = new char[(int)length+1];
	memset(scriptData, 0, (int)length+1);
	if (scriptData == NULL)
	{
		scriptFile.close();
		throw lm::Exception("Could not allocate script buffer.");
	}
	Print::printf(Print::DEBUG, "Alloctaed buffer.");
	
	// Read the script data.
	scriptFile.read(scriptData, length);
	Print::printf(Print::DEBUG, "Read script.");
	
	// Close the script.
	scriptFile.close();
	Print::printf(Print::DEBUG, "Closed script.");
	
	/*// Set the replicate as a variable.
	PyObject* module = PyImport_AddModule("__main__");
	PyObject* dictionary = PyModule_GetDict(module);
	PyObject* pyReplicate = PyInt_FromLong(replicate);
	PyDict_SetItemString(dictionary, "replicate", pyReplicate);
	Py_DECREF(pyReplicate);
	*/

	// Run the script.
	if (PyRun_SimpleString(scriptData))
		throw lm::Exception("Failed to run script.");
	Print::printf(Print::DEBUG, "Finished running script.");
	
	// Free the script data buffer.
	delete[] scriptData;
	scriptData = NULL;
	
	// Stop the python environment.
	finalizePython();
	
	// Free the command line argument storage.
	if (argv != NULL)
	{
		for (int i=0; i<argc; i++)
		{
			if (argv[i] != NULL)
			{
				delete[] argv[i];
				argv[i] = NULL;
			}
		}
		delete[] argv;
		argv = NULL;
		argc = 0;
	}
}
