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

#include <cassert>
#include <cmath>
#include <limits>

#include "hrtime.h"
#include "lm/ClassFactory.h"
#include "lm/Exceptions.h"
#include "lm/Types.h"
#include "lm/message/WorkUnitOutput.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/pde/ExplicitFiniteDifferenceSolver.h"
#include "robertslab/Types.h"
#include "robertslab/pbuf/NDArraySerializer.h"

namespace lm {
namespace pde {

bool ExplicitFiniteDifferenceSolver::registered=ExplicitFiniteDifferenceSolver::registerClass();

bool ExplicitFiniteDifferenceSolver::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::pde::DiffusionPDESolver","lm::pde::ExplicitFiniteDifferenceSolver",&ExplicitFiniteDifferenceSolver::allocateObject);
    return true;
}

void* ExplicitFiniteDifferenceSolver::allocateObject()
{
    return new ExplicitFiniteDifferenceSolver();
}

ExplicitFiniteDifferenceSolver::ExplicitFiniteDifferenceSolver()
:D(0.0),dx(0.0),dt(0.0),output(new lm::message::WorkUnitOutput()),status(lm::message::WorkUnitStatus::NONE),
time(0.0),timeLimit(std::numeric_limits<double>::infinity()),grid(NULL)
{
}

ExplicitFiniteDifferenceSolver::~ExplicitFiniteDifferenceSolver()
{
    // Free any output memory.
    if (output != NULL) delete output; output = NULL;

    // Free any grid memory.
    if (grid != NULL) delete grid; grid = NULL;
}


void ExplicitFiniteDifferenceSolver::setMicroenvironmentModel(const lm::input::MicroenvironmentModel& model)
{
    if (model.diffusion_coefficients_size() <= 0) throw lm::InvalidArgException("model", "the model did not have enough diffusion_coefficient values");

    // Extract the needed parameters.
    D = model.diffusion_coefficients(0);
    dx = model.grid_spacing();

    // Validate the parameters.
    if (D <= 0.0) throw lm::InvalidArgException("dx", "The diffusion coefficient must be positive.", dt);
    if (dx <= 0.0) throw lm::InvalidArgException("dx", "The grid length must be positive.", dt);
    if (dt < 0.0) throw lm::InvalidArgException("dt", "The timestep was negative.", dt);

    // Figure out the dt to use, if none was specified.
    if (dt == 0.0) dt = (dx*dx)/(6*D*2);

    // Make sure the stability criteria holds.
    if (dt > (dx*dx)/(6*D)) throw lm::InvalidArgException("dt", "The timestep did not follow obey stability criteria for the ExplicitFiniteDifferenceSolver.", dt);
}

void ExplicitFiniteDifferenceSolver::setLimits(const lm::io::TrajectoryLimits& limits)
{
    // Set the time limit.
    if (limits.has_time_limit() && limits.time_limit().limit_type() == lm::io::TrajectoryLimits::TIME && limits.time_limit().stopping_condition() == lm::io::TrajectoryLimits::MAX && limits.time_limit().has_dvalue())
        timeLimit = limits.time_limit().dvalue();
}

void ExplicitFiniteDifferenceSolver::setOutputOptions(const lm::io::OutputOptions& outputOptions)
{
}

void ExplicitFiniteDifferenceSolver::reset()
{
    // Reset the output.
    if (output != NULL) delete output;
    output = new lm::message::WorkUnitOutput();

    // Reset the status.
    status = lm::message::WorkUnitStatus::NONE;

    // Reset the time and time limit.
    time = 0.0;
    timeLimit = std::numeric_limits<double>::infinity();

    // Reset the grid.
    if (grid != NULL) delete grid; grid = NULL;
}

void ExplicitFiniteDifferenceSolver::getState(lm::io::TrajectoryState* state, uint trajectoryNumber)
{
    if (trajectoryNumber > 0) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());

    // Save the state into the message.
    state->mutable_diffusion_pde_state()->set_time(time);
    robertslab::pbuf::NDArraySerializer::serializeInto<double>(state->mutable_diffusion_pde_state()->mutable_concentrations(0), *grid);
}

void ExplicitFiniteDifferenceSolver::setState(const lm::io::TrajectoryState& state, uint trajectoryNumber)
{
    if (trajectoryNumber > 0) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());

    // Load the state from the message.
    time = state.diffusion_pde_state().time();
    grid = robertslab::pbuf::NDArraySerializer::deserialize<double>(state.diffusion_pde_state().concentrations(0), sizeof(double));
    if (grid->shape.len != 3) throw lm::InvalidArgException("grid", "the grid must be three-dimensional for ExplicitFiniteDifferenceSolver");
}

lm::message::WorkUnitOutput* ExplicitFiniteDifferenceSolver::getOutput(uint trajectoryNumber)
{
    if (trajectoryNumber > 0) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());

    // Get the output pointer.
    lm::message::WorkUnitOutput* ret = output;

    // Forget about the pointer, since the caller is now responsible for it.
    output = NULL;

    return ret;
}

lm::message::WorkUnitStatus::Status ExplicitFiniteDifferenceSolver::getStatus(uint trajectoryNumber)
{
    if (trajectoryNumber > 0) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());
    return status;
}

long long ExplicitFiniteDifferenceSolver::generateTrajectory(long long maxSteps)
{
    // Get the grid dimensions in various forms.
    const int ilen=(int)grid->shape[0];
    const int jlen=(int)grid->shape[1];
    const int klen=(int)grid->shape[2];
    const int jklen=jlen*klen;
    const int imax=(int)grid->shape[0]-1;
    const int jmax=(int)grid->shape[1]-1;
    const int kmax=(int)grid->shape[2]-1;

    // Allocate space for a second copy in aligned memory.
    double* grid2=NULL;
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&grid2, sizeof(double), grid->size*sizeof(double)));

    // Save pointers to the actual grid locations.
    double* c = grid->values;
    double* cFuture = grid2;

    // Go through the requested steps.
    long long steps=0;
    status = lm::message::WorkUnitStatus::STEPS_FINISHED;
    while (steps < maxSteps)
    {
        // If we have less than a full dt left, adjust tau.
        double tau = (time+dt<=timeLimit)?(dt):(timeLimit-time);

        // Calculate the diffusion constant.
        double k_diff = (D*tau)/(dx*dx);

        // Go through the grid and update each point.
        int index=0;
        double c_im, c_ip, c_jm, c_jp, c_km, c_kp;
        for (int i=0; i<ilen; i++)
            for (int j=0; j<jlen; j++)
                for (int k=0; k<klen; k++, index++)
                {
                    c_im = (i>0)?(c[index-jklen]):(c[index]);
                    c_ip = (i<imax)?(c[index+jklen]):(c[index]);
                    c_jm = (j>0)?(c[index-klen]):(c[index]);
                    c_jp = (j<jmax)?(c[index+klen]):(c[index]);
                    c_km = (k>0)?(c[index-1]):(c[index]);
                    c_kp = (k<kmax)?(c[index+1]):(c[index]);
                    cFuture[index] = c[index] + k_diff*(-6.0*c[index]+c_im+c_ip+c_jm+c_jp+c_km+c_kp);
                }

        // Update the step counter.
        steps++;
        time += tau;

        // See if we are done with the time.
        if (time >= timeLimit)
        {
            status = lm::message::WorkUnitStatus::LIMIT_REACHED;
            break;
        }

        // If we have another step, swap the concentration pointers.
        if (steps < maxSteps)
        {
            double* tmp=c;
            c = cFuture;
            cFuture=tmp;
        }
    }

    // Save the final results, if it is not already in the grid.
    if (grid->values != cFuture)
        memcpy(grid->values, cFuture, grid->size*sizeof(double));

    // Free the second grid memory.
    free(grid2);

    return steps;
}

}
}
