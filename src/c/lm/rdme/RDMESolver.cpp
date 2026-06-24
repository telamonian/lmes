/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
 *
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
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
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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

#include "lm/Exceptions.h"
#include "lm/Tune.h"
#include "lm/Print.h"
#include "lm/cme/CMESolver.h"
#include "lm/types/BoundaryConditions.pb.h"
#include "lm/input/DiffusionModel.pb.h"
#include "lm/types/Lattice.pb.h"
#include "lm/me/PropensityFunction.h"
#include "lm/rdme/Lattice.h"
#include "lm/rdme/ByteLattice.h"
#include "lm/rdme/DiffusionModel.h"
#include "lm/rdme/RDMESolver.h"
#include "lm/rng/RandomGenerator.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"
#include "robertslab/pbuf/NDArraySerializer.h"

using lm::input::DiffusionModel;
using lm::rdme::Lattice;
using lm::rng::RandomGenerator;
using robertslab::pbuf::NDArraySerializer;

namespace lm {
namespace rdme {

RDMESolver::RDMESolver(RandomGenerator::Distributions neededDists)
:CMESolver(neededDists),diffusionModel(NULL),lattice(NULL)
{
}

RDMESolver::~RDMESolver()
{
    // Free any model memory.
    if (diffusionModel != NULL) delete diffusionModel; diffusionModel = NULL;

    // Free any memory associated with the state.
    if (lattice != NULL) delete lattice; lattice = NULL;
}

void RDMESolver::setDiffusionModel(const lm::input::DiffusionModel& dm)
{
    CMESolver::setDiffusionModel(dm);

    // Validate the model.
    if (dm.number_species() != (int)reactionModel->numberSpecies) throw InvalidArgException("dm.number_species", "number of species in the diffusion model does not agree with the number in the reaction model");
    if (dm.number_reactions() != (int)reactionModel->numberReactions) throw InvalidArgException("dm.number_reactions", "number of reactions in the diffusion model does not agree with the number in the reaction model");
    if (dm.diffusion_matrix_size() != (dm.number_site_types()*dm.number_site_types()*dm.number_species())) throw InvalidArgException("dm", "diffusion matrix size does not agree with the number of species and site types");
    if (dm.reaction_location_matrix_size() != (dm.number_reactions() *dm.number_site_types())) throw InvalidArgException("dm", "reaction location matrix size does not agree with the number of reactions and site types");

    // Create the new model.
    if (diffusionModel != NULL) delete diffusionModel;
    diffusionModel = new DiffusionModel(dm);

    // Create the lattice.
    allocateLattice(diffusionModel->latticeXSize, diffusionModel->latticeYSize, diffusionModel->latticeZSize, diffusionModel->particlesPerSite, diffusionModel->latticeSpacing);

    // Update the propensity functions with the subvolume size.
    uint numberSubvolumes = lattice->getNumberSites();
    for (uint i=0; i<reactionModel->numberReactions; i++)
    {
        reactionModel->propensityFunctions[i]->changeVolume(1/double(numberSubvolumes));
        if (reactionModel->propensityFunctions[i]->getOrder() >= 3)
        {
            throw Exception("The following reaction propensity function type and order is not supported for RDME simulations",reactionModel->propensityFunctions[i]->getType(),reactionModel->propensityFunctions[i]->getOrder());
        }
    }
}

void RDMESolver::allocateLattice(lattice_size_t latticeXSize, lattice_size_t latticeYSize, lattice_size_t latticeZSize, site_size_t particlesPerSite, si_dist_t latticeSpacing)
{
    lattice = new ByteLattice(latticeXSize, latticeYSize, latticeZSize, latticeSpacing, particlesPerSite);
}

void RDMESolver::reset()
{
    if (diffusionModel == NULL || lattice == NULL) throw Exception("RDMESolver reset called before diffusion model was set.");

    CMESolver::reset();

    // Free any previous state.
    lattice->removeAllParticles();
}

void RDMESolver::getState(lm::io::TrajectoryState* state, uint trajectoryNumber)
{
    if (diffusionModel == NULL || lattice == NULL) throw Exception("RDMESolver get state called before diffusion model was set.");

    CMESolver::getState(state, trajectoryNumber);

    // Get the lattice sites.
    ndarray<uint8_t>* sitesBuffer = new ndarray<uint8_t>(utuple(lattice->getSize().x,lattice->getSize().y,lattice->getSize().z), 0, ndarray_ArrayOrder::IMPL_ORDER);
    lattice->copySitesTo(sitesBuffer);
    state->mutable_rdme_state()->mutable_lattice()->set_allocated_sites(NDArraySerializer::serializeAllocate(*sitesBuffer));
    delete sitesBuffer;
    sitesBuffer = NULL;

    // Get the lattice particles.
    ndarray<uint8_t>* particlesBuffer = new ndarray<uint8_t>(utuple(lattice->getSize().x,lattice->getSize().y,lattice->getSize().z,lattice->getMaxOccupancy()), 0, ndarray_ArrayOrder::IMPL_ORDER);
    lattice->copyParticlesTo(particlesBuffer);
    state->mutable_rdme_state()->mutable_lattice()->set_allocated_particles(NDArraySerializer::serializeAllocate(*particlesBuffer));
    delete particlesBuffer;
    particlesBuffer = NULL;
}

void RDMESolver::setState(const lm::io::TrajectoryState& state, uint trajectoryNumber)
{
    // Valdiate the state.
    if (diffusionModel == NULL || lattice == NULL) throw Exception("RDMESolver set state called before diffusion model was set.");
    if (!state.has_rdme_state()) throw Exception("State object does not contain rdme state to initialize the RDMESolver.");
    if (!state.rdme_state().lattice().has_sites()) throw Exception("State object does not contain the lattice sites to initialize the RDMESolver.");
    if (state.rdme_state().lattice().particles().shape(0) != lattice->getSize().x) throw Exception("State object and lattice have differing lattice x size",state.rdme_state().lattice().particles().shape(0),lattice->getSize().x);
    if (state.rdme_state().lattice().particles().shape(1) != lattice->getSize().y) throw Exception("State object and lattice have differing lattice y size",state.rdme_state().lattice().particles().shape(1),lattice->getSize().y);
    if (state.rdme_state().lattice().particles().shape(2) != lattice->getSize().z) throw Exception("State object and lattice have differing lattice z size",state.rdme_state().lattice().particles().shape(2),lattice->getSize().z);
    if (state.rdme_state().lattice().particles().shape(3) != lattice->getMaxOccupancy()) throw Exception("State object and lattice have differing number of particles per site",state.rdme_state().lattice().particles().shape(3),lattice->getMaxOccupancy());
    if (state.rdme_state().lattice().sites().shape(0) != lattice->getSize().x) throw Exception("State object and lattice have differing lattice sites x size",state.rdme_state().lattice().sites().shape(0),lattice->getSize().x);
    if (state.rdme_state().lattice().sites().shape(1) != lattice->getSize().y) throw Exception("State object and lattice have differing lattice sites y size",state.rdme_state().lattice().sites().shape(1),lattice->getSize().y);
    if (state.rdme_state().lattice().sites().shape(2) != lattice->getSize().z) throw Exception("State object and lattice have differing lattice sites z size",state.rdme_state().lattice().sites().shape(2),lattice->getSize().z);

    CMESolver::setState(state, trajectoryNumber);

    // Set the lattice sites.
    ndarray<uint8_t>* sitesBuffer = NDArraySerializer::deserializeAllocate<uint8_t>(state.rdme_state().lattice().sites());
    lattice->copySitesFrom(sitesBuffer);
    delete sitesBuffer;
    sitesBuffer = NULL;

    // Set the lattice particles.
    ndarray<uint8_t>* particlesBuffer = NDArraySerializer::deserializeAllocate<uint8_t>(state.rdme_state().lattice().particles());
    lattice->copyParticlesFrom(particlesBuffer);
    delete particlesBuffer;
    particlesBuffer = NULL;
}

void RDMESolver::setOutputOptions(const lm::input::OutputOptions& outputOptions)
{
    CMESolver::setOutputOptions(outputOptions);

    if (outputOptions.has_lattice_write_interval())
    {
        writeLatticeTimeSeries = true;
        latticeWriteInterval = outputOptions.lattice_write_interval();
    }
}

}
}
