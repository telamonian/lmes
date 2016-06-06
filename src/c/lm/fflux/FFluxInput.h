/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
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
#ifndef LM_FFLUX_FFLUXINPUT_H
#define LM_FFLUX_FFLUXINPUT_H

#include <list>
#include <map>
#include <string>

#include "lm/input/Input.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/BoundaryConditions.pb.h"
#include "lm/input/DiffusionModel.pb.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/input/OutputOptions.pb.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/input/SimulationParameters.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/oparam/OParams.h"
#include "lm/option/SimulationParameters.h"
#include "lm/tiling/Tilings.h"
#include "lm/trajectory/TrajectoryLimits.h"
#include "lm/Types.h"

namespace lm {
namespace fflux {

class FFluxInput : public lm::input::Input
{
public:
    FFluxInput(const lm::io::hdf5::Hdf5File& file);
    virtual ~FFluxInput();

protected:
    optional double precision_goal = 1;

    /*
     * - confidence level that is associated with the precision goal
     */
    optional double precision_goal_confidence = 2 [default=0.95];

    /*
     * - Stopping conditions for each forward flux phase. Can be specified one of two ways (an error will be raised if more than one way is used)
     *     - (default) If precisionGoal is specified, the phase limits are determined automatically
     *     - A list of phase limits of length equal to the total number of phases in the simulation (ie the number of interfaces in all the tilings used)
     */
    repeated lm.io.FFluxPhaseLimits fflux_phase_limits = 3;

    /*
     * - if true, after performing the forward simulation on each tiling, flip the tiling around and run the simulation again in order to perform the reverse simulation as well
     */
    optional bool do_reverse_simulations = 4 [default=false];


};

}
}
#endif // LM_FFLUX_FFLUXINPUT_H
