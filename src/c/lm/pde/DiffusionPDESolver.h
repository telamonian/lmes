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
#include "lm/input/MicroenvironmentModel.pb.h"
#include "lm/main/Solver.h"

namespace lm {
namespace pde {

class DiffusionPDESolver : public lm::main::Solver
{
public:
    DiffusionPDESolver();
    virtual ~DiffusionPDESolver();
    virtual void setMicroenvironmentModel(const lm::input::MicroenvironmentModel& model)=0;
};

}
}
#endif // LM_PDE_DIFFUSIONPDESOLVER_H
