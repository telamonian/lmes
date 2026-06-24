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

#include <limits>
#include <list>

#include "lm/cme/ReactionModel.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/Types.h"

using std::list;

namespace lm {
namespace cme {

ReactionModel::ReactionModel(const uint numberSpecies, const uint numberReactions)
:numberSpecies(numberSpecies),numberSpeciesToTrack(numberSpecies),numberReactions(numberReactions),S(ndarray<int>(utuple(numberSpecies,numberReactions))),D(ndarray<uint>(utuple(numberSpecies,numberReactions))),propensityFunctions(NULL),numberDependentSpecies(NULL),dependentSpecies(NULL),dependentSpeciesChange(NULL),numberDependentReactions(NULL),dependentReactions(NULL)
{
    // Allocate propensity function tables.
    propensityFunctions = new lm::me::PropensityFunction*[numberReactions];
    memset(propensityFunctions, 0, numberReactions*sizeof(*propensityFunctions));

    // Allocate the species dependency tables.
    numberDependentSpecies = new uint[numberReactions];
    memset(numberDependentSpecies, 0, numberReactions*sizeof(*numberDependentSpecies));
    dependentSpecies = new uint*[numberReactions];
    memset(dependentSpecies, 0, numberReactions*sizeof(*dependentSpecies));
    dependentSpeciesChange = new int*[numberReactions];
    memset(dependentSpeciesChange, 0, numberReactions*sizeof(*dependentSpeciesChange));

    // Allocate the reaction dependency tables.
    numberDependentReactions = new uint[numberReactions];
    memset(numberDependentReactions, 0, numberReactions*sizeof(*numberDependentReactions));
    dependentReactions = new uint*[numberReactions];
    memset(dependentReactions, 0, numberReactions*sizeof(*dependentReactions));
}

ReactionModel::ReactionModel(const lm::input::ReactionModel& rm)
:numberSpecies(rm.number_species()),numberSpeciesToTrack(rm.number_species()),numberReactions((uint)rm.number_reactions()),S(ndarray<int>(utuple(numberSpecies,numberReactions),rm.stoichiometric_matrix().data())),D(ndarray<uint>(utuple(numberSpecies,numberReactions),rm.dependency_matrix().data())),propensityFunctions(NULL),numberDependentSpecies(NULL),dependentSpecies(NULL),dependentSpeciesChange(NULL),numberDependentReactions(NULL),dependentReactions(NULL)
{
    // Allocate propensity function tables.
    propensityFunctions = new lm::me::PropensityFunction*[numberReactions];
    memset(propensityFunctions, 0, numberReactions*sizeof(*propensityFunctions));

    // Create the propensity functions table.
    lm::me::PropensityFunctionFactory fs;
    for (uint i=0; i<numberReactions; i++)
    {
        // Create the rate constant tuple.
        tuple<double> k(rm.reaction(i).rate_constant_size(), rm.reaction(i).rate_constant().data());

        // Get the propensity function.
        propensityFunctions[i] = fs.createPropensityFunction(rm.reaction(i).type(), i, S, D, k);
    }

    // Allocate the species dependency tables.
    numberDependentSpecies = new uint[numberReactions];
    memset(numberDependentSpecies, 0, numberReactions*sizeof(*numberDependentSpecies));
    dependentSpecies = new uint*[numberReactions];
    memset(dependentSpecies, 0, numberReactions*sizeof(*dependentSpecies));
    dependentSpeciesChange = new int*[numberReactions];
    memset(dependentSpeciesChange, 0, numberReactions*sizeof(*dependentSpeciesChange));

    // Create the species dependency tables from the S matrix.
    for (uint col=0; col<numberReactions; col++)
    {
        numberDependentSpecies[col]=0;
        for (uint row=0; row<numberSpecies; row++)
            if (S[utuple(row,col)] != 0)
                numberDependentSpecies[col]++;
        dependentSpecies[col] = new uint[numberDependentSpecies[col]];
        dependentSpeciesChange[col] = new int[numberDependentSpecies[col]];
        for (uint row=0, k=0; row<numberSpecies; row++)
        {
            if (S[utuple(row,col)] != 0 && k < numberDependentSpecies[col])
            {
                dependentSpecies[col][k] = row;
                dependentSpeciesChange[col][k] = S[utuple(row,col)];
                k++;
            }
        }
    }

    // Allocate the reaction dependency tables.
    numberDependentReactions = new uint[numberReactions];
    memset(numberDependentReactions, 0, numberReactions*sizeof(*numberDependentReactions));
    dependentReactions = new uint*[numberReactions];
    memset(dependentReactions, 0, numberReactions*sizeof(*dependentReactions));

    // Create the reaction dependency tables from the other tables.
    for (uint r=0; r<numberReactions; r++)
    {
        list<uint> dependentReactionList;

        // Go through all of the species changed by this reaction.
        for (uint d=0; d<numberDependentSpecies[r]; d++)
        {
            uint s = dependentSpecies[r][d];

            // Find all of the reactions that depend on this species.
            for (uint col=0; col<numberReactions; col++)
            {
                if (D[utuple(s,col)] > 0) dependentReactionList.push_back(col);
            }
        }

        // Eliminate any duplicates from the list.
        dependentReactionList.sort();
        dependentReactionList.unique();

        // Create the table.
        numberDependentReactions[r] = dependentReactionList.size();
        dependentReactions[r] = new uint[numberDependentReactions[r]];
        uint i=0;
        for (list<uint>::iterator it=dependentReactionList.begin(); it != dependentReactionList.end() && i<numberDependentReactions[r]; it++, i++)
        {
            dependentReactions[r][i] = *it;
        }
    }
}

ReactionModel::~ReactionModel()
{
    // Free the propensity function arguments and array.
    if (propensityFunctions != NULL)
    {
        for (uint i=0; i<numberReactions; i++)
        {
            if (propensityFunctions[i] != NULL) delete propensityFunctions[i]; propensityFunctions[i]=NULL;
        }
        delete[] propensityFunctions; propensityFunctions = NULL;
    }

    // Free the species dependency tables.
    if (numberDependentSpecies != NULL) delete[] numberDependentSpecies; numberDependentSpecies = NULL;
    if (dependentSpecies != NULL)
    {
        for (uint i=0; i<numberReactions; i++)
        {
            if (dependentSpecies[i] != NULL)
                delete[] dependentSpecies[i];
        }
        delete[] dependentSpecies;
        dependentSpecies = NULL;
    }
    if (dependentSpeciesChange != NULL)
    {
        for (uint i=0; i<numberReactions; i++)
        {
            if (dependentSpeciesChange[i] != NULL)
                delete[] dependentSpeciesChange[i];
        }
        delete[] dependentSpeciesChange;
        dependentSpeciesChange = NULL;
    }

    // Free the reaction dependency tables.
    if (numberDependentReactions != NULL) delete[] numberDependentReactions; numberDependentReactions = NULL;
    if (dependentReactions != NULL)
    {
        for (uint i=0; i<numberReactions; i++)
        {
            if (dependentReactions[i] != NULL)
                delete[] dependentReactions[i];
        }
        delete[] dependentReactions;
        dependentReactions = NULL;
    }
}

void ReactionModel::setPropensityFunction(uint reaction, lm::me::PropensityFunction* propensityFunction)
{
    if (reaction >= numberReactions) throw InvalidArgException("reaction", "reaction index exceeded the number of reactions",reaction);
    if (propensityFunctions[reaction] != NULL) delete propensityFunctions[reaction];
    propensityFunctions[reaction] = propensityFunction;
}

}
}
