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

#ifdef OPT_AVX

#ifndef LM_AVX_EXPLICITFINITEDIFFERENCESOLVERAVX_H
#define LM_AVX_EXPLICITFINITEDIFFERENCESOLVERAVX_H

#include "lm/pde/DiffusionPDESolver.h"
#include "lm/pde/ExplicitFiniteDifferenceSolver.h"

namespace lm {
namespace avx {

class ExplicitFiniteDifferenceSolverAVX : public lm::pde::ExplicitFiniteDifferenceSolver
{
public:
    ExplicitFiniteDifferenceSolverAVX(double D, double dx, double dt=0.0);
    virtual ~ExplicitFiniteDifferenceSolverAVX();
    virtual void calculate(ndarray<double>& domain, double runtime);

protected:
    //void calculateWithReflectingBoundary(ndarray<double>& grid, double runtime);
    //virtual void calculateAbsorbingBoundary(ndarray<double>& grid, double time, double value);
};

}
}

#endif // LM_AVX_EXPLICITFINITEDIFFERENCESOLVERAVX_H
#endif
