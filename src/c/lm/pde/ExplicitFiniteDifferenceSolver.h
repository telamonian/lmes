/*
 * Copyright 2016 Johns Hopkins University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
 *
 * Author(s): Elijah Roberts
 */

#ifndef LM_PDE_EXPLICITFINITEDIFFERENCESOLVER_H
#define LM_PDE_EXPLICITFINITEDIFFERENCESOLVER_H

#include "lm/Types.h"
#include "lm/input/MicroenvironmentModel.pb.h"
#include "lm/io/OutputOptions.pb.h"
#include "lm/io/TrajectoryLimits.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/pde/DiffusionPDESolver.h"

namespace lm {
namespace pde {

class ExplicitFiniteDifferenceSolver : public lm::pde::DiffusionPDESolver
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

public:
    ExplicitFiniteDifferenceSolver();
    virtual ~ExplicitFiniteDifferenceSolver();
    virtual void setMicroenvironmentModel(const lm::input::MicroenvironmentModel& model);
    virtual void setLimits(const lm::io::TrajectoryLimits& limits);
    virtual void setOutputOptions(const lm::io::OutputOptions& outputOptions);
    virtual void reset();
    virtual void getState(lm::io::TrajectoryState* state, uint trajectoryNumber=0);
    virtual void setState(const lm::io::TrajectoryState& state, uint trajectoryNumber=0);
    virtual lm::message::WorkUnitOutput* getOutput(uint trajectoryNumber=0);
    virtual lm::message::WorkUnitStatus::Status getStatus(uint trajectoryNumber=0);
    virtual uint64_t generateTrajectory(uint64_t maxSteps);

protected:
    //void calculateWithReflectingBoundary(ndarray<double>& grid, double runtime);
    //virtual void calculateAbsorbingBoundary(ndarray<double>& grid, double time, double value);

protected:
    double D;
    double dx;
    double dt;

    // Trajectory output.
    lm::message::WorkUnitOutput* output;
    bool writeConcentrationsTimeSeries;
    double concentrationsWriteInterval;

    // Trajectory status.
    lm::message::WorkUnitStatus::Status status;
    uint64_t trajectoryId;
    bool previouslyStarted;

    // The PDE state.
    double time, timeLimit;
    ndarray<double>* grid;
};

}
}

#endif // LM_PDE_EXPLICITFINITEDIFFERENCESOLVER_H
