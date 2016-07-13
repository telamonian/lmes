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

#include <vector>

#include "lm/fflux/io/FFluxPhaseOutput.pb.h"
#include "lm/fflux/io/FFluxPhaseOutputWrap.h"
#include "lm/fflux/io/FFluxStageOutput.pb.h"
#include "lm/Math.h"
#include "lm/protowrap/Repeated.h"
#include "lm/protowrap/Msg.h"
#include "lm/protowrap/WrappedFields.h"
#include "lm/Types.h"

namespace lm {
namespace protowrap {

class FFluxStageOutputRawWrap : public lm::protowrap::Msg<FFluxStageOutputRawWrap, lm::fflux::io::FFluxStageOuputRaw>
{
    typedef lm::fflux::io::FFluxPhaseOutput FFluxPhaseOutputMsg;
    typedef lm::protowrap::Repeated<FFluxPhaseOutputMsg> FFluxPhaseOutputsWrap;
    
    WRAPPED_FIELDS_W_SERIALIZERS(repeated, uint64_t, successful_trajectory_counts,
                                 repeated, double,   successful_trajectory_total_times,
                                 repeated, uint64_t, failed_trajectory_counts,
                                 repeated, double,   failed_trajectory_total_times)

    void buildFromFFluxPhaseOutputs(const FFluxPhaseOutputsWrap& ffluxPhaseOutputsWrap)
    {
        // store data from every phase output message in a set of repeated fields in the raw stage output
        ffluxPhaseOutputsWrap.GetAll(&FFluxPhaseOutputMsg::successful_trajectories_launched_count, mutable_successful_trajectory_counts()->back_inserter());
        ffluxPhaseOutputsWrap.GetAll(&FFluxPhaseOutputMsg::successful_trajectories_launched_total_time, mutable_successful_trajectory_total_times()->back_inserter());

        ffluxPhaseOutputsWrap.GetAll(&FFluxPhaseOutputMsg::failed_trajectories_launched_count, mutable_failed_trajectory_counts()->back_inserter());
        ffluxPhaseOutputsWrap.GetAll(&FFluxPhaseOutputMsg::failed_trajectories_launched_total_time, mutable_failed_trajectory_total_times()->back_inserter());
    }
};

class FFluxStageOutputWrap : public lm::protowrap::Msg<FFluxStageOutputWrap, lm::fflux::io::FFluxStageOutput>
{
    typedef lm::fflux::io::FFluxPhaseOutput FFluxPhaseOutputMsg;
    typedef lm::protowrap::Repeated<FFluxPhaseOutputMsg> FFluxPhaseOutputsWrap;

    WRAPPED_FIELDS_W_SERIALIZERS(repeated, double,                  switching_time_per_tile,
                                 repeated, double,                  fluxes,
                                 repeated, double,                  probabilities,
                                 optional, FFluxStageOutputRawWrap, fflux_stage_output_raw,
                                 repeated, FFluxPhaseOutputMsg,     fflux_phase_outputs)

    void buildFromFFluxPhaseOutputs(const FFluxPhaseOutputsWrap& ffluxPhaseOutputsWrap)
    {
        mutable_fflux_stage_output_raw()->buildFromFFluxPhaseOutputs(ffluxPhaseOutputsWrap);

        buildFluxes();
        buildProbabilites();
        buildSwitchingTimePerTile();
    }

    void buildFluxes()
    {
        // load some data from the raw stage output into a few valarrays
        std::valarray<double> successfulTrajectoryCounts(lm::protowrap::make_valarray<double>::call(fflux_stage_output_raw().successful_trajectory_counts()));
        std::valarray<double> successfulTrajectoryTotalTimes(lm::protowrap::make_valarray<>::call(fflux_stage_output_raw().successful_trajectory_total_times()));
        std::valarray<double> failedTrajectoryTotalTimes(lm::protowrap::make_valarray<>::call(fflux_stage_output_raw().failed_trajectory_total_times()));


//        std::valarray<double> successfulTrajectoryCounts(static_cast<const double*>(fflux_stage_output_raw().successful_trajectory_counts().data()), fflux_stage_output_raw().successful_trajectory_counts().size());
//        std::valarray<double> successfulTrajectoryTotalTimes(fflux_stage_output_raw().successful_trajectory_total_times().begin(), fflux_stage_output_raw().successful_trajectory_total_times().size());
//        std::valarray<double> failedTrajectoryTotalTimes(fflux_stage_output_raw().failed_trajectory_total_times().begin(), fflux_stage_output_raw().failed_trajectory_total_times().size());
        
        // calculate the fluxes
        std::valarray<double> newFluxes(successfulTrajectoryCounts/(successfulTrajectoryTotalTimes + failedTrajectoryTotalTimes));
        
        // set the fluxes
        mutable_fluxes()->serializeFrom(&newFluxes[0], &newFluxes[0] + newFluxes.size());
    }

    void buildProbabilites()
    {
        // load some data from the raw stage output into a few valarrays
        std::valarray<double> successfulTrajectoryCounts(lm::protowrap::make_valarray<double>::call(fflux_stage_output_raw().successful_trajectory_counts()));
        std::valarray<double> failedTrajectoryCounts(lm::protowrap::make_valarray<double>::call(fflux_stage_output_raw().failed_trajectory_counts()));

//        std::valarray<double> successfulTrajectoryCounts(static_cast<const double*>(fflux_stage_output_raw().successful_trajectory_counts().data()), fflux_stage_output_raw().successful_trajectory_counts().size());
//        std::valarray<double> failedTrajectoryCounts(static_cast<const double*>(fflux_stage_output_raw().failed_trajectory_counts().data()), fflux_stage_output_raw().failed_trajectory_counts().size());
        
        // calculate the probabilities
        std::valarray<double> newProbabilities(successfulTrajectoryCounts/(successfulTrajectoryCounts + failedTrajectoryCounts));

        // set the probabilities
        mutable_probabilities()->serializeFrom(&newProbabilities[0], &newProbabilities[0] + newProbabilities.size());
    }

    void buildSwitchingTimePerTile()
    {
        std::vector<double> newSwitchingTimePerTile;

        cumprod(probabilities().begin(), probabilities().end(), std::back_inserter(newSwitchingTimePerTile));
        pairwiseMul(newSwitchingTimePerTile.begin(), newSwitchingTimePerTile.end(), newSwitchingTimePerTile.begin(), fluxes(0));

//        std::partial_sum(probabilities().begin(), probabilities().end(), std::back_inserter(newSwitchingTimePerTile), std::multiplies<double>());
    }

};

}
}


#endif /* LM_PROTOWRAP_FFLUXSTAGEOUTPUT_H_ */
