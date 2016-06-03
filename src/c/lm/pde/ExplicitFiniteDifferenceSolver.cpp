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

#include "hrtime.h"
#include "lm/Exceptions.h"
#include "lm/Types.h"
#include "lm/pde/ExplicitFiniteDifferenceSolver.h"

namespace lm {
namespace pde {

ExplicitFiniteDifferenceSolver::ExplicitFiniteDifferenceSolver(double D, double dx, double dt_arg)
:D(D),dx(dx),dt(dt_arg)
{
    // Validate the arguments.
    if (D <= 0.0) throw lm::InvalidArgException("dx", "The diffusion coefficient must be positive.", dt);
    if (dx <= 0.0) throw lm::InvalidArgException("dx", "The grid length must be positive.", dt);
    if (dt < 0.0) throw lm::InvalidArgException("dt", "The timestep was negative.", dt);

    // If the user didn't specify a dt, figure it out.
    if (dt == 0.0) dt = (dx*dx)/(6*D*2);

    // Make sure the stability criteria holds.
    if (dt > (dx*dx)/(6*D)) throw lm::InvalidArgException("dt", "The timestep did not follow obey stability criteria for the ExplicitFiniteDifferenceSolver.", dt);
}

ExplicitFiniteDifferenceSolver::~ExplicitFiniteDifferenceSolver()
{
}

void ExplicitFiniteDifferenceSolver::calculate(ndarray<double>& grid, double runtime)
{
    if (grid.shape.len != 3) throw lm::InvalidArgException("grid", "The grid was not three-dimensional for ExplicitFiniteDifferenceSolver.", runtime);

    // Figure out how many time steps to run.
    int steps = int(floor((runtime/dt)+0.5));

    // Make sure that runtime is an interval of dt.
    if (fabs(runtime-double(steps)*dt) > 1e-9) throw lm::InvalidArgException("runtime", "The runtime was not a multiple of the timestep for ExplicitFiniteDifferenceSolver.", runtime);

    // Allocate space for a second copy in aligned memory.
    double* grid2=NULL;
    POSIX_EXCEPTION_CHECK(posix_memalign((void**)&grid2, sizeof(double), grid.size*sizeof(double)));

    const int ilen=(int)grid.shape[0];
    const int jlen=(int)grid.shape[1];
    const int klen=(int)grid.shape[2];
    const int jklen=jlen*klen;
    const int imax=(int)grid.shape[0]-1;
    const int jmax=(int)grid.shape[1]-1;
    const int kmax=(int)grid.shape[2]-1;

    // Go through the requested steps.
    double* c = grid.values;
    double* cFuture = grid2;
    double k_diff = (D*dt)/(dx*dx);
    while (steps > 0)
    {
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
        memcpy(grid.values, cFuture, grid.size*sizeof(double));

    // Free the second grid memory.
    free(grid2);
}

}
}
