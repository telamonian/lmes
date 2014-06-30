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
#include "lm/io/BoundaryConditions.pb.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/Lattice.pb.h"
#include "lm/rdme/Lattice.h"
#include "lm/rdme/ByteLattice.h"
#include "lm/rdme/RDMESolver.h"
#include "lm/rng/RandomGenerator.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using lm::io::DiffusionModel;
using lm::rdme::Lattice;
using lm::rng::RandomGenerator;

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

RDMESolver::DiffusionModel::DiffusionModel(int numberSpecies, int numberReactions, int numberSiteTypes)
:numberSpecies(numberSpecies),numberReactions(numberReactions),numberSiteTypes(numberSiteTypes),DF(NULL),RL(NULL),latticeSpacing(0.0),latticeXSize(0),latticeYSize(0),latticeZSize(0),particlesPerSite(0),boundaryConditions(PERIODIC),periodicBoundary(false)
{
    DF = new double[numberSiteTypes*numberSiteTypes*numberSpecies];
    RL = new bool[numberReactions*numberSiteTypes];
}

RDMESolver::DiffusionModel::~DiffusionModel()
{
    if (DF != NULL) delete[] DF; DF = NULL;
    if (RL != NULL) delete[] RL; RL = NULL;
}

void RDMESolver::setDiffusionModel(const lm::io::DiffusionModel& dm)
{
    CMESolver::setDiffusionModel(dm);

    // Validate the model.
    if (dm.number_species() != (int)reactionModel->numberSpecies) throw InvalidArgException("dm.number_species", "number of species in the diffusion model does not agree with the number in the reaction model");
    if (dm.number_reactions() != (int)reactionModel->numberReactions) throw InvalidArgException("dm.number_reactions", "number of reactions in the diffusion model does not agree with the number in the reaction model");
    if (dm.diffusion_matrix_size() != (dm.number_site_types()*dm.number_site_types()*dm.number_species())) throw InvalidArgException("dm", "diffusion matrix size does not agree with the number of species and site types");
    if (dm.reaction_location_matrix_size() != (dm.number_reactions() *dm.number_site_types())) throw InvalidArgException("dm", "reaction location matrix size does not agree with the number of reactions and site types");

    // Create the new model.
    if (diffusionModel != NULL) delete diffusionModel;
    diffusionModel = new DiffusionModel(dm.number_species(), dm.number_reactions(), dm.number_site_types());

    // Populate the model.
    for (int i=0; i<diffusionModel->numberSiteTypes*diffusionModel->numberSiteTypes*diffusionModel->numberSpecies; i++)
        diffusionModel->DF[i] = dm.diffusion_matrix(i);
    for (int i=0; i<diffusionModel->numberReactions*diffusionModel->numberSiteTypes; i++)
        diffusionModel->RL[i] = dm.reaction_location_matrix(i);
    diffusionModel->latticeSpacing = dm.lattice_spacing();
    diffusionModel->latticeXSize = dm.initial_lattice().lattice_x_size();
    diffusionModel->latticeYSize = dm.initial_lattice().lattice_y_size();
    diffusionModel->latticeZSize = dm.initial_lattice().lattice_z_size();
    diffusionModel->particlesPerSite = dm.initial_lattice().particles_per_site();
    if (dm.has_boundary_conditions())
    {
        diffusionModel->boundaryConditions = (DiffusionModel::BoundaryConditions)dm.boundary_conditions().global();
        diffusionModel->periodicBoundary = diffusionModel->boundaryConditions == DiffusionModel::PERIODIC;
    }

    // Create the lattice.
    allocateLattice(diffusionModel->latticeXSize, diffusionModel->latticeYSize, diffusionModel->latticeZSize, diffusionModel->particlesPerSite, diffusionModel->latticeSpacing);

    // Fill in the site types.
    const string sites = dm.initial_lattice().sites();
    lattice->deserializeSitesFrom(sites.data(), sites.size(), (Lattice::SerializationDataOrder)dm.initial_lattice().sites_ordering());
}

void RDMESolver::allocateLattice(lattice_size_t latticeXSize, lattice_size_t latticeYSize, lattice_size_t latticeZSize, site_size_t particlesPerSite, si_dist_t latticeSpacing)
{
    lattice = new ByteLattice(latticeXSize, latticeYSize, latticeZSize, latticeSpacing, particlesPerSite);
}

//void RDMESolver::buildDiffusionModel(const uint numberSiteTypesA, const double * DFA, const uint * RLA, lattice_size_t latticeXSize, lattice_size_t latticeYSize, lattice_size_t latticeZSize, site_size_t particlesPerSite, si_dist_t latticeSpacing, const uint8_t * latticeData, const uint8_t * latticeSitesData, bool rowMajorData) throw(InvalidArgException)
//{
//    // Set the lattice.
//    if (rowMajorData)
//    {
//        lattice_coord_t s = lattice->getSize();
//        site_size_t p = lattice->getMaxOccupancy();

//        // Set the lattice data.
//        for (uint i=0, index=0; i<s.x; i++)
//        {
//            for (uint j=0; j<s.y; j++)
//            {
//                for (uint k=0; k<s.z; k++)
//                {
//                    for (uint l=0; l<p; l++, index++)
//                    {
//                        if (latticeData[index] != 0)
//                        {
//                            if (latticeData[index] > numberSpecies) throw InvalidArgException("latticeData", "an invalid species was found",latticeData[index]);
//                            lattice->addParticle(i,j,k,latticeData[index]);
//                        }
//                    }
//                }
//            }
//        }

//        // Set the lattice sites.
//        for (uint i=0, index=0; i<s.x; i++)
//        {
//            for (uint j=0; j<s.y; j++)
//            {
//                for (uint k=0; k<s.z; k++, index++)
//                {
//                    if (latticeSitesData[index] != 0)
//                    {
//                        if (latticeSitesData[index] >= numberSiteTypes) throw InvalidArgException("latticeSitesData", "an invalid species was found",latticeSitesData[index]);
//                        lattice->setSiteType(i,j,k,latticeSitesData[index]);
//                    }
//                }
//            }
//        }
//    }
//    else
//    {
//        throw lm::InvalidArgException("rowMajorData","columns major lattice data is not currently supported");
//    }

//    Print::printf(Print::DEBUG, "Set diffusion model.");
//    printf("almost done\n");
//}

void RDMESolver::reset()
{
    if (diffusionModel == NULL || lattice == NULL) throw Exception("RDMESolver reset called before diffusion model was set.");

    CMESolver::reset();

    // Free any previous state.
    lattice->removeAllParticles();
}

void RDMESolver::getState(lm::io::TrajectoryState* state)
{
    if (diffusionModel == NULL || lattice == NULL) throw Exception("RDMESolver get state called before diffusion model was set.");

    CMESolver::getState(state);

    // Get the lattice state.
    lm::io::Lattice* l = state->mutable_rdme_state()->mutable_species_positions();
    l->set_particles_ordering(lm::io::NATIVE_ORDER);
    l->set_lattice_x_size(diffusionModel->latticeXSize);
    l->set_lattice_y_size(diffusionModel->latticeYSize);
    l->set_lattice_z_size(diffusionModel->latticeZSize);
    l->set_particles_per_site(diffusionModel->particlesPerSite);
    string* particles=new string();
    particles->resize(lattice->serializeParticlesSize());
    lattice->serializeParticlesTo(&((*particles)[0]), particles->size(), Lattice::NATIVE_ORDER);
    l->set_allocated_particles(particles);
}

void RDMESolver::setState(const lm::io::TrajectoryState& state)
{
    // Valdiate the state.
    if (diffusionModel == NULL || lattice == NULL) throw Exception("RDMESolver set state called before diffusion model was set.");
    if (!state.has_rdme_state()) throw Exception("State object does not contain the necessary data to initialize the RDMESolver.");
    if (state.rdme_state().species_positions().lattice_x_size() != diffusionModel->latticeXSize) throw Exception("State object and diffusion model have differing lattice x size",state.rdme_state().species_positions().lattice_x_size(),diffusionModel->latticeXSize);
    if (state.rdme_state().species_positions().lattice_y_size() != diffusionModel->latticeYSize) throw Exception("State object and diffusion model have differing lattice y size",state.rdme_state().species_positions().lattice_y_size(),diffusionModel->latticeYSize);
    if (state.rdme_state().species_positions().lattice_z_size() != diffusionModel->latticeZSize) throw Exception("State object and diffusion model have differing lattice z size",state.rdme_state().species_positions().lattice_z_size(),diffusionModel->latticeZSize);
    if (state.rdme_state().species_positions().particles_per_site() != diffusionModel->particlesPerSite) throw Exception("State object and diffusion model have differing number of particles per site",state.rdme_state().species_positions().particles_per_site(),diffusionModel->particlesPerSite);

    CMESolver::setState(state);

    // Set the lattice state.
    const string particles = state.rdme_state().species_positions().particles();
    lattice->deserializeParticlesFrom(particles.data(), particles.size(), (Lattice::SerializationDataOrder)state.rdme_state().species_positions().particles_ordering());
}

}
}
