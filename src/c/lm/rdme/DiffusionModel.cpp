/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
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
 * - Neither the names of the Roberts Group, Johns Hopkins University,
 * nor the names of its contributors may be used to endorse or
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

#include "lm/Math.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/types/BoundaryConditions.pb.h"
#include "lm/input/DiffusionModel.pb.h"
#include "lm/rdme/DiffusionModel.h"
#include "lm/rdme/ByteLattice.h"
#include "robertslab/Types.h"
#include "robertslab/pbuf/NDArraySerializer.h"

using std::string;
using lm::input::DiffusionModel;

namespace lm {
namespace rdme {

DiffusionModel::DiffusionModel(const lm::input::DiffusionModel& dm)
:numberSpecies((uint)dm.number_species()),numberReactions((uint)dm.number_reactions()),numberSiteTypes((uint)dm.number_site_types()),DF(NULL),RL(NULL),latticeSpacing(dm.lattice_spacing()),latticeXSize(dm.initial_lattice().particles().shape(0)),latticeYSize(dm.initial_lattice().particles().shape(1)),latticeZSize(dm.initial_lattice().particles().shape(2)),particlesPerSite(dm.initial_lattice().particles().shape(3)),sites(NULL),hasBoundaryInflux(false),boundaryInflux(NULL)
{
    // Allocate the matrices.
    DF = new double[numberSiteTypes*numberSiteTypes*numberSpecies];
    RL = new bool[numberReactions*numberSiteTypes];

    // Fill in the matrices.
    for (int i=0; i<numberSiteTypes*numberSiteTypes*numberSpecies; i++)
        DF[i] = dm.diffusion_matrix(i);
    for (int i=0; i<numberReactions*numberSiteTypes; i++)
        RL[i] = dm.reaction_location_matrix(i);

    // Get the initial site types.
    sites = robertslab::pbuf::NDArraySerializer::deserializeAllocate<uint8_t>(dm.initial_lattice().sites());

    // If no boundary conditinos were specified, set the default.
    if (!dm.has_boundary_conditions())
    {
        boundaryConditions.set_global(lm::types::BoundaryConditions::REFLECTING);
    }
    // Otherwise, copy the boundary conditions from the message.
    else
    {
        boundaryConditions = dm.boundary_conditions();

        // See if we need to use the boundary influx array for a constant concentration boundary.
        if ((!boundaryConditions.axis_specific_boundaries() && boundaryConditions.global() == lm::types::BoundaryConditions::FIXED_CONCENTRATION) || (boundaryConditions.axis_specific_boundaries() && (boundaryConditions.x_minus() == lm::types::BoundaryConditions::FIXED_CONCENTRATION || boundaryConditions.x_plus() == lm::types::BoundaryConditions::FIXED_CONCENTRATION || boundaryConditions.y_minus() == lm::types::BoundaryConditions::FIXED_CONCENTRATION || boundaryConditions.y_plus() == lm::types::BoundaryConditions::FIXED_CONCENTRATION || boundaryConditions.z_minus() == lm::types::BoundaryConditions::FIXED_CONCENTRATION || boundaryConditions.z_plus() == lm::types::BoundaryConditions::FIXED_CONCENTRATION)))
        {
            // Make sure we have the necessary parameters.
            if (!boundaryConditions.has_boundary_site()) throw Exception("No boundary site type specified for fixed concentration boundaries.");
            if (!boundaryConditions.has_boundary_species()) throw Exception("No boundary species specified for fixed concentration boundaries.");
            if (!boundaryConditions.has_boundary_concentration()) throw Exception("No boundary concentration specified for fixed concentration boundaries.");

            site_t boundarySiteType = boundaryConditions.boundary_site();
            particle_t boundarySpecies = boundaryConditions.boundary_species();
            double boundarySpeciesCount = boundaryConditions.boundary_concentration()*NA*latticeSpacing*latticeSpacing*latticeSpacing*1000.0;
            double latticeSpacingSquared = latticeSpacing*latticeSpacing;

            Print::printf(Print::DEBUG, "Creating boundary fluxes for fixed concentration boundary on one or more axis: %d %d %0.2f",boundarySiteType,boundarySpecies,boundarySpeciesCount);
            hasBoundaryInflux = true;
            boundaryInflux = new double[latticeXSize*latticeYSize*latticeZSize];
            for (int i=0; i<latticeXSize*latticeYSize*latticeZSize; i++)
                boundaryInflux[i] = 0.0;

            bool globalFixedConcentration = (!boundaryConditions.axis_specific_boundaries() && lm::types::BoundaryConditions::FIXED_CONCENTRATION);
            if (globalFixedConcentration || (boundaryConditions.axis_specific_boundaries() && boundaryConditions.x_minus() == lm::types::BoundaryConditions::FIXED_CONCENTRATION))
            {
                int x=0;
                for (int z=0; z<latticeZSize; z++)
                    for (int y=0; y<latticeYSize; y++)
                        boundaryInflux[z*latticeXSize*latticeYSize+y*latticeXSize+x] += boundarySpeciesCount*(DF[boundarySiteType*numberSiteTypes*numberSpecies + sites->get(utuple(x,y,z))*numberSpecies + boundarySpecies]/latticeSpacingSquared);
            }
            if (globalFixedConcentration || (boundaryConditions.axis_specific_boundaries() && boundaryConditions.x_plus() == lm::types::BoundaryConditions::FIXED_CONCENTRATION))
            {
                int x=latticeXSize-1;
                for (int z=0; z<latticeZSize; z++)
                    for (int y=0; y<latticeYSize; y++)
                        boundaryInflux[z*latticeXSize*latticeYSize+y*latticeXSize+x] += boundarySpeciesCount*(DF[boundarySiteType*numberSiteTypes*numberSpecies + sites->get(utuple(x,y,z))*numberSpecies + boundarySpecies]/latticeSpacingSquared);
            }
            if (globalFixedConcentration || (boundaryConditions.axis_specific_boundaries() && boundaryConditions.y_minus() == lm::types::BoundaryConditions::FIXED_CONCENTRATION))
            {
                int y=0;
                for (int z=0; z<latticeZSize; z++)
                    for (int x=0; x<latticeXSize; x++)
                        boundaryInflux[z*latticeXSize*latticeYSize+y*latticeXSize+x] += boundarySpeciesCount*(DF[boundarySiteType*numberSiteTypes*numberSpecies + sites->get(utuple(x,y,z))*numberSpecies + boundarySpecies]/latticeSpacingSquared);
            }
            if (globalFixedConcentration || (boundaryConditions.axis_specific_boundaries() && boundaryConditions.y_plus() == lm::types::BoundaryConditions::FIXED_CONCENTRATION))
            {
                int y=latticeYSize-1;
                for (int z=0; z<latticeZSize; z++)
                    for (int x=0; x<latticeXSize; x++)
                        boundaryInflux[z*latticeXSize*latticeYSize+y*latticeXSize+x] += boundarySpeciesCount*(DF[boundarySiteType*numberSiteTypes*numberSpecies + sites->get(utuple(x,y,z))*numberSpecies + boundarySpecies]/latticeSpacingSquared);
            }
            if (globalFixedConcentration || (boundaryConditions.axis_specific_boundaries() && boundaryConditions.z_minus() == lm::types::BoundaryConditions::FIXED_CONCENTRATION))
            {
                int z=0;
                for (int y=0; y<latticeYSize; y++)
                    for (int x=0; x<latticeXSize; x++)
                        boundaryInflux[z*latticeXSize*latticeYSize+y*latticeXSize+x] += boundarySpeciesCount*(DF[boundarySiteType*numberSiteTypes*numberSpecies + sites->get(utuple(x,y,z))*numberSpecies + boundarySpecies]/latticeSpacingSquared);
            }
            if (globalFixedConcentration || (boundaryConditions.axis_specific_boundaries() && boundaryConditions.z_plus() == lm::types::BoundaryConditions::FIXED_CONCENTRATION))
            {
                int z=latticeZSize-1;
                for (int y=0; y<latticeYSize; y++)
                    for (int x=0; x<latticeXSize; x++)
                        boundaryInflux[z*latticeXSize*latticeYSize+y*latticeXSize+x] += boundarySpeciesCount*(DF[boundarySiteType*numberSiteTypes*numberSpecies + sites->get(utuple(x,y,z))*numberSpecies + boundarySpecies]/latticeSpacingSquared);
            }
        }

        // See if we need to use the boundary influx array for a constant gradient boundary.
        if ((!boundaryConditions.axis_specific_boundaries() && boundaryConditions.global() == lm::types::BoundaryConditions::FIXED_GRADIENT) || (boundaryConditions.axis_specific_boundaries() && (boundaryConditions.x_minus() == lm::types::BoundaryConditions::FIXED_GRADIENT || boundaryConditions.x_plus() == lm::types::BoundaryConditions::FIXED_GRADIENT || boundaryConditions.y_minus() == lm::types::BoundaryConditions::FIXED_GRADIENT || boundaryConditions.y_plus() == lm::types::BoundaryConditions::FIXED_GRADIENT || boundaryConditions.z_minus() == lm::types::BoundaryConditions::FIXED_GRADIENT || boundaryConditions.z_plus() == lm::types::BoundaryConditions::FIXED_GRADIENT)))
        {
            // Make sure we have the necessary parameters.
            if (!boundaryConditions.has_boundary_site()) throw Exception("No boundary site type specified for fixed gradient boundaries.");
            if (!boundaryConditions.has_boundary_species()) throw Exception("No boundary species specified for fixed gradient boundaries.");
            if (boundaryConditions.boundary_gradient().shape(0)*boundaryConditions.boundary_gradient().shape(1)*boundaryConditions.boundary_gradient().shape(2) != (latticeXSize+2)*(latticeYSize+2)*(latticeZSize+2)) throw Exception("Invalid boundary gradient array specified for fixed gradient boundaries.");

            ndarray<double>* gradient = robertslab::pbuf::NDArraySerializer::deserializeAllocate<double>(boundaryConditions.boundary_gradient());
            site_t boundarySiteType = boundaryConditions.boundary_site();
            particle_t boundarySpecies = boundaryConditions.boundary_species();
            double latticeSpacingSquared = latticeSpacing*latticeSpacing;

            Print::printf(Print::DEBUG, "Creating boundary fluxes for fixed gradient boundary on one or more axis: %d %d",boundarySiteType,boundarySpecies);
            hasBoundaryInflux = true;
            boundaryInflux = new double[latticeXSize*latticeYSize*latticeZSize];
            for (int i=0; i<latticeXSize*latticeYSize*latticeZSize; i++)
                boundaryInflux[i] = 0.0;

            bool globalFixedConcentration = (!boundaryConditions.axis_specific_boundaries() && lm::types::BoundaryConditions::FIXED_GRADIENT);
            if (globalFixedConcentration || (boundaryConditions.axis_specific_boundaries() && boundaryConditions.x_minus() == lm::types::BoundaryConditions::FIXED_GRADIENT))
            {
                int x=0;
                for (int z=0; z<latticeZSize; z++)
                {
                    for (int y=0; y<latticeYSize; y++)
                    {
                        int x2=x+1, y2=y+1, z2=z+1;
                        x2-=1;
                        double boundarySpeciesCount = gradient->get(utuple(x2,y2,z2))*NA*latticeSpacing*latticeSpacing*latticeSpacing*1000.0;
                        boundaryInflux[z*latticeXSize*latticeYSize+y*latticeXSize+x] += boundarySpeciesCount*(DF[boundarySiteType*numberSiteTypes*numberSpecies + sites->get(utuple(x,y,z))*numberSpecies + boundarySpecies]/latticeSpacingSquared);
                    }
                }
            }
            if (globalFixedConcentration || (boundaryConditions.axis_specific_boundaries() && boundaryConditions.x_plus() == lm::types::BoundaryConditions::FIXED_GRADIENT))
            {
                int x=latticeXSize-1;
                for (int z=0; z<latticeZSize; z++)
                {
                    for (int y=0; y<latticeYSize; y++)
                    {
                        int x2=x+1, y2=y+1, z2=z+1;
                        x2+=1;
                        double boundarySpeciesCount = gradient->get(utuple(x2,y2,z2))*NA*latticeSpacing*latticeSpacing*latticeSpacing*1000.0;
                        boundaryInflux[z*latticeXSize*latticeYSize+y*latticeXSize+x] += boundarySpeciesCount*(DF[boundarySiteType*numberSiteTypes*numberSpecies + sites->get(utuple(x,y,z))*numberSpecies + boundarySpecies]/latticeSpacingSquared);
                    }
                }
            }
            if (globalFixedConcentration || (boundaryConditions.axis_specific_boundaries() && boundaryConditions.y_minus() == lm::types::BoundaryConditions::FIXED_GRADIENT))
            {
                int y=0;
                for (int z=0; z<latticeZSize; z++)
                {
                    for (int x=0; x<latticeXSize; x++)
                    {
                        int x2=x+1, y2=y+1, z2=z+1;
                        y2-=1;
                        double boundarySpeciesCount = gradient->get(utuple(x2,y2,z2))*NA*latticeSpacing*latticeSpacing*latticeSpacing*1000.0;
                        boundaryInflux[z*latticeXSize*latticeYSize+y*latticeXSize+x] += boundarySpeciesCount*(DF[boundarySiteType*numberSiteTypes*numberSpecies + sites->get(utuple(x,y,z))*numberSpecies + boundarySpecies]/latticeSpacingSquared);
                    }
                }
            }
            if (globalFixedConcentration || (boundaryConditions.axis_specific_boundaries() && boundaryConditions.y_plus() == lm::types::BoundaryConditions::FIXED_GRADIENT))
            {
                int y=latticeYSize-1;
                for (int z=0; z<latticeZSize; z++)
                {
                    for (int x=0; x<latticeXSize; x++)
                    {
                        int x2=x+1, y2=y+1, z2=z+1;
                        y2+=1;
                        double boundarySpeciesCount = gradient->get(utuple(x2,y2,z2))*NA*latticeSpacing*latticeSpacing*latticeSpacing*1000.0;
                        boundaryInflux[z*latticeXSize*latticeYSize+y*latticeXSize+x] += boundarySpeciesCount*(DF[boundarySiteType*numberSiteTypes*numberSpecies + sites->get(utuple(x,y,z))*numberSpecies + boundarySpecies]/latticeSpacingSquared);
                    }
                }
            }
            if (globalFixedConcentration || (boundaryConditions.axis_specific_boundaries() && boundaryConditions.z_minus() == lm::types::BoundaryConditions::FIXED_GRADIENT))
            {
                int z=0;
                for (int y=0; y<latticeYSize; y++)
                {
                    for (int x=0; x<latticeXSize; x++)
                    {
                        int x2=x+1, y2=y+1, z2=z+1;
                        z2-=1;
                        double boundarySpeciesCount = gradient->get(utuple(x2,y2,z2))*NA*latticeSpacing*latticeSpacing*latticeSpacing*1000.0;
                        boundaryInflux[z*latticeXSize*latticeYSize+y*latticeXSize+x] += boundarySpeciesCount*(DF[boundarySiteType*numberSiteTypes*numberSpecies + sites->get(utuple(x,y,z))*numberSpecies + boundarySpecies]/latticeSpacingSquared);
                    }
                }
            }
            if (globalFixedConcentration || (boundaryConditions.axis_specific_boundaries() && boundaryConditions.z_plus() == lm::types::BoundaryConditions::FIXED_GRADIENT))
            {
                int z=latticeZSize-1;
                for (int y=0; y<latticeYSize; y++)
                {
                    for (int x=0; x<latticeXSize; x++)
                    {
                        int x2=x+1, y2=y+1, z2=z+1;
                        z2+=1;
                        double boundarySpeciesCount = gradient->get(utuple(x2,y2,z2))*NA*latticeSpacing*latticeSpacing*latticeSpacing*1000.0;
                        boundaryInflux[z*latticeXSize*latticeYSize+y*latticeXSize+x] += boundarySpeciesCount*(DF[boundarySiteType*numberSiteTypes*numberSpecies + sites->get(utuple(x,y,z))*numberSpecies + boundarySpecies]/latticeSpacingSquared);
                    }
                }
            }

            // Uncomment to print out boundary flux array.
            /*
            Print::printf(Print::DEBUG, "Created boundary fluxes.");
            for (lattice_size_t z=0; z<latticeZSize; z++)
            {
                for (lattice_size_t x=0; x<latticeXSize; x++)
                {
                    for (lattice_size_t y=0; y<latticeYSize; y++)
                    {
                        int influxDataIndex = z*latticeXSize*latticeYSize+y*latticeXSize+x;
                        double flux = boundaryInflux[influxDataIndex];
                        printf("%6.2e%c",flux,y<latticeYSize-1?',':' ');
                    }
                    printf("\n");
                }
                printf("---------------\n");
            }
            */

            delete gradient;
        }
    }

    // Free any resources.
}

DiffusionModel::~DiffusionModel()
{
    if (DF != NULL) delete[] DF; DF = NULL;
    if (RL != NULL) delete[] RL; RL = NULL;
    if (sites != NULL) delete sites; sites = NULL;
    if (boundaryInflux != NULL) delete[] boundaryInflux; boundaryInflux = NULL;
}

}
}
