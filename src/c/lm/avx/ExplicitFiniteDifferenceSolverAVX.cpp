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

#include "lm/ClassFactory.h"
#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/avx/ExplicitFiniteDifferenceSolverAVX.h"
#include "robertslab/Types.h"
#include "robertslab/pbuf/NDArraySerializer.h"

namespace lm {
namespace avx {

bool ExplicitFiniteDifferenceSolverAVX::registered=ExplicitFiniteDifferenceSolverAVX::registerClass();

bool ExplicitFiniteDifferenceSolverAVX::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::pde::DiffusionPDESolver","lm::avx::ExplicitFiniteDifferenceSolverAVX",&ExplicitFiniteDifferenceSolverAVX::allocateObject);
    return true;
}

void* ExplicitFiniteDifferenceSolverAVX::allocateObject()
{
    return new ExplicitFiniteDifferenceSolverAVX();
}

ExplicitFiniteDifferenceSolverAVX::ExplicitFiniteDifferenceSolverAVX()
:ExplicitFiniteDifferenceSolver()
{
}

ExplicitFiniteDifferenceSolverAVX::~ExplicitFiniteDifferenceSolverAVX()
{
}

void ExplicitFiniteDifferenceSolverAVX::setState(const lm::io::TrajectoryState& state, uint trajectoryNumber)
{
    if (trajectoryNumber > 0) throw lm::InvalidArgException("trajectoryNumber", "exceeded the maximum number of simultaneous trajectories",trajectoryNumber,getSimultaneousTrajectories());

    time = state.diffusion_pde_state().time();
    grid = robertslab::pbuf::NDArraySerializer::deserialize<double>(state.diffusion_pde_state().concentrations(0), DOUBLES_PER_AVX*sizeof(double));
    if (grid->shape.len != 3) throw lm::InvalidArgException("grid", "the grid must be three-dimensional for ExplicitFiniteDifferenceSolver");
    if (grid->shape[2]%DOUBLES_PER_AVX != 0) throw lm::InvalidArgException("grid", "the grid z dimension was not evenly divisible by the AVX register size for ExplicitFiniteDifferenceSolverAVX", grid->shape[2]);
}

uint64_t ExplicitFiniteDifferenceSolverAVX::generateTrajectory(uint64_t maxSteps)
{
    if (grid->alignment != DOUBLES_PER_AVX*sizeof(double)) throw lm::InvalidArgException("grid", "the grid memory was not aligned correctly for ExplicitFiniteDifferenceSolverAVX", grid->alignment);

    // Get the grid dimensions in various forms.
    const int ilen=(int)grid->shape[0];
    const int jlen=(int)grid->shape[1];
    const int klen=(int)grid->shape[2];
    const int jklen=jlen*klen;
    const int imax=ilen-1;
    const int jmax=jlen-1;
    const int kmax=klen-DOUBLES_PER_AVX;

    // Allocate space for a second copy of the grid in aligned memory.
    double* grid2=NULL;
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&grid2, DOUBLES_PER_AVX*sizeof(double), grid->size*sizeof(double)));

    // Save pointers to the actual grid locations.
    double* c = grid->values;
    double* cFuture = grid2;

    // Go through the requested steps.
    uint64_t steps=0;
    status = lm::message::WorkUnitStatus::STEPS_FINISHED;
    const avxd m6v = _mm256_set1_pd(-6.0);
    while (steps < maxSteps)
    {
        // If we have less than a full dt left, adjust tau.
        double tau = (time+dt<=timeLimit)?(dt):(timeLimit-time);

        // Calculate the diffusion constant.
        double k_diff = (D*tau)/(dx*dx);
        const avxd k_diffv = _mm256_set1_pd(k_diff);

        // Go through the grid and update each point.
        int index;
        avxd c_index, c_im, c_ip, c_jm, c_jp, c_km, c_kp;
        for (int i=0; i<ilen; i++)
            for (int j=0; j<jlen; j++)
                for (int k=0; k<klen; k+=DOUBLES_PER_AVX)
                {
                    index = i*jklen + j*klen + k;

                    c_index = _mm256_load_pd(&c[index]);

                    c_im = (i>0)?(_mm256_load_pd(&c[index-jklen])):(c_index);
                    c_ip = (i<imax)?(_mm256_load_pd(&c[index+jklen])):(c_index);
                    c_jm = (j>0)?(_mm256_load_pd(&c[index-klen])):(c_index);
                    c_jp = (j<jmax)?(_mm256_load_pd(&c[index+klen])):(c_index);
                    c_km = (k>0)?(_mm256_loadu_pd(&c[index-1])):(_mm256_set_pd(c[index],c[index],c[index+1],c[index+2]));
                    c_kp = (k<kmax)?(_mm256_loadu_pd(&c[index+1])):(_mm256_set_pd(c[index+1],c[index+2],c[index+3],c[index+3]));

                    avxd iflux = _mm256_add_pd(c_im,c_ip);
                    avxd jflux = _mm256_add_pd(c_jm,c_jp);
                    avxd kflux = _mm256_add_pd(c_km,c_kp);
                    avxd flux = _mm256_add_pd(iflux, _mm256_add_pd(jflux,kflux));
                    flux = _mm256_fmadd_pd(m6v, c_index, flux);
                    avxd cfi = _mm256_fmadd_pd(k_diffv, flux, c_index);
                    _mm256_store_pd(&cFuture[index], cfi);
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

    // Free the grid memory.
    free(grid2);

    return steps;
}

}
}
