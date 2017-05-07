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

//const char* filenamesLiteralDwellTimes[] = {"genetic_toggle_switch_-_FFluxPhaseZeroDwellTimes.sfile"};
const char* filenamesLiteralDwellTimes[] = {"@TESTDATA_ROOT@/genetic_toggle_switch_-_FFluxPhaseZeroDwellTimes.sfile"};
const vector<string> filenamesDwellTimes(filenamesLiteralDwellTimes, filenamesLiteralDwellTimes+1);

//const int speciesCountsLiteral[] = {0,1,2,3,4,5,6};
//const vector<int> speciesCounts(speciesCountsLiteral, speciesCountsLiteral+7);

const double timesLiteralDwellTimes[] = {2143.1557044971123, 2.186540242404135, 1.5327591258437678, 38.35022841198361, 14.705618729017715, 5.1452259907050575, 113.92381583519318, 10.288274585985619, 94.35669860103098, 698.7542958845934, 10.1577715765834, 3.8856863106434503, 10.086255038259878, 8.06676147398457, 13.390464324034838, 157.1240187575895, 5.420272994558218, 31.887065521064414, 1.7622404895273576, 166.49746150329247, 5.8711508460664845, 9.177063304925923, 2039.7047068238498, 16.031005856341835, 6.910774337641669, 18.09174130286283, 15.34936849663427, 84.05703912622903, 56.86096368642029, 156.3245457232697, 333.3631322272795, 24.913254925038927, 44.48873464210919, 196.17235480119416, 22.381122340997536, 13.08829186528601, 136.0757635984027, 37.36133360639988, 6.991071246102308, 131.52970676049108, 10.928335313723267, 9.67351705117784, 52.86189354434026, 13.424365996739539, 89.87486740972707, 316.3375634263251, 333.4630278486716, 91.01290464291026, 51.83483814897659, 260.16384845672565, 94.13659535214856, 6.45638218060526, 5.7086374095472365, 4103.3489213610565, 11.145308213469434, 35.87691590621034, 67.16928960902806, 42.157385358968895, 14.973829537068923, 113.01172714914901, 48.280893861891855, 87.84897461680703, 1557.6405012858477, 35.03579225526573, 11.843663823236056, 17.341485879320317, 25.632532747235018, 27.246073068437, 11.310655949044303, 6.693007863340426, 16.52731301919539, 2670.188926018026, 76.11933389918158, 23.37416609168031, 12.996701550853686, 9.82786088283865, 1854.2668701376297, 6.6317400257778445, 369.7895520548975, 8.04607365071115, 1489.8483182779814, 3.5142216793965417, 3690.8491349067554, 116.08250494590538, 57.073220957889134, 25.012103155856494, 106.619940896363, 7.656542350615382, 11.893212007242255, 942.7277274757844, 17.22530072790323, 16.438039190858035, 30.07651328227621, 57.97641647784985, 61.46636252948883, 80.0124827044474, 51.114260589836704, 10.484347049225107, 57.26351814633199, 467.3847608781034};
const vector<double> intendedDwellTimes(timesLiteralDwellTimes, timesLiteralDwellTimes+36);

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
    FFluxPhaseZeroTrajectoryFixture(): initialWaitingTime(0),ltLoader(filenamesDwellTimes),previousEvent(lm::fflux::BASIN_ENTRY, 0)
    {
//        printCWD();
    }

public:
    double initialWaitingTime;
    LimitTrackingsLoader ltLoader;
    lm::fflux::Event previousEvent;
//    lm::fflux::FFluxPhaseZeroTrajectory ffluxPhaseZeroTrajectory;

    // streaming variance of the waiting time in between interface 0 forward crossing events
    std::vector<double> waitingTimes;
    StreamingVariance waitingTimeSV;

protected:
    lm::protowrap::Repeated<lm::io::LimitTracking> limitTrackingsWrap;
    lm::protowrap::NDArray<double> timeWrapForwardFlux, timeWrapBasinEntry, timeWrapBasinExit;

};

TEST_F(FFluxPhaseZeroTrajectoryFixture, getDwellTimes_test)
{
    // set wrapper on the limit_trackings field
    limitTrackingsWrap.setWrappedField(ltLoader.limitTrackings);

    int i = 0;
    for (;i<limitTrackingsWrap.size();)
    {
        if (limitTrackingsWrap.Get(i).trajectory_id()==152) // || limitTrackingsWrap.Get(i).trajectory_id()==153 || limitTrackingsWrap.Get(i).trajectory_id()==154 || limitTrackingsWrap.Get(i).trajectory_id()==155)
        {
            // the limit tracking is from one of the production stage phase zero trajectories
            // set wrappers on the ndarrays with the limit-triggering times
            timeWrapForwardFlux.setWrappedMsg(limitTrackingsWrap.Get(i++).times());
            timeWrapBasinEntry.setWrappedMsg(limitTrackingsWrap.Get(i++).times());
            timeWrapBasinExit.setWrappedMsg(limitTrackingsWrap.Get(i++).times());

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

                initialWaitingTime = lm::fflux::FFluxPhaseZeroTrajectory::getWaitingTimes(initialWaitingTime, &previousEvent, &waitingTimes, fluxTimes, fluxTimesEnd, entryTimes, entryTimesEnd, exitTimes, exitTimesEnd);

                if (timeWrapBasinEntry.compressed_deflate()) delete[] entryTimes;
                if (timeWrapBasinEntry.compressed_deflate()) delete[] entryTimes;
                if (timeWrapBasinExit.compressed_deflate()) delete[] exitTimes;
            }
        }
        else
        {
            // the limit tracking is from a trajectory we don't care about
            i++;
        }
    }

    // test some scalar values in ffluxStage
    EXPECT_EQ(36, waitingTimes.size());
    EXPECT_EQ(intendedDwellTimes, waitingTimes);

//    EXPECT_EQ(0, ffluxStage.basin_index());
//
//    // test some scalar values in ffluxPhase
//    EXPECT_EQ(0, ffluxPhase.tiling_id());
//    EXPECT_EQ(0, ffluxPhase.basin_index());
//    EXPECT_EQ(4, ffluxPhase.fflux_phase_index());
//
//    // test some scalar values in ffluxPhaseStartPoint
//    EXPECT_EQ(1, ffluxPhaseStartPoint.count());
//
//    // test some vector/repeated values in ffluxPhaseStartPoint
//    int countSize = ffluxPhaseStartPoint.species_coordinates_size();
//    for (int i=0; i<countSize; i++)
//    {
//        int valExpected = speciesCounts[i];
//        int valActual = ffluxPhaseStartPoint.species_coordinates(i);
//
//        EXPECT_EQ(valExpected, valActual);
//    }
//
//    int timesSize = ffluxPhaseStartPoint.times_size();
//    for (int i=0; i<timesSize; i++)
//    {
//        double valExpected = times[i];
//        double valActual = ffluxPhaseStartPoint.times(i);
//
//        EXPECT_NEAR(valExpected, valActual, absolute_tolerance);
//    }
}
