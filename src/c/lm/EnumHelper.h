/*
 * University of Illinois Open Source License
 * Copyright 2008-2012 Luthey-Schulten Group,
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
 *
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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
#ifndef ENUMHELPER_H
#define ENUMHELPER_H

//#include "lm/trajectory/Trajectory.h"
#include "lm/fflux/input/FFluxPhaseLimit.pb.h"
#include "lm/io/LimitTracking.pb.h"
#include "lm/fflux/input/FFluxPhase.pb.h"
#include "lm/input/Tilings.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"

// helper classes allowing for more direct access to the types and values of various enums

// from lm/fflux/input/FFluxPhaseLimit.proto
struct FFPhaseLimEnums {
    // enum typedefs
    typedef lm::fflux::input::FFluxPhaseLimit::StopCondition StopCondition;

    // StopCondition enum values
    static const StopCondition FORWARD_FLUXES = lm::fflux::input::FFluxPhaseLimit::FORWARD_FLUXES;
    static const StopCondition TRAJECTORY_COUNT = lm::fflux::input::FFluxPhaseLimit::TRAJECTORY_COUNT;
    static const StopCondition TIME = lm::fflux::input::FFluxPhaseLimit::TIME;

    // Functions for getting enum values as strings
    static inline const ::std::string& StopCondition_Name(StopCondition value)
    {
        return lm::fflux::input::FFluxPhaseLimit_StopCondition_Name(value);
    }
};

// from lm/input/SimulationPhase.proto
struct FFPhaseEnums {
    // enum typedefs
    typedef lm::fflux::input::FFluxPhase::TrajectoryDuplication TrajectoryDuplication;
    typedef lm::fflux::input::FFluxPhase::TrajectoryGeneration TrajectoryGeneration;

    // TrajectoryDuplication enum values
    static const TrajectoryDuplication NONE = lm::fflux::input::FFluxPhase::NONE;
    static const TrajectoryDuplication CYCLIC = lm::fflux::input::FFluxPhase::CYCLIC;
    static const TrajectoryDuplication UNIFORM_RANDOM = lm::fflux::input::FFluxPhase::UNIFORM_RANDOM;
    
    // TrajectoryGeneration enum values
    static const TrajectoryGeneration EAGER = lm::fflux::input::FFluxPhase::EAGER;
    static const TrajectoryGeneration LAZY = lm::fflux::input::FFluxPhase::LAZY;
};

// from lm/input/Tilings.proto
struct TilingEnums {
    // enum typedefs
    typedef lm::input::Tiling::SortOrder SortOrder;
    typedef lm::input::Tiling::TilingType TilingType;

    // SortOrder enum values
    static const SortOrder ASCENDING = lm::input::Tiling::ASCENDING;
    static const SortOrder DESCENDING = lm::input::Tiling::DESCENDING;

    // TilingType enum values
    static const TilingType LATTICE = lm::input::Tiling::LATTICE;
    static const TilingType VORONOI = lm::input::Tiling::VORONOI;
};

// from lm/input/TrajectoryLimits.proto
struct TrajLimEnums {
    // enum typedefs
    typedef lm::input::TrajectoryLimit::LimitType LimitType;
    typedef lm::input::TrajectoryLimit::StoppingCondition StoppingCondition;

    // LimitType enum values
    static const LimitType NONE = lm::input::TrajectoryLimit::NONE;
    static const LimitType TIME = lm::input::TrajectoryLimit::TIME;
    static const LimitType SPECIES = lm::input::TrajectoryLimit::SPECIES;
    static const LimitType ORDER_PARAMETER = lm::input::TrajectoryLimit::ORDER_PARAMETER;
    static const LimitType DEGREE_ADVANCEMENT = lm::input::TrajectoryLimit::DEGREE_ADVANCEMENT;

    // StoppingCondition enum values
    static const StoppingCondition MIN = lm::input::TrajectoryLimit::MIN;
    static const StoppingCondition MAX = lm::input::TrajectoryLimit::MAX;
    static const StoppingCondition INCREASING = lm::input::TrajectoryLimit::INCREASING;
    static const StoppingCondition DECREASING = lm::input::TrajectoryLimit::DECREASING;

    // Functions for getting enum values as strings
    static inline const ::std::string& LimitType_Name(LimitType value)
    {
        return lm::input::TrajectoryLimit_LimitType_Name(value);
    }
};

// from lm/message/WorkUnitStatus.proto
struct WUStatEnums {
    // enum typedefs
    typedef lm::message::WorkUnitStatus::Status Status;

    // Status enum values
    static const Status NONE = lm::message::WorkUnitStatus::NONE;
    static const Status STEPS_FINISHED = lm::message::WorkUnitStatus::STEPS_FINISHED;
    static const Status LIMIT_REACHED = lm::message::WorkUnitStatus::LIMIT_REACHED;
    static const Status ERROR = lm::message::WorkUnitStatus::ERROR;
};

// including stuff from Trajectory causes circular import nightmares. Maybe there's a way to deal with this?
// from lm/trajectory/Trajectory.h
//struct TrajEnums {
//    // enum typedefs
//    typedef lm::trajectory::Trajectory::Status Status;
//
//    // Status enum values
//    static const Status ABORTED = lm::trajectory::Trajectory::ABORTED;
//    static const Status FINISHED = lm::trajectory::Trajectory::FINISHED;
//    static const Status NOT_STARTED = lm::trajectory::Trajectory::NOT_STARTED;
//    static const Status RUNNING = lm::trajectory::Trajectory::RUNNING;
//    static const Status WAITING = lm::trajectory::Trajectory::WAITING;
//};

// from robertslab/pbuf/NDArray.proto
struct NDArrEnums {
    // enum typedefs
    typedef robertslab::pbuf::NDArray::ArrayOrder ArrayOrder;
    typedef robertslab::pbuf::NDArray::ByteOrder ByteOrder;
    typedef robertslab::pbuf::NDArray::DataType DataType;

    // ArrayOrder enum values
    static const ArrayOrder ROW_MAJOR = robertslab::pbuf::NDArray::ROW_MAJOR;
    static const ArrayOrder COLUMN_MAJOR = robertslab::pbuf::NDArray::COLUMN_MAJOR;
    static const ArrayOrder IMPL_ORDER = robertslab::pbuf::NDArray::IMPL_ORDER;
};

#endif /* ENUMHELPER_H */