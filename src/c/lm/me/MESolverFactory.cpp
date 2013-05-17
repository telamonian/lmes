/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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
 * Author(s): Elijah Roberts
 */

#include <string>
#include "lm/Exceptions.h"
#include "lm/cme/FluctuatingNRSolver.h"
#include "lm/cme/GillespieDSolver.h"
#include "lm/cme/HillSwitch.h"
#include "lm/cme/LacHillSwitch.h"
#include "lm/cme/SelfRegulatingGeneSwitch.h"
#include "lm/cme/TwoStateExpression.h"
#include "lm/cme/TwoStateHillSwitch.h"
#include "lm/cme/TwoStateHillLoopSwitch.h"
#include "lm/cme/GillespieDSolver.h"
#include "lm/me/MESolver.h"
#include "lm/me/MESolverFactory.h"
#ifdef OPT_CUDA
#include "lm/rdme/MpdRdmeSolver.h"
#endif
#include "lm/rdme/NextSubvolumeSolver.h"

using std::string;

namespace lm {
namespace me {

MESolverFactory::MESolverFactory()
:solver("")
{
}

void MESolverFactory::setSolver(string solver) throw(lm::InvalidArgException)
{
    if (solver == "lm::cme::FluctuatingNRSolver" || \
        solver == "lm::cme::GillespieDSolver" || \
        solver == "lm::cme::HillSwitch" || \
        solver == "lm::cme::LacHillSwitch" || \
        solver == "lm::cme::NextReactionSolver" || \
        solver == "lm::cme::SelfRegulatingGeneSwitch" || \
        solver == "lm::cme::TwoStateExpression" || \
        solver == "lm::cme::TwoStateHillSwitch" || \
        solver == "lm::cme::TwoStateHillLoopSwitch" || \
        solver == "lm::rdme::MpdRdmeSolver" || \
        solver == "lm::rdme::NextSubvolumeSolver")
    {
        this->solver = solver;
        return;
    }
    throw lm::InvalidArgException("solver", "The specified solver is not known", solver.c_str());
}

bool MESolverFactory::needsReactionModel() throw(lm::Exception)
{
    MESolver * solver = instantiate();
    bool ret = solver->needsReactionModel();
    delete solver;
    return ret;
}

bool MESolverFactory::needsDiffusionModel() throw(lm::Exception)
{
    MESolver * solver = instantiate();
    bool ret = solver->needsDiffusionModel();
    delete solver;
    return ret;
}

MESolver * MESolverFactory::instantiate() throw(lm::Exception)
{
    // Run the simulation using the specified model.
    if (solver == "lm::cme::FluctuatingNRSolver")
    {
        return new lm::cme::FluctuatingNRSolver;
    }
    else if (solver == "lm::cme::GillespieDSolver")
    {
        return new lm::cme::GillespieDSolver;
    }
    else if (solver == "lm::cme::HillSwitch")
    {
        return new lm::cme::HillSwitch;
    }
    else if (solver == "lm::cme::LacHillSwitch")
    {
        return new lm::cme::LacHillSwitch;
    }
    else if (solver == "lm::cme::NextReactionSolver")
    {
        return new lm::cme::NextReactionSolver;
    }
    else if (solver == "lm::cme::SelfRegulatingGeneSwitch")
    {
        return new lm::cme::SelfRegulatingGeneSwitch;
    }
    else if (solver == "lm::cme::TwoStateExpression")
    {
        return new lm::cme::TwoStateExpression;
    }
    else if (solver == "lm::cme::TwoStateHillLoopSwitch")
    {
        return new lm::cme::TwoStateHillLoopSwitch;
    }
    else if (solver == "lm::cme::TwoStateHillSwitch")
    {
        return new lm::cme::TwoStateHillSwitch;
    }
    #ifdef OPT_CUDA
    else if (solver == "lm::rdme::MpdRdmeSolver")
    {
        return new lm::rdme::MpdRdmeSolver;
    }
    #else
    else if (solver == "lm::rdme::MpdRdmeSolver")
    {
        throw lm::Exception("The specified solver is only available using CUDA:", solver.c_str());
    }
    #endif
    else if (solver == "lm::rdme::NextSubvolumeSolver")
    {
        return new lm::rdme::NextSubvolumeSolver;
    }
    else
    {
        throw lm::Exception("The specified solver is unknown:", solver.c_str());
    }
}

}
}
