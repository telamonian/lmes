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

#include <google/protobuf/repeated_field.h>
#include <stdio.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "lm/fflux/FFluxPhaseZeroTrajectory.h"
#include "lm/fflux/input/FFluxInput.h"
#include "lm/fflux/input/FFluxStage.pb.h"
#include "lm/io/sfile/LocalSFile.h"
#include "lm/Math.h"

#include <string>
#include <vector>
#include <lm/fflux/old_FFluxTrajectory.h>

using std::string;
using std::vector;

// tolerance for equality testing of doubles
double absolute_tolerance = 1e-10;

const char* filenamesLiteral[] = {"genetic_toggle_switch_-_FFluxPhaseZeroDwellTimes.sfile"};
const vector<string> filenames(filenamesLiteral, filenamesLiteral+1);

const int speciesCountsLiteral[] = {0,1,2,3,4,5,6};
const vector<int> speciesCounts(speciesCountsLiteral, speciesCountsLiteral+7);

const double timesLiteral[] = {0.0};
const vector<double> times(timesLiteral, timesLiteral+7);

class LimitTrackingsLoader
{
public:
    LimitTrackingsLoader(const vector<string>& filenames): filenames(filenames)
    {
        for (int i=0; i<filenames.size(); i++)
        {
            // See if the file is an SFile.
            lm::io::sfile::LocalSFile sfile(filenames[i]);
            if(sfile.exists() && sfile.isFile() && sfile.isSFile())
            {
                // Read the input from the sfile.
                sfile.openRead();
                sfile.readAllMessages(&limitTrackings);
                sfile.close();
            }
        }
    }

    vector<string> filenames;
    google::protobuf::RepeatedPtrField<lm::io::LimitTracking> limitTrackings;
};

class FFluxPhaseZeroTrajectoryFixture: public ::testing::Test
{
public:
    FFluxPhaseZeroTrajectoryFixture(): ltLoader(filenames)
    {
        char cwd[FILENAME_MAX];
        getcwd(cwd, sizeof(cwd));

        printf("current working directory: %s\n", cwd);
    }

    LimitTrackingsLoader ltLoader;
//    lm::fflux::FFluxPhaseZeroTrajectory ffluxPhaseZeroTrajectory;

    lm::protowrap::Repeated<lm::io::LimitTracking> limitTrackingsWrap;
    lm::protowrap::NDArray<double> timeWrapForwardFlux, timeWrapBasinEntry, timeWrapBasinExit;

    // streaming variance of the waiting time in between interface 0 forward crossing events
    StreamingVariance waitingTimeSV;
    WaitingTimeScratchpad waitingTimeScratchpad;
};

TEST_F(FFluxPhaseZeroTrajectoryFixture, getDwellTimes_test)
{
    // set wrapper on the limit_trackings field
    limitTrackingsWrap.setWrappedField(ltLoader.limitTrackings);

    int i = 0;
    for (;i<limitTrackingsWrap.size();i++)
    {

    }

    // set wrappers on the ndarrays with the limit-triggering times
    timeWrapForwardFlux.setWrappedMsg(limitTrackingsWrap.Get(0).times());
    timeWrapBasinEntry.setWrappedMsg(limitTrackingsWrap.Get(1).times());
    timeWrapBasinExit.setWrappedMsg(limitTrackingsWrap.Get(2).times());

    // If the trajectory was previously in a non-initial basin, or if it passed into a non-initial basin during this work unit, accumulate the time the trajectory spent in a non-initial basin during its most recent work unit
    if (timeWrapForwardFlux.size() > 0 or timeWrapBasinEntry.size() > 0 or timeWrapBasinExit.size() > 0)
    {
        double *fluxTimes, *fluxTimesEnd, *entryTimes, *entryTimesEnd, *exitTimes, *exitTimesEnd;
        fluxTimes = timeWrapForwardFlux.get_data(true);
        fluxTimesEnd = fluxTimes +  timeWrapForwardFlux.size();
        entryTimes = timeWrapBasinEntry.get_data(true);
        entryTimesEnd = entryTimes +  timeWrapBasinEntry.size();
        exitTimes = timeWrapBasinExit.get_data(true);
        exitTimesEnd = exitTimes +  timeWrapBasinExit.size();

        getWaitingTimes(waitingTimeScratchpad, waitingTimeSV, fluxTimes, fluxTimesEnd, entryTimes, entryTimesEnd, exitTimes, exitTimesEnd);

        if (timeWrapBasinEntry.compressed_deflate()) delete[] entryTimes;
        if (timeWrapBasinEntry.compressed_deflate()) delete[] entryTimes;
        if (timeWrapBasinExit.compressed_deflate()) delete[] exitTimes;
    }
    
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
