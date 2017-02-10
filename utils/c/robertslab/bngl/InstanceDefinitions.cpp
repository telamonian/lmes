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

using std::regex;
using std::regex_token_iterator;
using std::string;
using std::vector;

namespace robertslab {
namespace bngl {

ComponentInstance::ComponentInstance()
:valid(false)
{
}

ComponentInstance::ComponentInstance(string definition)
:valid(false)
{
    // Parse the component definition.
    regex statePattern("^([^~]+)(?:~([^~]+))?$");
    std::smatch match;
    if (std::regex_match(definition, match, statePattern) && (match.size() == 2 || match.size() == 3))
    {
        name = match[1].str();
        if (match.size() == 3) state = match[2].str();
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
            ComponentInstance component(componentString);
            components.push_back(component);
            if (!component.isValid()) valid = false;
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
        ss << (i==0?"":",") << components[i].getString();
    }
    ss << ")";

    return ss.str();
}

ComplexInstance::ComplexInstance()
:valid(false)
{
}

ComplexInstance::ComplexInstance(string definition)
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
        MoleculeInstance molecule(moleculeString);
        molecules.push_back(molecule);
        if (!molecule.isValid()) valid = false;
    }
}

bool ComplexInstance::isValid()
{
    return valid;
}

string ComplexInstance::getString()
{
    std::stringstream ss;
    for (int i=0; i<molecules.size(); i++)
    {
        ss << (i==0?"":".") << molecules[i].getString();
    }

    return ss.str();
}

}
}
