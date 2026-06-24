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
 * Author(s): Max Klein
 */
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "lm/fflux/FFluxMath.h"

class FFluxMathFixture : public ::testing::Test
{
public:
    FFluxMathFixture()
    {
    }
};

TEST_F(FFluxMathFixture, normalZ_test)
{
    double absolute_tolerance = 1e-10;

    // test values all taken from equivalent Mathematica function
    EXPECT_NEAR(normalZ(.1), 0.1256613468550741, absolute_tolerance);
    EXPECT_NEAR(normalZ(.5), 0.6744897501960818, absolute_tolerance);
    EXPECT_NEAR(normalZ(.90), 1.644853626951472, absolute_tolerance);
    EXPECT_NEAR(normalZ(.95), 1.959963984540054, absolute_tolerance);
    EXPECT_NEAR(normalZ(.99), 2.5758293035489, absolute_tolerance);
    EXPECT_NEAR(normalZ(.9999), 3.89059188641312, absolute_tolerance);
}
