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
#include "lm/io/Tilings.pb.h"
#include "lm/tiling/Tiling.h"

class TilingFixture : public ::testing::Test
{
public:
    TilingFixture(): file("/Users/tel/git/lm/gtest/data/lm/fflux/biphasic_switch.lm")
    {
        file.getTilings(&tilingsBuf);
        tAX.init(tilingsBuf.tilings(0));
    }
    lm::io::hdf5::Hdf5File file;
    lm::io::Tilings tilingsBuf;
    lm::tiling::TilingLattice tAX;
};

TEST_F(TilingFixture, GetEdge)
{
    ASSERT_DOUBLE_EQ(tAX.getEdge(1), -20.833333969116211);
    ASSERT_DOUBLE_EQ(tAX.getEdge(4), -8.3333330154418945);
    ASSERT_DOUBLE_EQ(tAX.getEdge(10), 16.666666030883789);
}

TEST_F(TilingFixture, GetEdgesCount)
{
    ASSERT_EQ(tAX.getEdgesCount(), 13);
}

//int main(int argc, char **argv) {
//  ::testing::InitGoogleTest(&argc, argv);
//  return RUN_ALL_TESTS();
//}
