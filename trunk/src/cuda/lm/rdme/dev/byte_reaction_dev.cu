/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 *               University of Illinois at Urbana-Champaign
 *               http://www.scs.uiuc.edu/~schulten
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

#include <cuda.h>
#include <cuda_runtime.h>

#if !defined MPD_WORDS_PER_SITE
#error "Must define the number of words per site."
#endif

#define MPD_PARTICLES_PER_SITE MPD_WORDS_PER_SITE*4

#if !defined TUNE_MPD_MAX_PARTICLE_OVERFLOWS
#error "Must define the maximum size of overflow particle list."
#endif

#define MPD_FIRST_ORDER_REACTION        1
#define MPD_SECOND_ORDER_REACTION       2
#define MPD_SECOND_ORDER_SELF_REACTION  3

#define MPD_MAX_REACTION_TABLE_ENTRIES	32
//TODO: check for performance hit/advantage if using byte arrays for constants.
__device__ __constant__ unsigned int numberReactionsC;
__device__ __constant__ unsigned int reactionOrdersC[MPD_MAX_REACTION_TABLE_ENTRIES];
__device__ __constant__ unsigned int reactionSitesC[MPD_MAX_REACTION_TABLE_ENTRIES];
__device__ __constant__ unsigned int D1C[MPD_MAX_REACTION_TABLE_ENTRIES];
__device__ __constant__ unsigned int D2C[MPD_MAX_REACTION_TABLE_ENTRIES];
__device__ __constant__ float reactionRatesC[MPD_MAX_REACTION_TABLE_ENTRIES];

/**
 * The stoichiometric matrix: numberSpecies x numberReactions
 */
#define MPD_MAX_S_MATRIX_ENTRIES    	512
__device__ __constant__ int8_t SC[MPD_MAX_S_MATRIX_ENTRIES];

/**
 * The reaction location matrix: numberReaction x numberSiteTypes
 */
#define MPD_MAX_RL_MATRIX_ENTRIES    	320
__device__ __constant__ uint8_t RLC[MPD_MAX_RL_MATRIX_ENTRIES];

inline __device__ float calculateReactionPropensity(const uint8_t siteType, const uint8_t * __restrict__ particles, const unsigned int reactionIndex)
{
    // Make sure that the reaction is valid for this site type.
    if (!RLC[reactionIndex*numberSiteTypesC+siteType]) return 0.0f;

    // Get the number of each reactant.
    float numberParticles1 = 0.0f;
    float numberParticles2 = 0.0f;
    for (int i=0; i<MPD_PARTICLES_PER_SITE; i++)
    {
        if (particles[i] > 0)
        {
            numberParticles1 += (particles[i] == D1C[reactionIndex])?(1.0f):(0.0f);
            numberParticles2 += (particles[i] == D2C[reactionIndex])?(1.0f):(0.0f);
        }
    }

    // Calculate the propensity according to the reaction order.
    if (reactionOrdersC[reactionIndex] == MPD_FIRST_ORDER_REACTION)
        return reactionRatesC[reactionIndex]*numberParticles1;
    else if (reactionOrdersC[reactionIndex] == MPD_SECOND_ORDER_REACTION)
        return reactionRatesC[reactionIndex]*numberParticles1*numberParticles2;
    else if (reactionOrdersC[reactionIndex] == MPD_SECOND_ORDER_SELF_REACTION)
        return reactionRatesC[reactionIndex]*numberParticles1*(numberParticles1-1.0f);
    return 0.0f;
}

inline __device__ float calculateReactionProbability(const float rate)
{
    #ifdef CUDA_DOUBLE_PRECISION
    return (float)(1.0-exp(-(double)rate));
    #else
    return (rate > 2e-4f)?(1.0f-__expf(-rate)):(rate);
    #endif
}

inline __device__ unsigned int checkForReaction(const unsigned int latticeIndex, const float reactionProbability, const unsigned long long timestepHash)
{
    return reactionProbability > 0.0f && getRandomHashFloat(latticeIndex, 1, 0, timestepHash) <= reactionProbability;
}

inline __device__ unsigned int determineReactionIndex(const uint8_t siteType, const uint8_t * __restrict__ particles, const unsigned int latticeIndex, const float totalReactionPropensity, const unsigned long long timestepHash)
{
    float randomPropensity = getRandomHashFloat(latticeIndex, 1, 1, timestepHash)*totalReactionPropensity;
    unsigned int reactionIndex = 0;
    for (int i=0; i<numberReactionsC; i++)
    {
        float propensity = calculateReactionPropensity(siteType, particles, i);
        if (propensity > 0.0f)
        {
            if (randomPropensity > 0.0f)
                reactionIndex = i;
            randomPropensity -= propensity;
        }

    }
    return reactionIndex;
}

__device__ void evaluateReaction(const unsigned int latticeIndex, const uint8_t siteType, uint8_t * __restrict__ particles, const unsigned int reactionIndex, unsigned int * siteOverflowList)
{
	// Copy the S matrix entries for this reaction.
    #if __CUDA_ARCH__ >= 200
	int8_t * S = (int8_t *)malloc(numberSpeciesC);
    #else
    int8_t S[256];
    #endif
	for (uint i=0, index=reactionIndex; i<numberSpeciesC; i++, index+=numberReactionsC)
		S[i] = SC[index];

    // Build the new site, copying particles that didn't react and removing those that did.
    int nextParticle=0;
    for (uint i=0; i<MPD_PARTICLES_PER_SITE; i++)
    {
    	uint8_t particle = particles[i];
        if (particle > 0)
        {
        	// If this particle was unaffected, copy it.
        	if (S[particle-1] >= 0)
        	{
        		particles[nextParticle++] = particle;
        	}

            // Otherwise, don't copy the particle and mark that we destroyed it.
        	else
        	{
        		S[particle-1]++;
        	}
        }
    }

    // Go through the S matrix and add in any new particles that were created.
    for (uint i=0; i<numberSpeciesC; i++)
    {
		for (uint j=0; j<S[i]; j++)
		{
			// If the particle will fit into the site, add it.
			if (nextParticle < MPD_PARTICLES_PER_SITE)
			{
				particles[nextParticle++] = i+1;
			}

			// Otherwise add it to the exception list.
			else
			{
				int exceptionIndex = atomicAdd(siteOverflowList, 1);
				if (exceptionIndex < TUNE_MPD_MAX_PARTICLE_OVERFLOWS)
				{
					siteOverflowList[(exceptionIndex*2)+1]=latticeIndex;
					siteOverflowList[(exceptionIndex*2)+2]=i+1;
				}
			}

		}
    }

    // Clear any remaining particles in the site.
    while (nextParticle < MPD_PARTICLES_PER_SITE)
    	particles[nextParticle++] = 0;

    // Free any allocated memory.
    #if __CUDA_ARCH__ >= 200
    free(S);
    #endif
}
