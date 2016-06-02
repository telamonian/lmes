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

#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/avx/ExplicitFiniteDifferenceSolverAVX.h"

namespace lm {
namespace avx {

ExplicitFiniteDifferenceSolverAVX::ExplicitFiniteDifferenceSolverAVX(double D, double dx, double dt_arg)
:ExplicitFiniteDifferenceSolver(D,dx,dt_arg)
{
}

ExplicitFiniteDifferenceSolverAVX::~ExplicitFiniteDifferenceSolverAVX()
{
}

void ExplicitFiniteDifferenceSolverAVX::calculate(ndarray<double>& grid, double runtime)
{
    if (grid.shape.len != 3) throw lm::InvalidArgException("grid", "the grid was not three-dimensional for ExplicitFiniteDifferenceSolverAVX", runtime);
    if (grid.shape[2]%DOUBLES_PER_AVX != 0) throw lm::InvalidArgException("grid", "the grid z dimension was not evenly divisible by the AVX register size for ExplicitFiniteDifferenceSolverAVX", grid.shape[2]);
    if (grid.alignment != DOUBLES_PER_AVX) throw lm::InvalidArgException("grid", "the grid memory was not aligned correctly for ExplicitFiniteDifferenceSolverAVX", grid.alignment);

    // If the grid is too small, run using the base solver.
    if (grid.shape[0] < 3 || grid.shape[1] < 3 || grid.shape[2] < 3*DOUBLES_PER_AVX)
    {
        lm::Print::printf(lm::Print::INFO, "The specified grid shape (%d,%d,%d) was too small for the ExplicitFiniteDifferenceSolverAVX solver, using the non-avx version instead.",grid.shape[0],grid.shape[1],grid.shape[2]);
        ExplicitFiniteDifferenceSolver::calculate(grid, runtime);
        return;
    }

    // Figure out how many time steps to run.
    int steps = int(floor((runtime/dt)+0.5));

    // Make sure that runtime is an interval of dt.
    if (fabs(runtime-double(steps)*dt) > 1e-9) throw lm::InvalidArgException("runtime", "the runtime was not a multiple of the timestep for ExplicitFiniteDifferenceSolverAVX", runtime);

    // Allocate space for a second copy in aligned memory.
    double* grid2=NULL;
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&grid2, DOUBLES_PER_AVX*sizeof(double), grid.numberValues*sizeof(double)));

    // Go through the requested steps.
    double* c = grid.values;
    double* cFuture = grid2;
    double k_diff = (D*dt)/(dx*dx);
    avxd k_diffv = _mm256_set1_pd(k_diff);
    avxd m6v = _mm256_set1_pd(-6.0);
    while (steps > 0)
    {

        const int ilen=(int)grid.shape[0];
        const int jlen=(int)grid.shape[1];
        const int klen=(int)grid.shape[2];
        const int jklen=jlen*klen;
        const int imax=ilen-1;
        const int jmax=jlen-1;
        const int kmax=klen-1;

        // Process the interior of the grid using avx.
        {
            int index;
            avxd c_index, c_im, c_ip, c_jm, c_jp, c_km, c_kp;
            for (int i=1; i<ilen-1; i++)
                for (int j=1; j<jlen-1; j++)
                    for (int k=DOUBLES_PER_AVX; k<klen-DOUBLES_PER_AVX; k+=DOUBLES_PER_AVX)
                    {
                        index = i*jklen + j*klen + k;

                        c_index = _mm256_load_pd(&c[index]);
                        c_im = _mm256_load_pd(&c[index-jklen]);
                        c_ip = _mm256_load_pd(&c[index+jklen]);
                        c_jm = _mm256_load_pd(&c[index-klen]);
                        c_jp = _mm256_load_pd(&c[index+klen]);
                        c_km = _mm256_loadu_pd(&c[index-1]);
                        c_kp = _mm256_loadu_pd(&c[index+1]);

                        avxd iflux = _mm256_add_pd(c_im,c_ip);
                        avxd jflux = _mm256_add_pd(c_jm,c_jp);
                        avxd kflux = _mm256_add_pd(c_km,c_kp);
                        avxd flux = _mm256_add_pd(iflux, _mm256_add_pd(jflux,kflux));
                        flux = _mm256_fmadd_pd(m6v, c_index, flux);
                        avxd cfi = _mm256_fmadd_pd(k_diffv, flux, c_index);
                        _mm256_store_pd(&cFuture[index], cfi);
                    }
        }

        // Process the boundary layer of the grid outside of avx.
        {
            int index;
            double c_im, c_ip, c_jm, c_jp, c_km, c_kp;

            // Process the -z and +z faces.
            for (int i=0; i<ilen; i++)
                for (int j=0; j<jlen; j++)
                    for (int k=0; k<klen; k++)
                    {
                        // If we are finished with the -z layer, skip to the +z.
                        if (k == DOUBLES_PER_AVX) k = klen-DOUBLES_PER_AVX;

                        index = i*jklen + j*klen + k;
                        c_im = (i>0)?(c[index-jklen]):(c[index]);
                        c_ip = (i<imax)?(c[index+jklen]):(c[index]);
                        c_jm = (j>0)?(c[index-klen]):(c[index]);
                        c_jp = (j<jmax)?(c[index+klen]):(c[index]);
                        c_km = (k>0)?(c[index-1]):(c[index]);
                        c_kp = (k<kmax)?(c[index+1]):(c[index]);
                        cFuture[index] = c[index] + k_diff*(-6.0*c[index]+c_im+c_ip+c_jm+c_jp+c_km+c_kp);
                    }

            // Process the -y and +y faces.
            for (int i=0; i<ilen; i+=ilen-1)
                for (int j=0; j<jlen; j++)
                    for (int k=DOUBLES_PER_AVX; k<klen-DOUBLES_PER_AVX; k++)
                    {
                        index = i*jklen + j*klen + k;
                        c_im = (i>0)?(c[index-jklen]):(c[index]);
                        c_ip = (i<imax)?(c[index+jklen]):(c[index]);
                        c_jm = (j>0)?(c[index-klen]):(c[index]);
                        c_jp = (j<jmax)?(c[index+klen]):(c[index]);
                        c_km = (k>0)?(c[index-1]):(c[index]);
                        c_kp = (k<kmax)?(c[index+1]):(c[index]);
                        cFuture[index] = c[index] + k_diff*(-6.0*c[index]+c_im+c_ip+c_jm+c_jp+c_km+c_kp);
                    }

            // Process the -x and +x faces.
            for (int i=1; i<ilen-1; i++)
                for (int j=0; j<jlen; j+=jlen-1)
                    for (int k=DOUBLES_PER_AVX; k<klen-DOUBLES_PER_AVX; k++)
                    {
                        index = i*jklen + j*klen + k;
                        c_im = (i>0)?(c[index-jklen]):(c[index]);
                        c_ip = (i<imax)?(c[index+jklen]):(c[index]);
                        c_jm = (j>0)?(c[index-klen]):(c[index]);
                        c_jp = (j<jmax)?(c[index+klen]):(c[index]);
                        c_km = (k>0)?(c[index-1]):(c[index]);
                        c_kp = (k<kmax)?(c[index+1]):(c[index]);
                        cFuture[index] = c[index] + k_diff*(-6.0*c[index]+c_im+c_ip+c_jm+c_jp+c_km+c_kp);
                    }
        }

        // Update the step counter.
        steps--;

        // If we have another step, swap the concentration pointers.
        if (steps > 0)
        {
            double* tmp=c;
            c = cFuture;
            cFuture=tmp;
        }
    }

    // Save the final results, if it is not already in the grid.
    if (grid.values != cFuture)
        memcpy(grid.values, cFuture, grid.numberValues*sizeof(double));

    // Free the second grid memory.
    free(grid2);
}

}
}
