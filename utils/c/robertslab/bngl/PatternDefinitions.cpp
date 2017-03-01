/*
 * Copyright 2017 Johns Hopkins University
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

#include <regex>
#include <iostream>
#include <list>
#include <sstream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "robertslab/bngl/InstanceDefinitions.h"
#include "robertslab/bngl/PatternDefinitions.h"
#include "robertslab/bngl/TypeDefinitions.h"

using std::list;
using std::regex;
using std::regex_token_iterator;
using std::set;
using std::string;
using std::vector;

namespace robertslab {
namespace bngl {

ComponentPattern::ComponentPattern()
:molecule(NULL),valid(false),bond(NULL)
{
}

ComponentPattern::ComponentPattern(MoleculePattern* molecule, string definition)
:type(NULL),molecule(molecule),valid(false),bond(NULL)
{
    // Parse the component definition.
    regex statePattern("^([^~!]+)(?:~([^~!]+))?(?:!([^~!]+))?$");
    std::smatch match;
    if (std::regex_match(definition, match, statePattern) && match.size() == 4)
    {
        name = match[1].str();
        state = match[2].str();
        bondName = match[3].str();
        valid = true;
    }
    //printf("%s=\t%s:%s:%s:%d\n",definition.c_str(),name.c_str(),state.c_str(),bond.c_str(),valid);
}

bool ComponentPattern::isValid()
{
    return valid;
}

string ComponentPattern::getString()
{
    std::stringstream ss;
    ss << name;
    if (state != "") ss << "~" << state;
    if (bondName != "") ss << "!" << bondName;
    return ss.str();
}

MoleculePattern::MoleculePattern()
:valid(false),null(false)
{
}

MoleculePattern::MoleculePattern(string definition, map<string,MoleculeClass*> moleculeClasses)
:type(NULL),valid(false),null(false)
{
    // See if this is a null pattern.
    if (definition == "0")
    {
        valid = true;
        null = true;
        name = "0";
        return;
    }

    regex moleculePattern("^(\\w+)\\(([^\\)]*)\\)$");
    std::smatch match;
    if (std::regex_match(definition, match, moleculePattern) && match.size() == 3)
    {
        // Get the name.
        name = match[1].str();

        // Figure out the class.
        if (moleculeClasses.count(name) > 0)
        {
            valid = true;
            type = moleculeClasses[name];

            // Create the empty components.
            set<int> unusedComponents;
            for (int i=0; i<type->getNumberComponents(); i++)
            {
                components.push_back(NULL);
                unusedComponents.insert(i);
            }

            // Parse the components.
            string componentString = match[2].str();
            regex componentPattern("([^ ,]+)");
            regex_token_iterator<string::iterator> endOfTokens;
            regex_token_iterator<std::string::iterator> tokens(componentString.begin(), componentString.end(), componentPattern);
            while (tokens != endOfTokens)
            {
                string componentString = *tokens++;
                ComponentPattern* component = new ComponentPattern(this, componentString);
                if (!component->isValid()) valid = false;

                // Find the index we should use for this component.
                bool foundComponent = false;
                for (int i=0; i<type->getNumberComponents(); i++)
                {
                    if (unusedComponents.count(i) == 1 && type->getComponent(i)->getName() == component->name)
                    {
                        component->type = type->getComponent(i);
                        components[i] = component;
                        unusedComponents.erase(i);
                        foundComponent = true;
                        break;
                    }
                }
                if (!foundComponent) valid = false;
            }
        }
    }    
}

bool MoleculePattern::isValid()
{
    return valid;
}

bool MoleculePattern::isNull()
{
    return null;
}

string MoleculePattern::getName()
{
    return name;
}

string MoleculePattern::getString()
{
    if (null) return "0";

    std::stringstream ss;
    ss << name << "(";
    bool firstPrinted = true;
    for (int i=0; i<components.size(); i++)
    {
        if (components[i] != NULL)
        {
            ss << (firstPrinted?"":",") << components[i]->getString();
            firstPrinted = false;
        }
    }
    ss << ")";

    return ss.str();
}

bool MoleculePattern::matches(MoleculeInstance* instance)
{
    if (name != instance->getName()) return false;
    if (components.size() != instance->components.size()) return false;

    // Check the bonding state of any component that is specified in the pattern.
    for (int i=0; i<components.size(); i++)
    {
        if (components[i] != NULL)
        {
            // See if the pattern specifies a bond.
            if (components[i]->bond != NULL)
            {
                // The pattern has a bond, so make sure the instance does to.
                if (instance->components[i]->bond == NULL) return false;

                // TODO: make sure the bond is the same.
            }
            else
            {
                // Otherwise the pattern didn't have a bond, so make sure the instance doesn't either.
                if (instance->components[i]->bond != NULL) return false;
            }
        }
    }

    return true;
}

bool MoleculePattern::matches(MoleculePattern* comp)
{
    if (name != comp->name) return false;
    if (components.size() != comp->components.size()) return false;
    return true;
}

int MoleculePattern::getMaxNumberEdges()
{
    return components.size();
}

Vertex* MoleculePattern::getEdge(int i)
{
    if (components[i] != NULL && components[i]->bond != NULL)
        return components[i]->bond->molecule;
    return NULL;
}

void MoleculePattern::addEdge(int sourceIndex, Vertex* dest, int destIndex)
{
    MoleculePattern* destMolecule = dynamic_cast<MoleculePattern*>(dest);
    if (destMolecule == NULL) throw std::runtime_error("could not cast to MoleculePattern in MoleculePattern::addEdge");
    if (components[sourceIndex] != NULL && destMolecule->components[destIndex] != NULL)
    {
        components[sourceIndex]->bondName = "*";
        components[sourceIndex]->bond = destMolecule->components[destIndex];
    }
}

void MoleculePattern::removeEdge(int i)
{
    if (components[i] != NULL)
    {
        components[i]->bondName = "";
        components[i]->bond = NULL;
    }
}

bool MoleculePattern::matches(Vertex* comp)
{
    if (comp == NULL) return false;

    if (dynamic_cast<MoleculeInstance*>(comp))
    {
        return matches((MoleculeInstance*)comp);
    }
    else if (dynamic_cast<MoleculePattern*>(comp))
    {
        return matches((MoleculePattern*)comp);
    }

    return false;
}

ComplexPattern::ComplexPattern()
:valid(false)
{
}

ComplexPattern::ComplexPattern(string definition, map<string,MoleculeClass*> moleculeClasses)
:valid(false)
{
    valid = true;

    // Parse the molecule definitions.
    regex moleculePattern("([^\\.]+)");
    regex_token_iterator<string::iterator> endOfTokens;
    regex_token_iterator<std::string::iterator> tokens(definition.begin(), definition.end(), moleculePattern);
    while (tokens != endOfTokens)
    {
        string moleculeString = *tokens++;
        MoleculePattern* molecule = new MoleculePattern(moleculeString, moleculeClasses);
        molecules.push_back(molecule);
        if (!molecule->isValid()) valid = false;
    }

    // Go through and establish the connectivity using the bond names.
    for (int m1=0; m1<molecules.size(); m1++)
    {
        for (int c1=0; c1<molecules[m1]->components.size(); c1++)
        {
            if (molecules[m1]->components[c1] != NULL && molecules[m1]->components[c1]->bondName != "")
            {
                int matches=0;
                for (int m2=0; m2<molecules.size(); m2++)
                {
                    for (int c2=0; c2<molecules[m2]->components.size(); c2++)
                    {
                        if ((m1 != m2 || c1 != c2) && molecules[m2]->components[c2] != NULL && molecules[m1]->components[c1]->bondName == molecules[m2]->components[c2]->bondName)
                        {
                            molecules[m1]->components[c1]->bond = molecules[m2]->components[c2];
                            matches++;
                        }
                    }
                }

                if (matches != 1) throw std::invalid_argument("inconsistent number of bond names in ComplexPattern::ComplexPattern");
            }
            else if (molecules[m1]->components[c1] != NULL)
            {
                molecules[m1]->components[c1]->bond = NULL;
            }
        }
    }
}

bool ComplexPattern::isValid()
{
    return valid;
}

string ComplexPattern::getString()
{
    std::stringstream ss;
    for (int i=0; i<molecules.size(); i++)
    {
        ss << (i==0?"":".") << molecules[i]->getString();
    }

    return ss.str();
}

int ComplexPattern::getNumberVertices()
{
    return molecules.size();
}

Vertex* ComplexPattern::getVertex(int i)
{
    return molecules[i];
}

ReactantPattern::ReactantPattern()
:valid(false)
{
}

ReactantPattern::ReactantPattern(string definition, map<string,MoleculeClass*> moleculeClasses)
:valid(false)
{
    valid = true;

    // Parse the molecule definitions.
    std::smatch match;
    regex tokenPattern("\\s*\\S+\\s*\\+?");
    regex complexPattern("\\s*(\\S+)\\s*\\+?");
    regex_token_iterator<string::iterator> endOfTokens;
    regex_token_iterator<std::string::iterator> tokens(definition.begin(), definition.end(), tokenPattern);
    while (tokens != endOfTokens)
    {
        string tokenString = *tokens++;
        if (std::regex_match(tokenString, match, complexPattern) && match.size() == 2)
        {
            string complexString = match[1].str();
            ComplexPattern* complex = new ComplexPattern(complexString, moleculeClasses);
            reactants.push_back(complex);
            if (!complex->isValid()) valid = false;
        }
        else
        {
            valid = false;
        }
    }
}

bool ReactantPattern::isValid()
{
    return valid;
}

string ReactantPattern::getString()
{
    std::stringstream ss;
    for (int i=0; i<reactants.size(); i++)
        ss << (i==0?"":" + ") << reactants[i]->getString();
    return ss.str();
}

int ReactantPattern::getNumberReactants()
{
    return reactants.size();
}

ComplexPattern* ReactantPattern::getReactant(int index)
{
    return reactants[index];
}

int ReactantPattern::getNumberVertices()
{
    int ret=0;
    for (int i=0; i<reactants.size(); i++)
        ret += reactants[i]->getNumberVertices();
    return ret;
}

Vertex* ReactantPattern::getVertex(int index)
{
    for (int i=0; i<reactants.size(); i++)
    {
        if (index <  reactants[i]->getNumberVertices())
            return reactants[i]->getVertex(index);
        index -= reactants[i]->getNumberVertices();
    }
    throw std::out_of_range("index out of range in call to ReactantPattern::getVertex");
}


ReactionPattern::ReactionPattern()
:valid(false)
{
}

ReactionPattern::ReactionPattern(string lhs, string rhs, double rate, map<string,MoleculeClass*> moleculeClasses)
:valid(false),rate(rate)
{
    valid = true;

    // Parse the substrates and products.
    substrates = new ReactantPattern(lhs, moleculeClasses);
    products = new ReactantPattern(rhs, moleculeClasses);
    if (!substrates->isValid() || !products->isValid()) valid = false;

    // Create a mapping of substrates to products.
    substrateToProductMapping = substrates->findGraphMapping(products);
}

bool ReactionPattern::isValid()
{
    return valid;
}

string ReactionPattern::getString(bool includeRate)
{
    std::stringstream ss;
    ss << substrates->getString();
    ss << " -> ";
    ss << products->getString();
    if (includeRate) ss << " " << rate;
    return ss.str();
}

ReactantPattern* ReactionPattern::getSubstrates()
{
    return substrates;
}

ReactantPattern* ReactionPattern::getProducts()
{
    return products;
}

double ReactionPattern::getRate()
{
    return rate;
}

GraphMapping ReactionPattern::getSubstrateToProductMapping()
{
    return substrateToProductMapping;
}

}
}

