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

#ifndef LM_PDE_DIFFUSIONPDESOLVER_H
#define LM_PDE_DIFFUSIONPDESOLVER_H

#include "lm/Types.h"

namespace lm {
namespace pde {

class DiffusionPDESolver
{
public:
    enum BoundaryConditions {REFLECTING              = 0,
                             ABSORBING               = 1,
                             PERIODIC                = 2,
                             FIXED_CONCENTRATION     = 3,
                             FIXED_GRADIENT          = 4,
                             LINEAR_GRADIENT         = 5};
public:
    DiffusionPDESolver();
    virtual ~DiffusionPDESolver();
    virtual void calculate(ndarray<double>& domain, double time)=0;

    virtual BoundaryConditions getBoundaryConditions() {return boundaries;}
    virtual void setBoundaryConditions(BoundaryConditions boundaries) {this->boundaries = boundaries;}

protected:
    BoundaryConditions boundaries;
};

}
}
#endif // LM_PDE_DIFFUSIONPDESOLVER_H
