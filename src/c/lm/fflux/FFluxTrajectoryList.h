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
#ifndef LM_FFLUX_FFLUXTRAJECTORYLIST_H_
#define LM_FFLUX_FFLUXTRAJECTORYLIST_H_

#include <google/protobuf/repeated_field.h>
#include <map>
#include <string>
#include <vector>

#include "lm/fflux/FFluxTrajectory.h"
#include "lm/fflux/FFluxPhaseZeroTrajectory.h"
#include "lm/fflux/input/FFluxInput.h"
#include "lm/fflux/input/FFluxPhase.pb.h"
#include "lm/fflux/io/FFluxPhaseOutputWrap.h"
#include "lm/input/Input.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/io/FFluxOutput.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/limit/LimitTrackingListWrap.h"
#include "lm/message/Communicator.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/rng/XORShift.h"
#include "lm/tiling/Tilings.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/trajectory/TrajectoryList.h"
#include "lm/Types.h"

namespace lm {
namespace fflux {

typedef std::vector<lm::io::TilingHist*> TilingVector;

class FFluxTrajectoryList : public lm::trajectory::TrajectoryList
{
public:
    // ffluxPhase n==0 constructor
    FFluxTrajectoryList(uint64_t count, uint64_t simulationPhaseIndex, const lm::fflux::input::FFluxPhase& ffluxPhase,
                        const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit, uint simultaneousWorkUnits,
                        const lm::fflux::input::FFluxInput& input, const lm::input::Basin& basin);

    // ffluxPhase n>0 constructor
    FFluxTrajectoryList(uint64_t count, uint64_t simulationPhaseIndex, const lm::fflux::input::FFluxPhase& ffluxPhase,
                        const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit, uint simultaneousWorkUnits,
                        const lm::fflux::input::FFluxInput& input, const lm::protowrap::FFluxPhaseOutputWrap& previousPhaseOutput);
    // ffluxPhase custom constructor
    FFluxTrajectoryList(uint64_t count, uint64_t simulationPhaseIndex, const lm::fflux::input::FFluxPhase& ffluxPhase,
                        const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit, uint simultaneousWorkUnits,
                        const lm::fflux::input::FFluxInput& input);
    virtual ~FFluxTrajectoryList() {}

    virtual void workUnitPartFinished(const lm::message::WorkUnitStatus& wusMsg, lm::trajectory::Trajectory* traj);

    // accessors
    static uint64_t getTrajectoriesToStart(const lm::fflux::input::FFluxPhase& ffluxPhase, const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit, uint simultaneousWorkUnits);

    // mutators
    virtual void initTrajectories(uint64_t trajectoriesToStart);
    virtual void recycleFFluxTrajectory(lm::trajectory::Trajectory* traj);

protected:
    // initializers
    template <typename InputIterator> lm::trajectory::Trajectory* initFFluxPhaseZeroTrajectory(const lm::input::Input& input, InputIterator speciesStart, InputIterator speciesEnd, double startTime, uint64_t phase, uint64_t id=DEFAULT_TRAJECTORY_ID)
    {
        return initTrajectory(new lm::fflux::FFluxPhaseZeroTrajectory(input, speciesStart, speciesEnd, startTime, phase, resolveTrajectoryID(id)));
    }
    template <typename InputIterator> lm::trajectory::Trajectory* initFFluxTrajectory(const lm::input::Input& input, InputIterator speciesStart, InputIterator speciesEnd, double startTime, uint64_t phase, uint64_t id=DEFAULT_TRAJECTORY_ID)
    {
        return initTrajectory(new lm::fflux::FFluxTrajectory(input, speciesStart, speciesEnd, startTime, phase, resolveTrajectoryID(id)));
    }
    virtual void initTrajectoriesCyclic(uint64_t trajectoriesToStart);
    virtual void initTrajectoriesUniformRandom(uint64_t trajectoriesToStart);
    virtual lm::trajectory::Trajectory* recycleTrajectoryCyclic(uint64_t oldID);
    virtual lm::trajectory::Trajectory* recycleTrajectoryUniformRandom(uint64_t oldID);

protected:
    const lm::fflux::input::FFluxInput& input;
    const lm::fflux::input::FFluxPhase& ffluxPhase;
    const lm::fflux::input::FFluxPhaseLimit& ffluxPhaseLimit;

    // this is a pointer (and not a ref) because in some cases it has to be set to NULL
    const lm::protowrap::FFluxPhaseOutputWrap* previousPhaseOutputPtr;
    lm::protowrap::FFluxPhaseOutputWrap previousPhaseOutputCustomWrap;
    lm::fflux::io::FFluxPhaseOutput previousPhaseOutputCustom;

    lm::limit::LimitTrackingListWrap limitTrackingListWrap;

    size_t cyclicCounter;
};

}
}
#endif
