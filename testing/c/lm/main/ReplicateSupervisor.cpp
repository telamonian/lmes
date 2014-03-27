/*
 * LocalReplicateSupervisor.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: tel
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
