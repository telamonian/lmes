/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
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
#undef MPI_EXCEPTION_CHECK
#define MPI_EXCEPTION_CHECK(x) {}

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "lm/main/Main.h"
#include "lm/main/ReplicateSupervisor.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/Print.h"
#include <vector>

#define MPI_EXCEPTION_CHECK(x) {}

using lm::main::ReplicateSupervisor;
using lm::io::hdf5::SimulationFile;
using lm::io::hdf5::Hdf5File;

Hdf5File * fi = NULL;

class MockSimulationFile : public SimulationFile
{
};



class SupervisorFixture : public ::testing::Test
{
public:
    SupervisorFixture():
    file(),
    supervisor(NULL, fi)
    {
        for (int i=0;i<4;++i)
        {
            replicates.push_back(i);
        }
        for (vector<int>::iterator it=replicates.begin(); it<replicates.end(); it++)
        {
            supervisor.simulationStatusTable[*it] = 0;
        }
    }

    MockSimulationFile file;
    ReplicateSupervisor supervisor;
};

//class FooFixture : public ::testing::Test {
//    public:
//
//}

TEST_F(SupervisorFixture, FindRep_StartsReplicateAtFirstZero)
{
    supervisor.simulationStatusTable[0] = 2;
    supervisor.simulationStatusTable[1] = 2;
    supervisor.simulationStatusTable[3] = 2;
    ASSERT_EQ(supervisor.FindRep(0), 2);
}

//TEST(foo, bar)
//{
//    lm::Print::printf(0, "hey");
//    int bob=19;
//    int sam=18;
//    EXPECT_EQ(sam,bob);
//}
//
//TEST(foo, doh)
//{
//    lm::Print::printf(0, "hey");
//    int bob=19;
//    int sam=19;
//    EXPECT_EQ(sam,bob);
//}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
