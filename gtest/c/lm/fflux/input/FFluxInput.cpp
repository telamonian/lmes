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
#include "lm/gtest.h"

#include <stdio.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "lm/fflux/input/FFluxInput.h"
#include "lm/fflux/input/FFluxStage.pb.h"

#include <string>
#include <vector>

using std::string;
using std::vector;

const char* filenamesLiteralFFluxInputs[] = {"/Users/tel/git/lm_ndarray/gtest/c/lm/fflux/input/ffluxSimulationInput.sfile"};
const vector<string> filenamesFFluxInputs(filenamesLiteralFFluxInputs, filenamesLiteralFFluxInputs+1);

const int speciesCountsLiteral[] = {0,1,2,3,4,5,6};
const vector<int> speciesCounts(speciesCountsLiteral, speciesCountsLiteral+7);

const double timesLiteral[] = {0.0};
const vector<double> times(timesLiteral, timesLiteral+7);

class FFluxInputFixture : public ::testing::Test
{
public:
    FFluxInputFixture(): ffluxInput(filenamesFFluxInputs)
    {
        char cwd[FILENAME_MAX];
        getcwd(cwd, sizeof(cwd));

        printf("current working directory: %s\n", cwd);
    }

    lm::fflux::input::FFluxInput ffluxInput;
};

TEST_F(FFluxInputFixture, readSFileInput_test)
{
    // make some references to the innards of the FFluxSimulationInput msg for easy access
    const lm::fflux::input::FFluxStage& ffluxStage = ffluxInput.ffluxSimulationInput().fflux_stage_list().fflux_stages(0);
    const lm::fflux::input::FFluxPhase& ffluxPhase = ffluxStage.fflux_phases(0);
    const lm::fflux::input::FFluxPhaseLimit&  ffluxPhaseLimit = ffluxPhase.fflux_phase_limit();
    const lm::fflux::io::EndPoint& ffluxPhaseStartPoint = ffluxPhase.start_points(0);
    
    // test some scalar values in ffluxStage
    EXPECT_EQ(0, ffluxStage.tiling_id());
    EXPECT_EQ(0, ffluxStage.basin_index());

    // test some scalar values in ffluxPhase
    EXPECT_EQ(0, ffluxPhase.tiling_id());
    EXPECT_EQ(0, ffluxPhase.basin_index());
    EXPECT_EQ(4, ffluxPhase.fflux_phase_index());

    // test some scalar values in ffluxPhaseStartPoint
    EXPECT_EQ(1, ffluxPhaseStartPoint.count());

    // test some vector/repeated values in ffluxPhaseStartPoint
    int countSize = ffluxPhaseStartPoint.species_coordinates_size();
    for (int i=0; i<countSize; i++)
    {
        int valExpected = speciesCounts[i];
        int valActual = ffluxPhaseStartPoint.species_coordinates(i);

        EXPECT_EQ(valExpected, valActual);
    }

    int timesSize = ffluxPhaseStartPoint.times_size();
    for (int i=0; i<timesSize; i++)
    {
        double valExpected = times[i];
        double valActual = ffluxPhaseStartPoint.times(i);

        EXPECT_NEAR(valExpected, valActual, absolute_tolerance);
    }
}
