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
#include <sstream>
#include <string>
#include <vector>

#include "robertslab/bngl/InstanceDefinitions.h"
#include "robertslab/bngl/PatternDefinitions.h"

using std::regex;
using std::regex_token_iterator;
using std::string;
using std::vector;

namespace robertslab {
namespace bngl {

ComponentInstance::ComponentInstance()
:molecule(NULL),valid(false),bond(NULL)
{
}

ComponentInstance::ComponentInstance(MoleculeInstance*molecule, string definition)
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

bool ComponentInstance::isValid()
{
    return valid;
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

bool MoleculeInstance::isValid()
{
    return valid;
}

string MoleculeInstance::getName()
{
    return name;
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

bool MoleculeInstance::matches(MoleculeInstance* comp)
{
    if (name != comp->name) return false;
    if (components.size() != comp->components.size()) return false;
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
        }
    }
}

bool ComplexInstance::isValid()
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

}
}
