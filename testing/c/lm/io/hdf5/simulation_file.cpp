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

class SimulationFileFixture : public ::testing::Test
{
public:
    SimulationFileFixture(): file("/Users/tel/git/lm/test_case/fflux_test/biphasic_switch.lm") {}
    lm::io::hdf5::Hdf5File file;
    lm::io::OrderParameters ops;
    lm::io::Tilings tngs;
};

TEST_F(SimulationFileFixture, ReadOrderParameters)
{
    if (file.hasOrderParameters())
    {
        file.getOrderParameters(&ops);
    }
    ASSERT_EQ(ops.order_parameters(0).id(), 0);
    ASSERT_EQ(ops.order_parameters(0).type(), 0);
    ASSERT_EQ(ops.order_parameters(0).species_id(1), 1);
    ASSERT_EQ(ops.order_parameters(0).species_id(4), 4);
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
    ASSERT_EQ(ops.order_parameters(0).species_id(1), 1);
    ASSERT_EQ(ops.order_parameters(0).species_id(4), 4);
    ASSERT_DOUBLE_EQ(ops.order_parameters(0).species_coefficient(1), 2.0);
    ASSERT_DOUBLE_EQ(ops.order_parameters(0).species_coefficient(3), -1.0);
}

TEST_F(SimulationFileFixture, ReadTilings)
{
    ASSERT_EQ(file.hasTilings(), true);
    file.getTilings(&tngs);
    ASSERT_EQ(tngs.tilings(0).id(), 0);
    ASSERT_EQ(tngs.tilings(0).type(), 0);
    ASSERT_EQ(tngs.tilings(0).order_parameter_id(), 0);
    ASSERT_DOUBLE_EQ(tngs.tilings(0).edges(1), -20.833333969116211);
    ASSERT_DOUBLE_EQ(tngs.tilings(0).edges(10), 16.666666030883789);
    ASSERT_EQ(tngs.tilings(0).arrangement(), lm::io::Tilings::ASCENDING);
}

TEST_F(SimulationFileFixture, ReadWriteReadTilings)
{
    ASSERT_EQ(file.hasTilings(), true);
    file.getTilings(&tngs);
    file.setTilings(&tngs);
    file.getTilings(&tngs);
    ASSERT_EQ(tngs.tilings(0).id(), 0);
    ASSERT_EQ(tngs.tilings(0).type(), 0);
    ASSERT_EQ(tngs.tilings(0).order_parameter_id(), 0);
    ASSERT_DOUBLE_EQ(tngs.tilings(0).edges(1), -20.833333969116211);
    ASSERT_DOUBLE_EQ(tngs.tilings(0).edges(10), 16.666666030883789);
    ASSERT_EQ(tngs.tilings(0).arrangement(), lm::io::Tilings::ASCENDING);
}

//int main(int argc, char **argv) {
//  ::testing::InitGoogleTest(&argc, argv);
//  return RUN_ALL_TESTS();
//}
