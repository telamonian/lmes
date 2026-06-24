/*
 * University of Illinois Open Source License
 * Copyright 2008-2012 Luthey-Schulten Group,
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 *               University of Illinois at Urbana-Champaign
 *               http://www.scs.uiuc.edu/~schulten
 * 
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
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
 * Author(s): Elijah Roberts
 */

#ifdef OPT_MPI

#ifndef LM_MPI_H_
#define LM_MPI_H_

#include <mpi.h>
#include "lm/Exceptions.h"

#define MPI_EXCEPTION_CHECK(mpi_call) {int _mpi_ret_=mpi_call; if (_mpi_ret_ != MPI_SUCCESS) throw lm::MPIException(_mpi_ret_);}

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
    static const int MSG_EXIT                   = 99;

    static const int OUTPUT_DATA_STATIC_MAX_SIZE    = 10*1024*1024;

    static void init(int argc, char** argv) throw(MPIException);
    static void printCapabilities() throw(MPIException);
    static void finalize(bool abort=false) throw(MPIException);

    //send from master node to all nodes, one by one, including master. nodes should MPI_Recv plus the relevant tag to receive
    static void MastBcastOut(void *buf, int count, MPI_Datatype datatype, int tag, MPI_Comm comm);

    //receive from all nodes, one by one, including master. nodes should use MPI_Send plus the relevant tag to send
    template <typename t>
    static void MastBcastIn(t * recvtable, int recvcount, MPI_Datatype recvtype, int recvtag, MPI_Comm comm)
    {
        MPI_Status messageStatus;
        for(int sendProc=0; sendProc < lm::MPI::worldSize; ++sendProc)
        {
            MPI_EXCEPTION_CHECK(MPI_Recv(recvtable + sendProc, recvcount, recvtype, sendProc, recvtag, comm, &messageStatus));
        }
    }

};

}
#endif /*LM_MPI_H_*/

#endif
