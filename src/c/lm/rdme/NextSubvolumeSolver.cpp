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

#include "lm/ClassFactory.h"
#include "lm/Exceptions.h"
#include "lm/Tune.h"
#include "lm/Print.h"
#include "lm/cme/CMESolver.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/Lattice.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/rdme/Lattice.h"
#include "lm/rdme/ByteLattice.h"
#include "lm/rdme/NextSubvolumeSolver.h"
#include "lm/reaction/ReactionQueue.h"
#include "lm/rng/RandomGenerator.h"
#include "lptf/Profile.h"
#include "lptf/ProfileCodes.h"

using lm::rng::RandomGenerator;

namespace lm {
namespace rdme {

bool NextSubvolumeSolver::registered=NextSubvolumeSolver::registerClass();

bool NextSubvolumeSolver::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::me::MESolver","lm::rdme::NextSubvolumeSolver",&NextSubvolumeSolver::allocateObject);
    return true;
}

void* NextSubvolumeSolver::allocateObject()
{
    return new NextSubvolumeSolver();
}


NextSubvolumeSolver::NextSubvolumeSolver():RDMESolver((RandomGenerator::Distributions)(RandomGenerator::EXPONENTIAL|RandomGenerator::UNIFORM)),numberSubvolumes(0),latticeSpacingSquared(0.0),reactionQueue(NULL),currentSubvolumeSpeciesCounts(NULL)
{
}

NextSubvolumeSolver::NextSubvolumeSolver(RandomGenerator::Distributions neededDists):RDMESolver((RandomGenerator::Distributions)(RandomGenerator::EXPONENTIAL|RandomGenerator::UNIFORM|neededDists)),numberSubvolumes(0),latticeSpacingSquared(0.0),reactionQueue(NULL),currentSubvolumeSpeciesCounts(NULL)
{
}


NextSubvolumeSolver::~NextSubvolumeSolver()
{
    if (currentSubvolumeSpeciesCounts != NULL) delete[] currentSubvolumeSpeciesCounts; currentSubvolumeSpeciesCounts = NULL;
    if (reactionQueue != NULL) delete reactionQueue; reactionQueue = NULL;
}

void NextSubvolumeSolver::reset()
{
    RDMESolver::reset();

    // Reset the subvolume species counts.
    for (uint i=0; i<reactionModel->numberSpecies; i++)
        currentSubvolumeSpeciesCounts[i] = 0;

    // Create the reaction queue.
    PROF_BEGIN(PROF_NSM_INIT_QUEUE);
    if (reactionQueue != NULL) delete reactionQueue;
    reactionQueue = new ReactionQueue(numberSubvolumes);
    PROF_END(PROF_NSM_INIT_QUEUE);

}

void NextSubvolumeSolver::getState(lm::io::TrajectoryState* state)
{
    RDMESolver::getState(state);
}

void NextSubvolumeSolver::setState(const lm::io::TrajectoryState& state)
{
    RDMESolver::setState(state);
}

void NextSubvolumeSolver::setDiffusionModel(const lm::io::DiffusionModel& dm)
{
    RDMESolver::setDiffusionModel(dm);

    // Allocate the subvolume species counts.
    if (currentSubvolumeSpeciesCounts != NULL) delete[] currentSubvolumeSpeciesCounts;
    currentSubvolumeSpeciesCounts = new int[reactionModel->numberSpecies];

    // Fill in some parameter variables.
    numberSubvolumes = lattice->getNumberSites();
    latticeSpacingSquared = diffusionModel->latticeSpacing*diffusionModel->latticeSpacing;

    // Update the propensity functions with the subvolume size.
    for (uint i=0; i<reactionModel->numberReactions; i++)
    {
        if (reactionModel->reactionTypes[i] == SecondOrderPropensityArgs::REACTION_TYPE)
        {
            ((SecondOrderPropensityArgs *)reactionModel->propensityFunctionArgs[i])->k *= numberSubvolumes;
            Print::printf(Print::VERBOSE_DEBUG, "Updated second order reaction %d rate constant to: %8.2e",i,((SecondOrderPropensityArgs *)reactionModel->propensityFunctionArgs[i])->k);
        }
        else if (reactionModel->reactionTypes[i] == SecondOrderSelfPropensityArgs::REACTION_TYPE)
        {
            ((SecondOrderSelfPropensityArgs *)reactionModel->propensityFunctionArgs[i])->k *= numberSubvolumes;
        }
    }
}

bool NextSubvolumeSolver::generateTrajectory(long long maxSteps)
{
    if (reactionModel == NULL) throw Exception("NextSubvolumeSolver did not have a reaction model.");
    if (diffusionModel == NULL) throw Exception("NextSubvolumeSolver did not have a diffusion model.");
    if (reactionQueue == NULL) throw Exception("NextSubvolumeSolver state was not initialized.");

    // Make sure we have propensity functions for every reaction.
    for (uint i=0; i<reactionModel->numberReactions; i++)
        if (reactionModel->propensityFunctions[i] == NULL || reactionModel->propensityFunctionArgs[i] == NULL)
            throw Exception("A reaction did not have a valid propensity function",i);

    // Make sure that the initial species counts agree with the actual number in the lattice.
    checkSpeciesCountsAgainstLattice();

    // Get the interval for writing species counts and lattices.
//    double speciesCountsWriteInterval=atof((*parameters)["writeInterval"].c_str());
//    double nextSpeciesCountsWriteTime = speciesCountsWriteInterval;
//    double latticeWriteInterval=atof((*parameters)["latticeWriteInterval"].c_str());
//    double nextLatticeWriteTime = latticeWriteInterval;

    // Create the species counts data set to use during the simulation.
//    lm::io::SpeciesCounts speciesCountsDataSet;
//    speciesCountsDataSet.set_number_species(numberSpeciesToTrack);
//    speciesCountsDataSet.set_number_entries(0);

    // Create the lattice data set to use during the simulation.
//    lm::io::Lattice latticeDataSet;

    // Local cache of random numbers.
    double expRngValues[TUNE_LOCAL_RNG_CACHE_SIZE];
    double uniRngValues[TUNE_LOCAL_RNG_CACHE_SIZE];
    rng->getExpRandomDoubles(expRngValues,TUNE_LOCAL_RNG_CACHE_SIZE);
    rng->getRandomDoubles(uniRngValues,TUNE_LOCAL_RNG_CACHE_SIZE);
    int expRngNext=0;
    int uniRngNext=0;

    // Initialize the reaction queue.
    expRngNext=updateAllSubvolumePropensities(time, expRngNext, expRngValues);

    // Record the initial species counts.
//    recordSpeciesCounts(time, &speciesCountsDataSet);

    // Write the initial lattice.
//    if (nextLatticeWriteTime > 0.0)
//    	writeLatticeData(time, lattice, &latticeDataSet);

    // Run the next subvolume method.
    Print::printf(Print::DEBUG, "Running next subvolume simulation for %d steps with %d species, %d reactions, %d subvolumes, %d site types for %e s.", maxSteps, reactionModel->numberSpecies, reactionModel->numberReactions, numberSubvolumes, diffusionModel->numberSiteTypes, maxTime);
    lattice->print();
    PROF_BEGIN(PROF_SIM_EXECUTE);
    long long steps=0;
    long long reactionSteps=0;
    bool affectedNeighbor;
    lattice_size_t subvolume;
    lattice_size_t neighborSubvolume;
    while (steps < maxSteps)
    {
        steps++;

        // Get the next subvolume with a reaction and the reaction time.
        subvolume = reactionQueue->getNextReaction();
        time = reactionQueue->getReactionEvent(subvolume).time;

        // If the new time is past the end time, we are done.
       if (time >= maxTime)
       {
           break;
       }

       // Write species counts until the next write time is past the current time.
//       while (nextSpeciesCountsWriteTime <= (time+EPS))
//       {
//           recordSpeciesCounts(nextSpeciesCountsWriteTime, &speciesCountsDataSet);
//           nextSpeciesCountsWriteTime += speciesCountsWriteInterval;
//           addedSpeciesCounts = true;
//       }

       // Write lattice frames until the next write time is past the current time.
//       while (nextLatticeWriteTime > 0.0 && nextLatticeWriteTime <= (time+EPS))
//       {
//           writeLatticeData(nextLatticeWriteTime, lattice, &latticeDataSet);
//           nextLatticeWriteTime += latticeWriteInterval;
//       }

       // Update the system with the reaction.
       affectedNeighbor = false;
       uniRngNext=performSubvolumeEvent(time, subvolume, uniRngNext, uniRngValues, affectedNeighbor, neighborSubvolume);

       // If the event didn't affect a neighbor, it must have been a reaction.
       if (!affectedNeighbor) reactionSteps++;

       // Update the propensity in the affected subvolumes.
       expRngNext=updateSubvolumePropensity(time, subvolume, expRngNext, expRngValues);
       if (affectedNeighbor) expRngNext=updateSubvolumePropensity(time, neighborSubvolume, expRngNext, expRngValues);

//       // Update the first passage time tables.
//       for (uint i=0; i<numberFptTrackedSpecies; i++)
//       {
//           uint speciesCount = speciesCounts[fptTrackedSpecies[i].species];
//           while (fptTrackedSpecies[i].minValueAchieved > speciesCount)
//           {
//               fptTrackedSpecies[i].dataSet.add_species_count(--fptTrackedSpecies[i].minValueAchieved);
//               fptTrackedSpecies[i].dataSet.add_first_passage_time(time);
//               addedFpt = true;
//           }
//           while (fptTrackedSpecies[i].maxValueAchieved < speciesCount)
//           {
//               fptTrackedSpecies[i].dataSet.add_species_count(++fptTrackedSpecies[i].maxValueAchieved);
//               fptTrackedSpecies[i].dataSet.add_first_passage_time(time);
//               addedFpt = true;
//           }

//           // See if we have accumulated enough fpt data to send.
//           if (addedFpt && fptTrackedSpecies[i].dataSet.first_passage_time_size() >= TUNE_FIRST_PASSAGE_TIME_BUFFER_SIZE)
//           {
//               // Push it to the output queue.
//               PROF_BEGIN(PROF_SERIALIZE_FPT);
//               lm::main::DataOutputQueue::getInstance()->pushDataSet(lm::main::DataOutputQueue::FIRST_PASSAGE_TIMES, replicate, &fptTrackedSpecies[i].dataSet);
//               PROF_END(PROF_SERIALIZE_FPT);

//               // Reset the data set.
//               fptTrackedSpecies[i].dataSet.Clear();
//               fptTrackedSpecies[i].dataSet.set_species(fptTrackedSpecies[i].species);
//           }
//       }

//       // See if we have accumulated enough species counts to send.
//       if (addedSpeciesCounts && speciesCountsDataSet.number_entries() >= TUNE_SPECIES_COUNTS_BUFFER_SIZE)
//       {
//           // Push it to the output queue.
//           writeSpeciesCounts(&speciesCountsDataSet);
//       }

       //Print::printf(Print::VERBOSE_DEBUG, "Step %d: time=%e, count=%d,%d,%d",steps,time,speciesCounts[0],speciesCounts[1],speciesCounts[2]);
    }
    PROF_END(PROF_SIM_EXECUTE);

    // Make sure that the final species counts agree with the actual number in the lattice.
    checkSpeciesCountsAgainstLattice();

    bool reachedLimit = false;

    // If we finished the total time, write out the remaining time steps.
    if (time >= maxTime)
    {
        time = maxTime;
        Print::printf(Print::DEBUG, "Generated trajectory through time %e.", time);
//        while (nextSpeciesCountsWriteTime <= (maxTime+EPS))
//        {
//            // Record the species counts.
//            recordSpeciesCounts(nextSpeciesCountsWriteTime, &speciesCountsDataSet);
//            nextSpeciesCountsWriteTime += speciesCountsWriteInterval;
//        }
//        while (nextLatticeWriteTime > 0.0 && nextLatticeWriteTime <= (maxTime+EPS))
//        {
//            writeLatticeData(nextLatticeWriteTime, lattice, &latticeDataSet);
//            nextLatticeWriteTime += latticeWriteInterval;
//        }
        reachedLimit = true;
    }

    // See if we finished all of the steps.
    else if (steps >= maxSteps)
    {
        Print::printf(Print::DEBUG, "Generated trajectory with %llu steps (%llu reaction events).", steps, reactionSteps);
    }

    // Otherwise we must have finished because of a species limit or step, so just write out the last time.
    else
    {
        // Record the species counts.
//        recordSpeciesCounts(time, &speciesCountsDataSet);
//        if (nextLatticeWriteTime > 0.0)
//        	writeLatticeData(time, lattice, &latticeDataSet);
        reachedLimit = true;
    }

    // Send any remaining first passage times to the queue.
//    for (uint i=0; i<numberFptTrackedSpecies; i++)
//    {
//        if (fptTrackedSpecies[i].dataSet.first_passage_time_size() > 0)
//        {
//            // Push it to the output queue.
//            PROF_BEGIN(PROF_SERIALIZE_FPT);
//            lm::main::DataOutputQueue::getInstance()->pushDataSet(lm::main::DataOutputQueue::FIRST_PASSAGE_TIMES, replicate, &fptTrackedSpecies[i].dataSet);
//            PROF_END(PROF_SERIALIZE_FPT);
//        }
//    }

//    // Send any remaining species counts to the queue.
//    writeSpeciesCounts(&speciesCountsDataSet);

    return reachedLimit;
}

void NextSubvolumeSolver::checkSpeciesCountsAgainstLattice()
{
	std::map<particle_t,uint> particleCounts = lattice->getParticleCounts();
    for (uint i=0; i<reactionModel->numberSpecies; i++)
	{
		if (speciesCounts[i] != ((particleCounts.count(i+1)>0)?particleCounts[i+1]:0))
			throw lm::Exception("Consistency error between species counts and lattice data", i, speciesCounts[i], ((particleCounts.count(i+1)>0)?particleCounts[i+1]:0));
	}
}

//void NextSubvolumeSolver::writeLatticeData(double time, ByteLattice * lattice, lm::io::Lattice * latticeDataSet)
//{
//    Print::printf(Print::DEBUG, "Writing lattice at %e s", time);

//    // Record the lattice data.
//    latticeDataSet->Clear();
//    latticeDataSet->set_lattice_x_size(lattice->getSize().x);
//    latticeDataSet->set_lattice_y_size(lattice->getSize().y);
//    latticeDataSet->set_lattice_z_size(lattice->getSize().z);
//    latticeDataSet->set_particles_per_site(lattice->getMaxOccupancy());
//    latticeDataSet->set_time(time);

//    // Push it to the output queue.
//    size_t payloadSize = lattice->getSize().x*lattice->getSize().y*lattice->getSize().z*lattice->getMaxOccupancy()*sizeof(uint8_t);
//    lm::main::DataOutputQueue::getInstance()->pushDataSet(lm::main::DataOutputQueue::BYTE_LATTICE, replicate, latticeDataSet, lattice, payloadSize, &lm::rdme::ByteLattice::nativeSerialize);
//}

//void NextSubvolumeSolver::recordSpeciesCounts(double time, lm::io::SpeciesCounts * speciesCountsDataSet)
//{
//    speciesCountsDataSet->set_number_entries(speciesCountsDataSet->number_entries()+1);
//    speciesCountsDataSet->add_time(time);
//    for (uint i=0; i<numberSpeciesToTrack; i++) speciesCountsDataSet->add_species_count(speciesCounts[i]);
//}

//void NextSubvolumeSolver::writeSpeciesCounts(lm::io::SpeciesCounts * speciesCountsDataSet)
//{
//    if (speciesCountsDataSet->number_entries() > 0)
//    {
//        PROF_BEGIN(PROF_SERIALIZE_COUNTS);
//        // Push it to the output queue.
//        lm::main::DataOutputQueue::getInstance()->pushDataSet(lm::main::DataOutputQueue::SPECIES_COUNTS, replicate, speciesCountsDataSet);

//        // Reset the data set.
//        speciesCountsDataSet->Clear();
//        speciesCountsDataSet->set_number_species(numberSpeciesToTrack);
//        speciesCountsDataSet->set_number_entries(0);
//        PROF_END(PROF_SERIALIZE_COUNTS);
//    }
//}

int NextSubvolumeSolver::updateAllSubvolumePropensities(si_time_t time, int rngNext, double * expRngValues)
{
    // Update all of the subvolumes.
    PROF_BEGIN(PROF_NSM_BUILD_QUEUE);
    for (lattice_size_t s=0; s<numberSubvolumes; s++)
    {
        double propensity = calculateSubvolumePropensity(time, s);
        double newTime = INFINITY;
        if (propensity > 0.0)
        {
            if (rngNext >= TUNE_LOCAL_RNG_CACHE_SIZE)
            {
                rng->getExpRandomDoubles(expRngValues,TUNE_LOCAL_RNG_CACHE_SIZE);
                rngNext=0;
            }
            newTime = time+expRngValues[rngNext++]/propensity;
        }
        reactionQueue->updateReactionEvent(s, newTime, propensity);
    }
    PROF_END(PROF_NSM_BUILD_QUEUE);

    return rngNext;
}

int NextSubvolumeSolver::updateSubvolumePropensity(si_time_t time, lattice_size_t subvolume, int rngNext, double * expRngValues)
{
    double propensity = calculateSubvolumePropensity(time, subvolume);

    double newTime = INFINITY;
    if (propensity > 0.0)
    {
        if (rngNext >= TUNE_LOCAL_RNG_CACHE_SIZE)
        {
            rng->getExpRandomDoubles(expRngValues,TUNE_LOCAL_RNG_CACHE_SIZE);
            rngNext=0;
        }
        newTime = time+expRngValues[rngNext++]/propensity;
    }
    reactionQueue->updateReactionEvent(subvolume, newTime, propensity);

    return rngNext;
}

double NextSubvolumeSolver::calculateSubvolumePropensity(si_time_t time, lattice_size_t subvolume)
{
	// Update the species counts member for this subvolume.
	updateSpeciesCountsForSubvolume(subvolume);

	// Get the site type for this subvolume.
    site_t sourceSite = lattice->getSiteType(subvolume);

    // Calculate all of the reaction propensities.
	double subvolumePropensity = 0.0;
    for (uint i=0; i<reactionModel->numberReactions; i++)
    {
        // Make sure the reaction can occur in this subvolume.
        if (diffusionModel->RL[i*diffusionModel->numberSiteTypes+sourceSite])
    	{
            double (*propensityFunction)(double, uint *, void *) = (double (*)(double, uint*, void*))reactionModel->propensityFunctions[i];
            subvolumePropensity += (*propensityFunction)(time, (uint*)currentSubvolumeSpeciesCounts, reactionModel->propensityFunctionArgs[i]);
        }
    }

    // Add in the diffusion propensity.
    subvolumePropensity+=calculateSubvolumeDiffusionPropensity(time, subvolume, sourceSite);

    // Add in any propensity for particle influx from the boundaries.
    subvolumePropensity+=calculateSubvolumeInfluxPropensity(time, subvolume);

    return subvolumePropensity;
}

double NextSubvolumeSolver::calculateSubvolumeDiffusionPropensity(si_time_t time, lattice_size_t subvolume, site_t sourceSite)
{
    double subvolumePropensity=0.0;
    const int NUM_NEIGHBORS=6;

    // Get the neighboring sites.
    lattice_size_t neighboringSubvolumes[NUM_NEIGHBORS];
    lattice->getNeighboringSites(subvolume, neighboringSubvolumes, false);

    // Fill in the boundary conditions.
    lm::io::BoundaryConditions::BoundaryConditionsType bc[NUM_NEIGHBORS];
    if (diffusionModel->boundaryConditions.axis_specific_boundaries())
    {
        bc[0]=diffusionModel->boundaryConditions.x_minus();
        bc[1]=diffusionModel->boundaryConditions.x_plus();
        bc[2]=diffusionModel->boundaryConditions.y_minus();
        bc[3]=diffusionModel->boundaryConditions.y_plus();
        bc[4]=diffusionModel->boundaryConditions.z_minus();
        bc[5]=diffusionModel->boundaryConditions.z_plus();
    }
    else
    {
        for (int j=0; j<NUM_NEIGHBORS; j++)
            bc[j]=diffusionModel->boundaryConditions.global();
    }

    // Calculate all of the diffusion propensities.
    for (uint i=0; i<reactionModel->numberSpecies; i++)
    {
        if (currentSubvolumeSpeciesCounts[i] > 0)
        {
            for (int j=0; j<NUM_NEIGHBORS; j++)
            {
                // See if the neighbor is a boundary.
                int neighborIndex=neighboringSubvolumes[j];
                if (neighborIndex == LATTICE_SIZE_MAX)
                {
                    if (bc[j] == lm::io::BoundaryConditions::REFLECTING)
                    {
                    }
                    else if (bc[j] == lm::io::BoundaryConditions::ABSORBING || bc[j] == lm::io::BoundaryConditions::FIXED_CONCENTRATION || bc[j] == lm::io::BoundaryConditions::FIXED_GRADIENT)
                    {
                        subvolumePropensity += ((double)currentSubvolumeSpeciesCounts[i]) * (diffusionModel->DF[sourceSite*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + sourceSite*reactionModel->numberSpecies + i]/latticeSpacingSquared);
                    }
                    else if (bc[j] == lm::io::BoundaryConditions::PERIODIC)
                    {
                        lattice_size_t neighboringSubvolumesPeriodic[NUM_NEIGHBORS];
                        lattice->getNeighboringSites(subvolume, neighboringSubvolumesPeriodic, true);
                        int neighborIndexPeriodic=neighboringSubvolumesPeriodic[j];
                        subvolumePropensity += ((double)currentSubvolumeSpeciesCounts[i]) * (diffusionModel->DF[sourceSite*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(neighborIndexPeriodic)*reactionModel->numberSpecies + i]/latticeSpacingSquared);
                    }
                }
                else
                {
                    subvolumePropensity += ((double)currentSubvolumeSpeciesCounts[i]) * (diffusionModel->DF[sourceSite*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(neighborIndex)*reactionModel->numberSpecies + i]/latticeSpacingSquared);
                }
            }
        }
    }
    return subvolumePropensity;
}

double NextSubvolumeSolver::calculateSubvolumeInfluxPropensity(si_time_t time, lattice_size_t subvolume)
{
    if (diffusionModel->hasBoundaryInflux && lattice->isBoundarySite(subvolume))
    {
        return diffusionModel->boundaryInflux[subvolume];
    }

    return 0.0;
}

void  NextSubvolumeSolver::updateSpeciesCountsForSubvolume(lattice_size_t subvolume)
{
	// Reset the species counts.
    for (uint i=0; i<reactionModel->numberSpecies; i++)
		currentSubvolumeSpeciesCounts[i] = 0;

	// Count the species that are in this subvolume.
	site_size_t numberParticles = lattice->getOccupancy(subvolume);
	for (site_size_t i=0; i<numberParticles; i++)
		currentSubvolumeSpeciesCounts[lattice->getParticle(subvolume, i)-1]++;
}

void NextSubvolumeSolver::updateSubvolumeWithSpeciesCounts(lattice_size_t subvolume)
{
	lattice->removeParticles(subvolume);
    for (uint i=0; i<reactionModel->numberSpecies; i++)
			addParticles(subvolume, i+1, currentSubvolumeSpeciesCounts[i]);
}

int NextSubvolumeSolver::performSubvolumeEvent(si_time_t time, lattice_size_t subvolume, int rngNext, double * uniRngValues, bool& affectedNeighbor, lattice_size_t& neighborSubvolume)
{
	// Only set the affected neighbor if this was a diffusion event.
    affectedNeighbor = false;

	// Update the species counts member for this subvolume.
	updateSpeciesCountsForSubvolume(subvolume);

    // Stretch the random value across the total propensity range.
    if (rngNext >= TUNE_LOCAL_RNG_CACHE_SIZE)
    {
        rng->getRandomDoubles(uniRngValues,TUNE_LOCAL_RNG_CACHE_SIZE);
        rngNext=0;
    }
    double rngValue = uniRngValues[rngNext++]*reactionQueue->getReactionEvent(subvolume).propensity;

    // Get the site type for this subvolume.
    site_t sourceSite = lattice->getSiteType(subvolume);

    // See if it was a reaction that occurred.
    for (uint r=0; r<reactionModel->numberReactions; r++)
    {
    	// Make sure the reaction can occur in this subvolume.
        if (diffusionModel->RL[r*diffusionModel->numberSiteTypes+sourceSite])
    	{
            double (*propensityFunction)(double, uint *, void *) = (double (*)(double, uint*, void*))reactionModel->propensityFunctions[r];
            double reactionPropensity = (*propensityFunction)(time, (uint*)currentSubvolumeSpeciesCounts, reactionModel->propensityFunctionArgs[r]);
			if (reactionPropensity > 0.0)
			{
				if (rngValue <= reactionPropensity)
				{
					updateSpeciesCounts(r);
					updateCurrentSubvolumeSpeciesCounts(r);
					updateSubvolumeWithSpeciesCounts(subvolume);
					return rngNext;
				}
				else
				{
					rngValue -= reactionPropensity;
				}
			}
    	}
    }

    // See if it was a diffusion event that occurred.
    if (performSubvolumeDiffusionEvent(time, subvolume, sourceSite, rngValue, affectedNeighbor, neighborSubvolume))
        return rngNext;

    // See if it was an influx event that occurred.
    if (performSubvolumeInfluxEvent(time, subvolume, rngValue))
        return rngNext;

    throw Exception("Unable to determine correct reaction or diffusion event in subvolume.");
}

bool NextSubvolumeSolver::performSubvolumeDiffusionEvent(si_time_t time, lattice_size_t subvolume, site_t sourceSite, double& rngValue, bool& affectedNeighbor, lattice_size_t& neighborSubvolume)
{
    const int NUM_NEIGHBORS=6;

    // Get the neighboring sites.
    lattice_size_t neighboringSubvolumes[NUM_NEIGHBORS];
    lattice->getNeighboringSites(subvolume, neighboringSubvolumes, false);

    // Fill in the boundary conditions.
    lm::io::BoundaryConditions::BoundaryConditionsType bc[NUM_NEIGHBORS];
    if (diffusionModel->boundaryConditions.axis_specific_boundaries())
    {
        bc[0]=diffusionModel->boundaryConditions.x_minus();
        bc[1]=diffusionModel->boundaryConditions.x_plus();
        bc[2]=diffusionModel->boundaryConditions.y_minus();
        bc[3]=diffusionModel->boundaryConditions.y_plus();
        bc[4]=diffusionModel->boundaryConditions.z_minus();
        bc[5]=diffusionModel->boundaryConditions.z_plus();
    }
    else
    {
        for (int j=0; j<NUM_NEIGHBORS; j++)
            bc[j]=diffusionModel->boundaryConditions.global();
    }

    // See if it was a diffusion event that occurred.
    for (uint i=0; i<reactionModel->numberSpecies; i++)
    {
        if (currentSubvolumeSpeciesCounts[i] > 0)
        {
            for (int j=0; j<NUM_NEIGHBORS; j++)
            {
                // See if the neighbor is a boundary.
                int neighborIndex=neighboringSubvolumes[j];
                if (neighborIndex == LATTICE_SIZE_MAX)
                {
                    if (bc[j] == lm::io::BoundaryConditions::REFLECTING)
                    {
                    }
                    else if (bc[j] == lm::io::BoundaryConditions::ABSORBING ||bc[j] == lm::io::BoundaryConditions::FIXED_CONCENTRATION || bc[j] == lm::io::BoundaryConditions::FIXED_GRADIENT)
                    {
                        double diffusionPropensity = ((double)currentSubvolumeSpeciesCounts[i]) * (diffusionModel->DF[sourceSite*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + sourceSite*reactionModel->numberSpecies + i]/latticeSpacingSquared);

                        // See if this is the diffusion event that occurred.
                        if (rngValue <= diffusionPropensity)
                        {
                            speciesCounts[i]--;
                            updatedSpeciesCounts();
                            currentSubvolumeSpeciesCounts[i]--;
                            updateSubvolumeWithSpeciesCounts(subvolume);
                            affectedNeighbor = false;
                            return true;
                        }
                        else
                        {
                            rngValue -= diffusionPropensity;
                        }
                    }
                    else if (bc[j] == lm::io::BoundaryConditions::PERIODIC)
                    {
                        lattice_size_t neighboringSubvolumesPeriodic[NUM_NEIGHBORS];
                        lattice->getNeighboringSites(subvolume, neighboringSubvolumesPeriodic, true);
                        int neighborIndexPeriodic=neighboringSubvolumesPeriodic[j];
                        double diffusionPropensity = ((double)currentSubvolumeSpeciesCounts[i]) * (diffusionModel->DF[sourceSite*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(neighborIndexPeriodic)*reactionModel->numberSpecies + i]/latticeSpacingSquared);

                        // See if this is the diffusion event that occurred.
                        if (rngValue <= diffusionPropensity)
                        {
                            currentSubvolumeSpeciesCounts[i]--;
                            updateSubvolumeWithSpeciesCounts(subvolume);
                            affectedNeighbor = true;
                            neighborSubvolume = neighborIndexPeriodic;
                            addParticles(neighborSubvolume, i+1, 1);
                            return true;
                        }
                        else
                        {
                            rngValue -= diffusionPropensity;
                        }
                    }
                }
                else
                {
                    double diffusionPropensity = ((double)currentSubvolumeSpeciesCounts[i]) * (diffusionModel->DF[sourceSite*diffusionModel->numberSiteTypes*reactionModel->numberSpecies + lattice->getSiteType(neighborIndex)*reactionModel->numberSpecies + i]/latticeSpacingSquared);

                    // See if this is the diffusion event that occurred.
                    if (rngValue <= diffusionPropensity)
                    {
                        currentSubvolumeSpeciesCounts[i]--;
                        updateSubvolumeWithSpeciesCounts(subvolume);
                        affectedNeighbor = true;
                        neighborSubvolume = neighboringSubvolumes[j];
                        addParticles(neighborSubvolume, i+1, 1);
                        return true;
                    }
                    else
                    {
                        rngValue -= diffusionPropensity;
                    }
                }
            }
        }
    }
    return false;
}

bool NextSubvolumeSolver::performSubvolumeInfluxEvent(si_time_t time, lattice_size_t subvolume, double& rngValue)
{
    if (diffusionModel->hasBoundaryInflux && lattice->isBoundarySite(subvolume))
    {
        double influxPropensity = diffusionModel->boundaryInflux[subvolume];
        if (rngValue <= influxPropensity)
        {
            speciesCounts[diffusionModel->boundaryConditions.boundary_species()]++;
            updatedSpeciesCounts();
            currentSubvolumeSpeciesCounts[diffusionModel->boundaryConditions.boundary_species()]++;
            updateSubvolumeWithSpeciesCounts(subvolume);
            return true;
        }
        else
        {
            rngValue -= influxPropensity;
        }
    }
    return false;
}


void NextSubvolumeSolver::updateCurrentSubvolumeSpeciesCounts(uint r)
{
    for (uint i=0; i<reactionModel->numberDependentSpecies[r]; i++)
    {
        currentSubvolumeSpeciesCounts[reactionModel->dependentSpecies[r][i]] += reactionModel->dependentSpeciesChange[r][i];
    }
}

void NextSubvolumeSolver::addParticles(lattice_size_t subvolume, particle_t particle, uint count)
{
	for (uint i=0; i<count; i++)
	{
		try
		{
				lattice->addParticle(subvolume, particle);
		}
		catch (InvalidParticleException & e)
		{
			// We need to perform some overflow processing.
			const uint NUM_NEIGHBORS=6;
			lattice_size_t neighboringSubvolumes[NUM_NEIGHBORS];
            lattice->getNeighboringSites(subvolume, neighboringSubvolumes, diffusionModel->boundaryConditions.global() == lm::io::BoundaryConditions::PERIODIC);
			bool handled = false;
			for (uint i=0; i<NUM_NEIGHBORS && !handled; i++)
			{
                int neighborIndex=neighboringSubvolumes[i];
                if (neighborIndex != LATTICE_SIZE_MAX)
                {
                    if (lattice->getOccupancy(neighborIndex) < lattice->getMaxOccupancy() && lattice->getSiteType(neighborIndex) == lattice->getSiteType(subvolume))
                    {
                        lattice->addParticle(neighborIndex, particle);
                        handled = true;
                        Print::printf(Print::WARNING, "Handled overflow of particle type %d (%d total) from subvolume %d (type %d,occupancy %d) by moving to subvolume %d (type %d,occupancy %d).", particle, count, subvolume, lattice->getSiteType(subvolume), lattice->getOccupancy(subvolume), neighboringSubvolumes[i], lattice->getSiteType(neighboringSubvolumes[i]), lattice->getOccupancy(neighboringSubvolumes[i]));
                    }
                }
			}
			if (!handled) throw Exception("Unable to handle overflow at site", subvolume);
		}
	}
}

}
}
