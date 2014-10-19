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
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/io/Tilings.pb.h"

class SimulationFileFixture : public ::testing::Test
{
public:
    SimulationFileFixture(): file("/Users/tel/git/lm/gtest/data/lm/fflux/biphasic_switch.lm") {}
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

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
