/*
 * LocalReplicateSupervisor.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: tel
 */
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/io/Tilings.pb.h"
#include "lm/oparam/OParam.h"

class OParamFixture : public ::testing::Test
{
public:
    OParamFixture(): file("/Users/tel/git/lm/test_case/fflux_test/biphasic_switch.lm")
    {
        file.getOrderParameters(&opBuf);
        opL.init(opBuf.order_parameters(0));
    }
    static uint speciesCounts[7];
    lm::io::hdf5::Hdf5File file;
    lm::io::OrderParameters opBuf;
    lm::oparam::OParamLinear opL;
};

uint OParamFixture::speciesCounts[7] = {4,16,1,0,0,0,0};

TEST_F(OParamFixture, InitLinearOparam)
{
    ASSERT_EQ(opL.speciesID[0], 0);
    ASSERT_EQ(opL.speciesID[1], 1);
    ASSERT_EQ(opL.speciesID[2], 2);
    ASSERT_DOUBLE_EQ(opL.speciesCoefficient[0], 1.0);
    ASSERT_DOUBLE_EQ(opL.speciesCoefficient[1], 2.0);
    ASSERT_DOUBLE_EQ(opL.speciesCoefficient[2], 2.0);
}

TEST_F(OParamFixture, CalcLinearOparam)
{
    ASSERT_DOUBLE_EQ(opL.calc(speciesCounts), 38);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
