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
    :numberSpecies(numberSpecies),numberReactions(numberReactions),numberSiteTypes(numberSiteTypes),DF(NULL),RL(NULL),latticeSpacing(0.0),latticeXSize(0),latticeYSize(0),latticeZSize(0),particlesPerSite(0),hasBoundaryInflux(false),boundaryInflux(NULL)
{
    DF = new double[numberSiteTypes*numberSiteTypes*numberSpecies];
    RL = new bool[numberReactions*numberSiteTypes];
}

RDMESolver::DiffusionModel::~DiffusionModel()
{
    if (DF != NULL) delete[] DF; DF = NULL;
    if (RL != NULL) delete[] RL; RL = NULL;
    if (boundaryInflux != NULL) delete[] boundaryInflux; boundaryInflux = NULL;
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

    // Create the lattice.
    allocateLattice(diffusionModel->latticeXSize, diffusionModel->latticeYSize, diffusionModel->latticeZSize, diffusionModel->particlesPerSite, diffusionModel->latticeSpacing);

    // Fill in the site types.
    const string sites = dm.initial_lattice().sites();
    lattice->deserializeSitesFrom(sites.data(), sites.size(), (Lattice::SerializationDataOrder)dm.initial_lattice().sites_ordering(), dm.initial_lattice().sites_compressed_deflate());


    if (dm.has_boundary_conditions())
    {
        diffusionModel->boundaryConditions = dm.boundary_conditions();

        // See if we need to use the boundary influx array for a constant concentration boundary.
        if ((!diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.global() == lm::io::BoundaryConditions::FIXED_CONCENTRATION) || (diffusionModel->boundaryConditions.axis_specific_boundaries() && (diffusionModel->boundaryConditions.x_minus() == lm::io::BoundaryConditions::FIXED_CONCENTRATION || diffusionModel->boundaryConditions.x_plus() == lm::io::BoundaryConditions::FIXED_CONCENTRATION || diffusionModel->boundaryConditions.y_minus() == lm::io::BoundaryConditions::FIXED_CONCENTRATION || diffusionModel->boundaryConditions.y_plus() == lm::io::BoundaryConditions::FIXED_CONCENTRATION || diffusionModel->boundaryConditions.z_minus() == lm::io::BoundaryConditions::FIXED_CONCENTRATION || diffusionModel->boundaryConditions.z_plus() == lm::io::BoundaryConditions::FIXED_CONCENTRATION)))
        {
            // Make sure we have the necessary parameters.
            if (!diffusionModel->boundaryConditions.has_boundary_site()) throw Exception("No boundary site type specified for fixed concentration boundaries.");
            if (!diffusionModel->boundaryConditions.has_boundary_species()) throw Exception("No boundary species specified for fixed concentration boundaries.");
            if (!diffusionModel->boundaryConditions.has_boundary_concentration()) throw Exception("No boundary concentration specified for fixed concentration boundaries.");

            site_t boundarySiteType = diffusionModel->boundaryConditions.boundary_site();
            particle_t boundarySpecies = diffusionModel->boundaryConditions.boundary_species();
            double boundarySpeciesCount = diffusionModel->boundaryConditions.boundary_concentration()*NA*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*1000.0;
            double latticeSpacingSquared = diffusionModel->latticeSpacing*diffusionModel->latticeSpacing;

            Print::printf(Print::DEBUG, "Creating boundary fluxes for fixed concentration boundary on one or more axis: %d %d %0.2f",boundarySiteType,boundarySpecies,boundarySpeciesCount);
            diffusionModel->hasBoundaryInflux = true;
            diffusionModel->boundaryInflux = new double[diffusionModel->latticeXSize*diffusionModel->latticeYSize*diffusionModel->latticeZSize];
            for (int i=0; i<diffusionModel->latticeXSize*diffusionModel->latticeYSize*diffusionModel->latticeZSize; i++)
                diffusionModel->boundaryInflux[i] = 0.0;

            bool globalFixedConcentration = (!diffusionModel->boundaryConditions.axis_specific_boundaries() && lm::io::BoundaryConditions::FIXED_CONCENTRATION);
            if (globalFixedConcentration || (diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.x_minus() == lm::io::BoundaryConditions::FIXED_CONCENTRATION))
            {
                int x=0;
                for (int z=0; z<diffusionModel->latticeZSize; z++)
                    for (int y=0; y<diffusionModel->latticeYSize; y++)
                        diffusionModel->boundaryInflux[z*diffusionModel->latticeXSize*diffusionModel->latticeYSize+y*diffusionModel->latticeXSize+x] += boundarySpeciesCount*(diffusionModel->DF[boundarySiteType*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(x,y,z)*reactionModel->numberSpecies + boundarySpecies]/latticeSpacingSquared);
            }
            if (globalFixedConcentration || (diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.x_plus() == lm::io::BoundaryConditions::FIXED_CONCENTRATION))
            {
                int x=diffusionModel->latticeXSize-1;
                for (int z=0; z<diffusionModel->latticeZSize; z++)
                    for (int y=0; y<diffusionModel->latticeYSize; y++)
                        diffusionModel->boundaryInflux[z*diffusionModel->latticeXSize*diffusionModel->latticeYSize+y*diffusionModel->latticeXSize+x] += boundarySpeciesCount*(diffusionModel->DF[boundarySiteType*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(x,y,z)*reactionModel->numberSpecies + boundarySpecies]/latticeSpacingSquared);
            }
            if (globalFixedConcentration || (diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.y_minus() == lm::io::BoundaryConditions::FIXED_CONCENTRATION))
            {
                int y=0;
                for (int z=0; z<diffusionModel->latticeZSize; z++)
                    for (int x=0; x<diffusionModel->latticeXSize; x++)
                        diffusionModel->boundaryInflux[z*diffusionModel->latticeXSize*diffusionModel->latticeYSize+y*diffusionModel->latticeXSize+x] += boundarySpeciesCount*(diffusionModel->DF[boundarySiteType*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(x,y,z)*reactionModel->numberSpecies + boundarySpecies]/latticeSpacingSquared);
            }
            if (globalFixedConcentration || (diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.y_plus() == lm::io::BoundaryConditions::FIXED_CONCENTRATION))
            {
                int y=diffusionModel->latticeYSize-1;
                for (int z=0; z<diffusionModel->latticeZSize; z++)
                    for (int x=0; x<diffusionModel->latticeXSize; x++)
                        diffusionModel->boundaryInflux[z*diffusionModel->latticeXSize*diffusionModel->latticeYSize+y*diffusionModel->latticeXSize+x] += boundarySpeciesCount*(diffusionModel->DF[boundarySiteType*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(x,y,z)*reactionModel->numberSpecies + boundarySpecies]/latticeSpacingSquared);
            }
            if (globalFixedConcentration || (diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.z_minus() == lm::io::BoundaryConditions::FIXED_CONCENTRATION))
            {
                int z=0;
                for (int y=0; y<diffusionModel->latticeYSize; y++)
                    for (int x=0; x<diffusionModel->latticeXSize; x++)
                        diffusionModel->boundaryInflux[z*diffusionModel->latticeXSize*diffusionModel->latticeYSize+y*diffusionModel->latticeXSize+x] += boundarySpeciesCount*(diffusionModel->DF[boundarySiteType*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(x,y,z)*reactionModel->numberSpecies + boundarySpecies]/latticeSpacingSquared);
            }
            if (globalFixedConcentration || (diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.z_plus() == lm::io::BoundaryConditions::FIXED_CONCENTRATION))
            {
                int z=diffusionModel->latticeZSize-1;
                for (int y=0; y<diffusionModel->latticeYSize; y++)
                    for (int x=0; x<diffusionModel->latticeXSize; x++)
                        diffusionModel->boundaryInflux[z*diffusionModel->latticeXSize*diffusionModel->latticeYSize+y*diffusionModel->latticeXSize+x] += boundarySpeciesCount*(diffusionModel->DF[boundarySiteType*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(x,y,z)*reactionModel->numberSpecies + boundarySpecies]/latticeSpacingSquared);
            }
        }

        // See if we need to use the boundary influx array for a constant gradient boundary.
        if ((!diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.global() == lm::io::BoundaryConditions::FIXED_GRADIENT) || (diffusionModel->boundaryConditions.axis_specific_boundaries() && (diffusionModel->boundaryConditions.x_minus() == lm::io::BoundaryConditions::FIXED_GRADIENT || diffusionModel->boundaryConditions.x_plus() == lm::io::BoundaryConditions::FIXED_GRADIENT || diffusionModel->boundaryConditions.y_minus() == lm::io::BoundaryConditions::FIXED_GRADIENT || diffusionModel->boundaryConditions.y_plus() == lm::io::BoundaryConditions::FIXED_GRADIENT || diffusionModel->boundaryConditions.z_minus() == lm::io::BoundaryConditions::FIXED_GRADIENT || diffusionModel->boundaryConditions.z_plus() == lm::io::BoundaryConditions::FIXED_GRADIENT)))
        {
            // Make sure we have the necessary parameters.
            if (!diffusionModel->boundaryConditions.has_boundary_site()) throw Exception("No boundary site type specified for fixed gradient boundaries.");
            if (!diffusionModel->boundaryConditions.has_boundary_species()) throw Exception("No boundary species specified for fixed gradient boundaries.");
            if (!diffusionModel->boundaryConditions.has_boundary_gradient_ordering()) throw Exception("No boundary gradient array ordering specified for fixed gradient boundaries.");
            lm::io::ArrayOrdering dataOrdering = diffusionModel->boundaryConditions.boundary_gradient_ordering();
            if (dataOrdering != lm::io::ROW_MAJOR && dataOrdering != lm::io::COLUMN_MAJOR) throw Exception("Invalid boundary gradient array ordering specified for fixed flux boundaries.");
            if (diffusionModel->boundaryConditions.boundary_gradient_size() != (diffusionModel->latticeXSize+2)*(diffusionModel->latticeYSize+2)*(diffusionModel->latticeZSize+2)) throw Exception("Invalid boundary gradient array specified for fixed gradient boundaries.");

            site_t boundarySiteType = diffusionModel->boundaryConditions.boundary_site();
            particle_t boundarySpecies = diffusionModel->boundaryConditions.boundary_species();
            double latticeSpacingSquared = diffusionModel->latticeSpacing*diffusionModel->latticeSpacing;

            Print::printf(Print::DEBUG, "Creating boundary fluxes for fixed gradient boundary on one or more axis: %d %d",boundarySiteType,boundarySpecies);
            diffusionModel->hasBoundaryInflux = true;
            diffusionModel->boundaryInflux = new double[diffusionModel->latticeXSize*diffusionModel->latticeYSize*diffusionModel->latticeZSize];
            for (int i=0; i<diffusionModel->latticeXSize*diffusionModel->latticeYSize*diffusionModel->latticeZSize; i++)
                diffusionModel->boundaryInflux[i] = 0.0;

            bool globalFixedConcentration = (!diffusionModel->boundaryConditions.axis_specific_boundaries() && lm::io::BoundaryConditions::FIXED_GRADIENT);
            if (globalFixedConcentration || (diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.x_minus() == lm::io::BoundaryConditions::FIXED_GRADIENT))
            {
                int x=0;
                for (int z=0; z<diffusionModel->latticeZSize; z++)
                {
                    for (int y=0; y<diffusionModel->latticeYSize; y++)
                    {
                        int x2=x+1, y2=y+1, z2=z+1;
                        x2-=1;
                        int influxDataIndex = z*diffusionModel->latticeXSize*diffusionModel->latticeYSize+y*diffusionModel->latticeXSize+x;
                        int gradientDataIndex;
                        if (dataOrdering == lm::io::ROW_MAJOR)
                            gradientDataIndex = x2*(diffusionModel->latticeYSize+2)*(diffusionModel->latticeZSize+2) + y2*(diffusionModel->latticeZSize+2) + z2;
                        else
                            gradientDataIndex = z2*(diffusionModel->latticeXSize+2)*(diffusionModel->latticeYSize+2) + y2*(diffusionModel->latticeXSize+2) + x2;
                        double boundarySpeciesCount = diffusionModel->boundaryConditions.boundary_gradient(gradientDataIndex)*NA*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*1000.0;
                        diffusionModel->boundaryInflux[influxDataIndex] += boundarySpeciesCount*(diffusionModel->DF[boundarySiteType*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(x,y,z)*reactionModel->numberSpecies + boundarySpecies]/latticeSpacingSquared);
                    }
                }
            }
            if (globalFixedConcentration || (diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.x_plus() == lm::io::BoundaryConditions::FIXED_GRADIENT))
            {
                int x=diffusionModel->latticeXSize-1;
                for (int z=0; z<diffusionModel->latticeZSize; z++)
                {
                    for (int y=0; y<diffusionModel->latticeYSize; y++)
                    {
                        int x2=x+1, y2=y+1, z2=z+1;
                        x2+=1;
                        int influxDataIndex = z*diffusionModel->latticeXSize*diffusionModel->latticeYSize+y*diffusionModel->latticeXSize+x;
                        int gradientDataIndex;
                        if (dataOrdering == lm::io::ROW_MAJOR)
                            gradientDataIndex = x2*(diffusionModel->latticeYSize+2)*(diffusionModel->latticeZSize+2) + y2*(diffusionModel->latticeZSize+2) + z2;
                        else
                            gradientDataIndex = z2*(diffusionModel->latticeXSize+2)*(diffusionModel->latticeYSize+2) + y2*(diffusionModel->latticeXSize+2) + x2;
                        double boundarySpeciesCount = diffusionModel->boundaryConditions.boundary_gradient(gradientDataIndex)*NA*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*1000.0;
                        diffusionModel->boundaryInflux[influxDataIndex] += boundarySpeciesCount*(diffusionModel->DF[boundarySiteType*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(x,y,z)*reactionModel->numberSpecies + boundarySpecies]/latticeSpacingSquared);
                    }
                }
            }
            if (globalFixedConcentration || (diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.y_minus() == lm::io::BoundaryConditions::FIXED_GRADIENT))
            {
                int y=0;
                for (int z=0; z<diffusionModel->latticeZSize; z++)
                {
                    for (int x=0; x<diffusionModel->latticeXSize; x++)
                    {
                        int x2=x+1, y2=y+1, z2=z+1;
                        y2-=1;
                        int influxDataIndex = z*diffusionModel->latticeXSize*diffusionModel->latticeYSize+y*diffusionModel->latticeXSize+x;
                        int gradientDataIndex;
                        if (dataOrdering == lm::io::ROW_MAJOR)
                            gradientDataIndex = x2*(diffusionModel->latticeYSize+2)*(diffusionModel->latticeZSize+2) + y2*(diffusionModel->latticeZSize+2) + z2;
                        else
                            gradientDataIndex = z2*(diffusionModel->latticeXSize+2)*(diffusionModel->latticeYSize+2) + y2*(diffusionModel->latticeXSize+2) + x2;
                        double boundarySpeciesCount = diffusionModel->boundaryConditions.boundary_gradient(gradientDataIndex)*NA*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*1000.0;
                        diffusionModel->boundaryInflux[influxDataIndex] += boundarySpeciesCount*(diffusionModel->DF[boundarySiteType*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(x,y,z)*reactionModel->numberSpecies + boundarySpecies]/latticeSpacingSquared);
                    }
                }
            }
            if (globalFixedConcentration || (diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.y_plus() == lm::io::BoundaryConditions::FIXED_GRADIENT))
            {
                int y=diffusionModel->latticeYSize-1;
                for (int z=0; z<diffusionModel->latticeZSize; z++)
                {
                    for (int x=0; x<diffusionModel->latticeXSize; x++)
                    {
                        int x2=x+1, y2=y+1, z2=z+1;
                        y2+=1;
                        int influxDataIndex = z*diffusionModel->latticeXSize*diffusionModel->latticeYSize+y*diffusionModel->latticeXSize+x;
                        int gradientDataIndex;
                        if (dataOrdering == lm::io::ROW_MAJOR)
                            gradientDataIndex = x2*(diffusionModel->latticeYSize+2)*(diffusionModel->latticeZSize+2) + y2*(diffusionModel->latticeZSize+2) + z2;
                        else
                            gradientDataIndex = z2*(diffusionModel->latticeXSize+2)*(diffusionModel->latticeYSize+2) + y2*(diffusionModel->latticeXSize+2) + x2;
                        double boundarySpeciesCount = diffusionModel->boundaryConditions.boundary_gradient(gradientDataIndex)*NA*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*1000.0;
                        diffusionModel->boundaryInflux[influxDataIndex] += boundarySpeciesCount*(diffusionModel->DF[boundarySiteType*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(x,y,z)*reactionModel->numberSpecies + boundarySpecies]/latticeSpacingSquared);
                    }
                }
            }
            if (globalFixedConcentration || (diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.z_minus() == lm::io::BoundaryConditions::FIXED_GRADIENT))
            {
                int z=0;
                for (int y=0; y<diffusionModel->latticeYSize; y++)
                {
                    for (int x=0; x<diffusionModel->latticeXSize; x++)
                    {
                        int x2=x+1, y2=y+1, z2=z+1;
                        z2-=1;
                        int influxDataIndex = z*diffusionModel->latticeXSize*diffusionModel->latticeYSize+y*diffusionModel->latticeXSize+x;
                        int gradientDataIndex;
                        if (dataOrdering == lm::io::ROW_MAJOR)
                            gradientDataIndex = x2*(diffusionModel->latticeYSize+2)*(diffusionModel->latticeZSize+2) + y2*(diffusionModel->latticeZSize+2) + z2;
                        else
                            gradientDataIndex = z2*(diffusionModel->latticeXSize+2)*(diffusionModel->latticeYSize+2) + y2*(diffusionModel->latticeXSize+2) + x2;
                        double boundarySpeciesCount = diffusionModel->boundaryConditions.boundary_gradient(gradientDataIndex)*NA*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*1000.0;
                        diffusionModel->boundaryInflux[influxDataIndex] += boundarySpeciesCount*(diffusionModel->DF[boundarySiteType*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(x,y,z)*reactionModel->numberSpecies + boundarySpecies]/latticeSpacingSquared);
                    }
                }
            }
            if (globalFixedConcentration || (diffusionModel->boundaryConditions.axis_specific_boundaries() && diffusionModel->boundaryConditions.z_plus() == lm::io::BoundaryConditions::FIXED_GRADIENT))
            {
                int z=diffusionModel->latticeZSize-1;
                for (int y=0; y<diffusionModel->latticeYSize; y++)
                {
                    for (int x=0; x<diffusionModel->latticeXSize; x++)
                    {
                        int x2=x+1, y2=y+1, z2=z+1;
                        z2+=1;
                        int influxDataIndex = z*diffusionModel->latticeXSize*diffusionModel->latticeYSize+y*diffusionModel->latticeXSize+x;
                        int gradientDataIndex;
                        if (dataOrdering == lm::io::ROW_MAJOR)
                            gradientDataIndex = x2*(diffusionModel->latticeYSize+2)*(diffusionModel->latticeZSize+2) + y2*(diffusionModel->latticeZSize+2) + z2;
                        else
                            gradientDataIndex = z2*(diffusionModel->latticeXSize+2)*(diffusionModel->latticeYSize+2) + y2*(diffusionModel->latticeXSize+2) + x2;
                        double boundarySpeciesCount = diffusionModel->boundaryConditions.boundary_gradient(gradientDataIndex)*NA*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*diffusionModel->latticeSpacing*1000.0;
                        diffusionModel->boundaryInflux[influxDataIndex] += boundarySpeciesCount*(diffusionModel->DF[boundarySiteType*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(x,y,z)*reactionModel->numberSpecies + boundarySpecies]/latticeSpacingSquared);
                    }
                }
            }

            // Uncomment to print out boundary flux array.
            /*
            Print::printf(Print::DEBUG, "Created boundary fluxes.");
            for (lattice_size_t z=0; z<diffusionModel->latticeZSize; z++)
            {
                for (lattice_size_t x=0; x<diffusionModel->latticeXSize; x++)
                {
                    for (lattice_size_t y=0; y<diffusionModel->latticeYSize; y++)
                    {
                        int influxDataIndex = z*diffusionModel->latticeXSize*diffusionModel->latticeYSize+y*diffusionModel->latticeXSize+x;
                        double flux = diffusionModel->boundaryInflux[influxDataIndex];
                        printf("%6.2e%c",flux,y<diffusionModel->latticeYSize-1?',':' ');
                    }
                    printf("\n");
                }
                printf("---------------\n");
            }
            */
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
    l->set_particles_compressed_deflate(true);
    size_t dataSizeEstimate = lattice->serializeParticlesSize(true);
    string* particles=new string();
    particles->resize(dataSizeEstimate);
    size_t dataSizeActual=lattice->serializeParticlesTo(&((*particles)[0]), dataSizeEstimate, Lattice::NATIVE_ORDER, true);
    particles->resize(dataSizeActual);
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
    lattice->deserializeParticlesFrom(particles.data(), particles.size(), (Lattice::SerializationDataOrder)state.rdme_state().species_positions().particles_ordering(), state.rdme_state().species_positions().particles_compressed_deflate());
}

}
}
