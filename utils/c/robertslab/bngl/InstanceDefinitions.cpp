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
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "robertslab/bngl/InstanceDefinitions.h"
#include "robertslab/bngl/PatternDefinitions.h"

using std::regex;
using std::regex_token_iterator;
using std::set;
using std::string;
using std::vector;

namespace robertslab {
namespace bngl {

ComponentInstance::ComponentInstance()
:molecule(NULL),valid(false),bond(NULL)
{
}

ComponentInstance::ComponentInstance(MoleculeInstance* molecule, string definition)
:molecule(molecule),valid(false),bond(NULL)
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
}

ComponentInstance::ComponentInstance(MoleculeInstance* molecule, const ComponentInstance& other)
:molecule(molecule),valid(other.valid),name(other.name),state(other.state),bondName(other.bondName),bond(NULL)
{
}

bool ComponentInstance::isValid()
{
    return valid;
}

void ComponentInstance::setState(string newState)
{
    state = newState;
}

string ComponentInstance::getString()
{
    std::stringstream ss;
    ss << name;
    if (state != "") ss << "~" << state;
    if (bondName != "") ss << "!" << bondName;
    return ss.str();
}

MoleculeInstance::MoleculeInstance()
:valid(false)
{
}

MoleculeInstance::MoleculeInstance(string definition)
:valid(false)
{
    regex moleculePattern("^(\\w+)\\(([^\\)]*)\\)$");
    std::smatch match;
    if (std::regex_match(definition, match, moleculePattern) && match.size() == 3)
    {
        valid = true;
        name = match[1].str();

        // Parse the components.
        string componentString = match[2].str();
        regex componentPattern("([^ ,]+)");
        regex_token_iterator<string::iterator> endOfTokens;
        regex_token_iterator<std::string::iterator> tokens(componentString.begin(), componentString.end(), componentPattern);
        while (tokens != endOfTokens)
        {
            string componentString = *tokens++;
            ComponentInstance* component = new ComponentInstance(this, componentString);
            components.push_back(component);
            if (!component->isValid()) valid = false;
        }
    }
}

MoleculeInstance::MoleculeInstance(const MoleculeInstance& other)
:valid(other.valid),name(other.name)
{
    for (int i=0; i<other.components.size(); i++)
        components.push_back(new ComponentInstance(this, *other.components[i]));
}

bool MoleculeInstance::isValid()
{
    return valid;
}

string MoleculeInstance::getName()
{
    return name;
}

ComponentInstance* MoleculeInstance::getComponent(int index)
{
    return components[index];
}

string MoleculeInstance::getString()
{
    std::stringstream ss;
    ss << name << "(";
    for (int i=0; i<components.size(); i++)
    {
        ss << (i==0?"":",") << components[i]->getString();
    }
    ss << ")";

    return ss.str();
}

bool MoleculeInstance::matches(MoleculeInstance* instance)
{
    if (name != instance->name) return false;
    if (components.size() != instance->components.size()) return false;

    // Check the bonding state of the components.
    for (int i=0; i<components.size(); i++)
    {
        // Make sure the components have the same name.
        if (components[i]->name != instance->components[i]->name) return false;

        // See if the source has a bond.
        if (components[i]->bond != NULL)
        {
            // The source has a bond, so make sure the target does to.
            if (instance->components[i]->bond == NULL) return false;

            // TODO: make sure the bond is the same.
        }
        else
        {
            // Otherwise the source didn't have a bond, so make sure the target doesn't either.
            if (instance->components[i]->bond != NULL) return false;
        }

        // Make sure the component state is the same.
        if (components[i]->state != instance->components[i]->state) return false;
    }

    return true;
}

int MoleculeInstance::getMaxNumberEdges()
{
    return components.size();
}

Vertex* MoleculeInstance::getEdge(int i)
{
    if (components[i]->bond != NULL)
        return components[i]->bond->molecule;
    return NULL;
}

void MoleculeInstance::addEdge(int sourceIndex, Vertex* dest, int destIndex)
{
    MoleculeInstance* destMolecule = dynamic_cast<MoleculeInstance*>(dest);
    if (destMolecule == NULL) throw std::runtime_error("could not cast to MoleculeInstance in MoleculeInstance::addEdge");
    components[sourceIndex]->bondName = "*";
    components[sourceIndex]->bond = destMolecule->components[destIndex];
}

void MoleculeInstance::removeEdge(int index)
{
    components[index]->bondName = "";
    components[index]->bond = NULL;
}

bool MoleculeInstance::matches(Vertex* comp)
{
    if (dynamic_cast<MoleculeInstance*>(comp))
    {
        return matches((MoleculeInstance*)comp);
    }
    else if (dynamic_cast<MoleculePattern*>(comp))
    {
        return ((MoleculePattern*)comp)->matches(this);
    }

    return false;
}


ComplexInstance::ComplexInstance()
:valid(false)
{
}

ComplexInstance::ComplexInstance(string definition, double count)
:valid(false),count(count)
{
    valid = true;

    // Parse the molecule definitions.
    regex moleculePattern("([^\\.]+)");
    regex_token_iterator<string::iterator> endOfTokens;
    regex_token_iterator<std::string::iterator> tokens(definition.begin(), definition.end(), moleculePattern);
    while (tokens != endOfTokens)
    {
        string moleculeString = *tokens++;
        MoleculeInstance* molecule = new MoleculeInstance(moleculeString);
        molecules.push_back(molecule);
        if (!molecule->isValid()) valid = false;
    }

    // Connect all of the bonds.
    connectBonds();
}

ComplexInstance::ComplexInstance(set<Vertex*> connectedMolecules)
:valid(false)
{
    valid = true;

    // Add all of the moelcules.
    for (auto it=connectedMolecules.begin(); it != connectedMolecules.end(); it++)
    {
        MoleculeInstance* molecule = dynamic_cast<MoleculeInstance*>(*it);
        if (molecule == NULL) throw std::runtime_error("could not cast to MoleculeInstance in ComplexInstance::ComplexInstance");
        molecules.push_back(molecule);
        if (!molecule->isValid()) valid = false;
    }

    // Construct the bond names.
    constructBondNames();
}


ComplexInstance::ComplexInstance(const ComplexInstance& other)
:valid(other.valid),count(0.0)
{
    // Create the new molecules.
    for (int i=0; i<other.molecules.size(); i++)
        molecules.push_back(new MoleculeInstance(*other.molecules[i]));

    // Connect all of the bonds.
    connectBonds();
}

void ComplexInstance::connectBonds()
{
    // Go through and establish the connectivity using the bond names.
    for (int m1=0; m1<molecules.size(); m1++)
    {
        for (int c1=0; c1<molecules[m1]->components.size(); c1++)
        {
            if (molecules[m1]->components[c1]->bondName != "")
            {
                int matches=0;
                for (int m2=0; m2<molecules.size(); m2++)
                {
                    for (int c2=0; c2<molecules[m2]->components.size(); c2++)
                    {
                        if ((m1 != m2 || c1 != c2) && molecules[m1]->components[c1]->bondName == molecules[m2]->components[c2]->bondName)
                        {
                            molecules[m1]->components[c1]->bond = molecules[m2]->components[c2];
                            matches++;
                        }
                    }
                }

                if (matches != 1) throw std::invalid_argument("inconsistent number of bond names in ComplexInstance::ComplexInstance");
            }
            else
            {
                molecules[m1]->components[c1]->bond = NULL;
            }
        }
    }
}

void ComplexInstance::constructBondNames()
{
    // Go through and reset all of the bond names.
    for (int i=0; i<molecules.size(); i++)
    {
        for (int j=0; j<molecules[i]->components.size(); j++)
        {
            if (molecules[i]->components[j] != NULL)
            {
                molecules[i]->components[j]->bondName = "";
            }
        }
    }

    // Go through and specify all fot he bond names.
    int nextBond=1;
    for (int i=0; i<molecules.size(); i++)
    {
        for (int j=0; j<molecules[i]->components.size(); j++)
        {
            if (molecules[i]->components[j] != NULL)
            {
                if (molecules[i]->components[j]->bond != NULL && molecules[i]->components[j]->bondName == "")
                {
                    // Set the name of this bond and the target.
                    molecules[i]->components[j]->bondName = std::to_string(nextBond);
                    molecules[i]->components[j]->bond->bondName = std::to_string(nextBond);
                    nextBond++;
                }
            }
        }
    }
}

bool ComplexInstance::isValid() const
{
    return valid;
}

string ComplexInstance::getString(bool withCounts)
{
    std::stringstream ss;
    for (int i=0; i<molecules.size(); i++)
        ss << (i==0?"":".") << molecules[i]->getString();
    if (withCounts) ss << " " << count;
    return ss.str();
}

string ComplexInstance::getString()
{
    return getString(false);
}


int ComplexInstance::getNumberMolecules()
{
    return molecules.size();
}

MoleculeInstance* ComplexInstance::getMolecule(int i)
{
    return molecules[i];
}

int ComplexInstance::getNumberVertices()
{
    return molecules.size();
}

Vertex* ComplexInstance::getVertex(int i)
{
    return molecules[i];
}

ReactantInstance::ReactantInstance()
:valid(false)
{
}

ReactantInstance::ReactantInstance(ComplexInstance* complex1)
{
    valid = true;
    if (!complex1->isValid()) valid = false;
    complexes.push_back(complex1);
}

ReactantInstance::ReactantInstance(ComplexInstance* complex1, ComplexInstance* complex2)
{
    valid = true;
    if (!complex1->isValid()) valid = false;
    if (!complex2->isValid()) valid = false;
    complexes.push_back(complex1);
    complexes.push_back(complex2);
}

ReactantInstance::ReactantInstance(const ReactantInstance& other)
:valid(other.valid)
{
    // Create the new complexes.
    for (int i=0; i<other.complexes.size(); i++)
        complexes.push_back(new ComplexInstance(*other.complexes[i]));

}

bool ReactantInstance::isValid()
{
    return valid;
}

string ReactantInstance::getString()
{
    std::stringstream ss;
    for (int i=0; i<complexes.size(); i++)
        ss << (i==0?"":" + ") << complexes[i]->getString();
    return ss.str();
}

int ReactantInstance::getNumberComplexes()
{
    return complexes.size();
}

ComplexInstance* ReactantInstance::getComplex(int index)
{
    return complexes[index];
}

int ReactantInstance::getNumberVertices()
{
    int ret=0;
    for (int i=0; i<complexes.size(); i++)
        ret += complexes[i]->getNumberVertices();
    return ret;
}

Vertex* ReactantInstance::getVertex(int index)
{
    for (int i=0; i<complexes.size(); i++)
    {
        if (index <  complexes[i]->getNumberVertices())
            return complexes[i]->getVertex(index);
        index -= complexes[i]->getNumberVertices();
    }
    throw std::out_of_range("index out of range in call to ReactantInstance::getVertex");
}

void ReactantInstance::recreateComplexes()
{
    // Get a list of vertices that anchor a set of connected subgraphs.
    list<Vertex*> anchorVertices = findConnectedSubgraphs();

    // Clear the complexes list.
    complexes.clear();

    // Go through each anchor vertex.
    for (auto it=anchorVertices.begin(); it != anchorVertices.end(); it++)
    {
        MoleculeInstance* anchorMolecule = dynamic_cast<MoleculeInstance*>(*it);
        if (anchorMolecule == NULL) throw std::runtime_error("could not cast to MoleculeInstance in ReactantInstance::recreateComplexes");
        complexes.push_back(new ComplexInstance(anchorMolecule->getConnectedVertices()));
    }

}

}
}
