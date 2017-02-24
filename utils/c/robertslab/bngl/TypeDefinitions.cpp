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
#include "robertslab/bngl/TypeDefinitions.h"

using std::regex;
using std::regex_token_iterator;
using std::string;
using std::vector;

namespace robertslab {
namespace bngl {

ComponentClass::ComponentClass()
:valid(false)
{
}

ComponentClass::ComponentClass(string definitionString)
:valid(false)
{
    // Parse the component definitions.
    regex statePattern("([^~]+)");
    regex_token_iterator<string::iterator> endOfTokens;
    regex_token_iterator<std::string::iterator> tokens(definitionString.begin(), definitionString.end(), statePattern);
    while (tokens != endOfTokens)
    {
        string tokenString = *tokens++;
        if (name == "")
        {
            name = tokenString;
            valid = true;
        }
        else
        {
            states.push_back(tokenString);
        }
    }
}

bool ComponentClass::isValid()
{
    return valid;
}

string ComponentClass::getName()
{
    return name;
}

string ComponentClass::getString()
{
    std::stringstream ss;
    ss << name;
    for (int i=0; i<states.size(); i++)
    {
        ss << "~" << states[i];
    }

    return ss.str();
}

bool ComponentClass::isInstance(ComponentInstance* componentInstance)
{
    if (name != componentInstance->name) return false;
    if (states.size() == 0 && componentInstance->state == "") return true;
    for (int i=0; i<states.size(); i++)
        if (states[i] == componentInstance->state)
            return true;
    return false;
}

MoleculeClass::MoleculeClass()
:valid(false)
{
}

MoleculeClass::MoleculeClass(string definition)
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
            ComponentClass* component = new ComponentClass(*tokens++);
            components.push_back(component);
            if (!component->isValid()) valid = false;
        }
    }
}

bool MoleculeClass::isValid()
{
    return valid;
}

string MoleculeClass::getName()
{
    return name;
}

int MoleculeClass::getNumberComponents()
{
    return components.size();
}

ComponentClass* MoleculeClass::getComponent(int i)
{
    return components[i];
}

string MoleculeClass::getString()
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

bool MoleculeClass::isInstance(MoleculeInstance* moleculeInstance)
{
    if (name != moleculeInstance->name) return false;
    if (components.size() != moleculeInstance->components.size()) return false;
    for (int i=0; i<components.size(); i++)
        if (!components[i]->isInstance(moleculeInstance->components[i])) return false;

    return true;
}

vector<string> MoleculeClass::getStateCombinations(int componentIndex)
{
    vector<string> ret;
    if (componentIndex == components.size())
    {
        ret.push_back("");
    }
    else
    {
        vector<string> children = getStateCombinations(componentIndex+1);
        if (components[componentIndex]->states.size() == 0)
        {
            for (int j=0; j<children.size(); j++)
            {
                ret.push_back(components[componentIndex]->name+","+children[j]);
            }
        }
        else
        {
            for (int i=0; i<components[componentIndex]->states.size(); i++)
            {
                string state = components[componentIndex]->states[i];
                for (int j=0; j<children.size(); j++)
                {
                    string combination = components[componentIndex]->name+"~"+state;
                    if (children[j] != "") combination += ","+children[j];
                    ret.push_back(combination);
                }
            }
        }
    }
    return ret;
}

}
}
