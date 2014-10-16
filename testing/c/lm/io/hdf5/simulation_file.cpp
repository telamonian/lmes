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

class SimulationFileFixture : public ::testing::Test
{
public:
    SimulationFileFixture(): file("/Users/tel/git/lm/test_case/fflux_test/biphasic_switch.lm") {}
    lm::io::hdf5::Hdf5File file;
    lm::io::OrderParameters ops;
};

TEST_F(SimulationFileFixture, ReadOrderParameters)
{
    if (file.hasOrderParameters())
    {
        file.getOrderParameters(&ops);
    }
    ASSERT_EQ(ops.order_parameters(0).id(), 0);
    ASSERT_EQ(ops.order_parameters(0).type(), 0);
    ASSERT_EQ(ops.order_parameters(0).species_id(1), 2);
    ASSERT_EQ(ops.order_parameters(0).species_id(4), 5);
    ASSERT_DOUBLE_EQ(ops.order_parameters(0).species_coefficient(1), 2.0);
    ASSERT_DOUBLE_EQ(ops.order_parameters(0).species_coefficient(3), -1.0);
}

TEST_F(SimulationFileFixture, ReadWriteReadOrderParameters)
{
    if (file.hasOrderParameters())
    {
        file.getOrderParameters(&ops);
        file.setOrderParameters(&ops);
        file.getOrderParameters(&ops);
    }
    ASSERT_EQ(ops.order_parameters(0).id(), 0);
    ASSERT_EQ(ops.order_parameters(0).type(), 0);
    ASSERT_EQ(ops.order_parameters(0).species_id(1), 2);
    ASSERT_EQ(ops.order_parameters(0).species_id(4), 5);
    ASSERT_DOUBLE_EQ(ops.order_parameters(0).species_coefficient(1), 2.0);
    ASSERT_DOUBLE_EQ(ops.order_parameters(0).species_coefficient(3), -1.0);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
