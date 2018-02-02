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
#ifndef LM_PROTWRAP_FFLUXSTAGEOUTPUT_H_
#define LM_PROTWRAP_FFLUXSTAGEOUTPUT_H_

#include <cmath>
#include <vector>

#include "lm/fflux/io/FFluxPhaseOutput.pb.h"
#include "lm/fflux/io/FFluxPhaseOutputWrap.h"
#include "lm/fflux/io/FFluxStageOutput.pb.h"
#include "lm/Math.h"
#include "lm/protowrap/Repeated.h"
#include "lm/protowrap/Msg.h"
#include "lm/protowrap/WrappedFields.h"
#include "lm/Types.h"
#include "lm/VectorMath.h"

namespace lm {
namespace protowrap {

class FFluxStageOutputRawWrap : public lm::protowrap::Msg<FFluxStageOutputRawWrap, lm::fflux::io::FFluxStageOutputRaw>
{
    typedef lm::fflux::io::FFluxPhaseOutput FFluxPhaseOutputMsg;
    typedef lm::protowrap::Repeated<FFluxPhaseOutputMsg> FFluxPhaseOutputsWrap;
    
    WRAPPED_FIELDS(repeated, uint64_t, successful_trajectory_counts,
                   repeated, double,   successful_trajectory_total_times,
                   repeated, uint64_t, failed_trajectory_counts,
                   repeated, double,   failed_trajectory_total_times,
                   repeated, double,   variances,
                   repeated, uint64_t, first_trajectory_ids,
                   repeated, uint64_t, final_trajectory_ids)

    void buildFromFFluxPhaseOutputs(const FFluxPhaseOutputsWrap& ffluxPhaseOutputsWrap)
    {
        // store data from every phase output message in a set of repeated fields in the raw stage output
        ffluxPhaseOutputsWrap.GetAll(&FFluxPhaseOutputMsg::successful_trajectories_launched_count, mutable_successful_trajectory_counts()->back_inserter());
        ffluxPhaseOutputsWrap.GetAll(&FFluxPhaseOutputMsg::successful_trajectories_launched_total_time, mutable_successful_trajectory_total_times()->back_inserter());

        ffluxPhaseOutputsWrap.GetAll(&FFluxPhaseOutputMsg::failed_trajectories_launched_count, mutable_failed_trajectory_counts()->back_inserter());
        ffluxPhaseOutputsWrap.GetAll(&FFluxPhaseOutputMsg::failed_trajectories_launched_total_time, mutable_failed_trajectory_total_times()->back_inserter());

        ffluxPhaseOutputsWrap.GetAll(&FFluxPhaseOutputMsg::variance, mutable_variances()->back_inserter());

        ffluxPhaseOutputsWrap.GetAll(&FFluxPhaseOutputMsg::first_trajectory_id, mutable_first_trajectory_ids()->back_inserter());
        ffluxPhaseOutputsWrap.GetAll(&FFluxPhaseOutputMsg::final_trajectory_id, mutable_final_trajectory_ids()->back_inserter());
    }
};

class FFluxStageOutputSummaryWrap : public lm::protowrap::Msg<FFluxStageOutputSummaryWrap, lm::fflux::io::FFluxStageOutputSummary>
{
    WRAPPED_FIELDS(repeated, double, first_passage_times,
                   repeated, double, costs,
                   repeated, double, weights)

    void buildFromFFluxStageOutputRaw(const FFluxStageOutputRawWrap& outputRaw)
    {
        buildCosts(outputRaw);
        buildWeights(outputRaw);
        buildFirstPassageTimes();
    }

    void buildCosts(const FFluxStageOutputRawWrap& outputRaw)
    {
        // load some data from the raw stage output into a few vectors
        std::vector<double> successfulTrajectoryCounts, failedTrajectoryCounts, successfulTrajectoryTotalTimes, failedTrajectoryTotalTimes;
        outputRaw.successful_trajectory_counts().deserializeTo(successfulTrajectoryCounts);
        outputRaw.failed_trajectory_counts().deserializeTo(failedTrajectoryCounts);
        outputRaw.successful_trajectory_total_times().deserializeTo(successfulTrajectoryTotalTimes);
        outputRaw.failed_trajectory_total_times().deserializeTo(failedTrajectoryTotalTimes);

        // calculate the costs
        std::vector<double> newCosts = (successfulTrajectoryTotalTimes + failedTrajectoryTotalTimes) / (successfulTrajectoryCounts + failedTrajectoryCounts);

        // (re)calculate the phase zero cost without the time spent outside of the initial basin. This method slightly underestimates the cost, but produces consistent results (since trajectories sometimes will and sometimes won't leave the starting basin during phase 0).
        newCosts[0] = successfulTrajectoryTotalTimes[0] / successfulTrajectoryCounts[0];

        // set the costs
        mutable_costs()->serializeFrom(newCosts);
    }

    void buildWeights(const FFluxStageOutputRawWrap& outputRaw)
    {
        // load some data from the raw stage output into a few vectors
        std::vector<double> successfulTrajectoryCounts, successfulTrajectoryTotalTimes, failedTrajectoryCounts, variances;
        outputRaw.successful_trajectory_counts().deserializeTo(successfulTrajectoryCounts);
        outputRaw.successful_trajectory_total_times().deserializeTo(successfulTrajectoryTotalTimes);
        outputRaw.failed_trajectory_counts().deserializeTo(failedTrajectoryCounts);
        outputRaw.variances().deserializeTo(variances);

        // calculate the probabilities, ie the phase weights for phases i>0. This won't produce the correct value for phase zero
        std::vector<double> newWeights = successfulTrajectoryCounts / (successfulTrajectoryCounts + failedTrajectoryCounts);

        // calculate the phase zero weight as the mean waiting time in between phase zero forward flux events. successfulTrajectoryTotalTimes[0] has been corrected in that the time spent in other basins (than the starting one) has been subtracted.
        newWeights[0] = successfulTrajectoryTotalTimes[0] / successfulTrajectoryCounts[0];
        // weights[0] = variances[0] / pow(successfulTrajectoryTotalTimes[0] / successfulTrajectoryCounts[0], 2);

        // set the phase weights
        mutable_weights()->serializeFrom(newWeights);
    }

    void buildFirstPassageTimes()
    {
        // initialize a vector (with a copy of the weights) to temporarily hold the first passage times as we calculate them
        std::vector<double> newFirstPassageTimes(weights().begin(), weights().end());

        // set the phase zero weight to 1 so it doesn't affect the calculation of the cumulative probabilities.
        newFirstPassageTimes[0] = 1.0;

        // get the cumulative product of the phase i>0 forward flux probabilities
        cumprod(newFirstPassageTimes.begin(), newFirstPassageTimes.end(), newFirstPassageTimes.begin());

        // calculate the first passage time to each tile edge by inverting the cumulative probabilites and multiplying them by the phase zero waiting time
        newFirstPassageTimes = (1 / newFirstPassageTimes) * weights(0);

        // set the first passage times
        mutable_first_passage_times()->serializeFrom(newFirstPassageTimes);
    }
};

class FFluxStageOutputWrap : public lm::protowrap::Msg<FFluxStageOutputWrap, lm::fflux::io::FFluxStageOutput>
{
    typedef lm::fflux::io::FFluxPhaseOutput FFluxPhaseOutputMsg;
    typedef lm::protowrap::Repeated<FFluxPhaseOutputMsg> FFluxPhaseOutputsWrap;

    WRAPPED_FIELDS(optional, FFluxStageOutputSummaryWrap, fflux_stage_output_summary,
                   optional, FFluxStageOutputRawWrap,     fflux_stage_output_raw)

    void buildFromFFluxPhaseOutputs(const FFluxPhaseOutputsWrap& ffluxPhaseOutputsWrap)
    {
        mutable_fflux_stage_output_raw()->buildFromFFluxPhaseOutputs(ffluxPhaseOutputsWrap);
        mutable_fflux_stage_output_summary()->buildFromFFluxStageOutputRaw(fflux_stage_output_raw());
    }
};

}
}


#endif /* LM_PROTOWRAP_FFLUXSTAGEOUTPUT_H_ */
