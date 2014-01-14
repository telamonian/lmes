/*
 * University of Illinois Open Source License
 * Copyright 2008-2010 Luthey-Schulten Group,
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

#ifndef LM_MPI_H_
#define LM_MPI_H_

#include <mpi.h>
#include "lm/Exceptions.h"

void MPIErrorHandler(MPI_Comm *, int *rc, ...);

namespace lm {

/**
 * MPI exception.
 */
class MPIException : public Exception
{
public:
    int errorCode;
    MPIException(int error);
};

/**
 * Class for accessing basic MPI functions and constants.
 */
class MPI
{
public:
    static int version;
    static int subversion;
    static int threadSupport;
    static int worldSize;
    static int worldRank;
    static const int MASTER=0;

    // MPI messages.
    // replicate running messages
    static const int MSG_RUN_SIMULATION         = 1;
    static const int MSG_SIMULATION_FINISHED    = 2;
    static const int MSG_OUTPUT_DATA_STATIC     = 10;

    // replicate initialization messages
    static const int MSG_SIMULTANEOUS_REPLICATES = 26;
    static const int MSG_MSG_SIZE = 27;
    static const int MSG_SIMULATION_PARAMETERS = 28;
    static const int MSG_REACTION_MODEL = 29;
    static const int MSG_DIFFUSION_MODEL = 30;
    static const int MSG_LATTICE = 31;
    static const int MSG_LATTICE_SITES = 32;

    // thread waking messages
    static const int MSG_WAKE_LOCAL_REPLICATE_SUPERVISOR    = 96;
    static const int MSG_WAKE_REPLICATE_MANAGER = 97;
    static const int MSG_WAKE_DATA_OUTPUT_WORKER    = 98;

    static const int MSG_EXIT                   = 99;

    static const int OUTPUT_DATA_STATIC_MAX_SIZE    = 10*1024*1024;

    static void init(int argc, char** argv) throw(MPIException);
    static void printCapabilities() throw(MPIException);
    static void finalize() throw(MPIException);
};

}

#define MPI_EXCEPTION_CHECK(mpi_call) {int _mpi_ret_=mpi_call; if (_mpi_ret_ != MPI_SUCCESS) throw lm::MPIException(_mpi_ret_);}


#endif /*LM_MPI_H_*/
