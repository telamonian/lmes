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

#include "lm/io/SimulationPhase.pb.h"
#include "lm/io/Tilings.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"

// helper classes allowing for more direct access to the types and values of various enums

//from lm/io/SimulationPhase.proto
struct SimPhaseEnums {
    // enum typedefs
    typedef lm::io::SimulationPhase::TrajectorySource TrajectorySource;

    // TrajectorySource enum values
    static const TrajectorySource TRAJECTORY_STATES = lm::io::SimulationPhase::TRAJECTORY_STATES;
    static const TrajectorySource PREVIOUS_PHASE = lm::io::SimulationPhase::PREVIOUS_PHASE;
};

// from lm/io/Tilings.proto
struct TilingEnums {
    // enum typedefs
    typedef lm::io::Tiling::SortOrder SortOrder;
    typedef lm::io::Tiling::TilingType TilingType;

    // SortOrder enum values
    static const SortOrder ASCENDING = lm::io::Tiling::ASCENDING;
    static const SortOrder DESCENDING = lm::io::Tiling::DESCENDING;

    // TilingType enum values
    static const TilingType LATTICE = lm::io::Tiling::LATTICE;
    static const TilingType VORONOI = lm::io::Tiling::VORONOI;
};

// from lm/io/TrajectoryLimits.proto
struct TrajLimEnums {
    // enum typedefs
    typedef lm::io::TrajectoryLimit::LimitType LimitType;
    typedef lm::io::TrajectoryLimit::StoppingCondition StoppingCondition;

    // LimitType enum values
    static const LimitType NONE = lm::io::TrajectoryLimit::NONE;
    static const LimitType TIME = lm::io::TrajectoryLimit::TIME;
    static const LimitType SPECIES = lm::io::TrajectoryLimit::SPECIES;
    static const LimitType ORDER_PARAMETER = lm::io::TrajectoryLimit::ORDER_PARAMETER;
    static const LimitType DEGREE_ADVANCEMENT = lm::io::TrajectoryLimit::DEGREE_ADVANCEMENT;

    // StoppingCondition enum values
    static const StoppingCondition MIN = lm::io::TrajectoryLimit::MIN;
    static const StoppingCondition MAX = lm::io::TrajectoryLimit::MAX;
    static const StoppingCondition INCREASING = lm::io::TrajectoryLimit::INCREASING;
    static const StoppingCondition DECREASING = lm::io::TrajectoryLimit::DECREASING;
};

#endif /* ENUMHELPER_H */