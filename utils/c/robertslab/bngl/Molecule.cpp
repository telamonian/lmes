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

#include "robertslab/bngl/Molecule.h"

using std::regex;
using std::regex_token_iterator;
using std::string;
using std::vector;

namespace robertslab {
namespace bngl {

Component::Component()
{
}

Component::Component(string definitionString)
{
    // Parse the component definition.
    regex statePattern("([^~]+)");
    regex_token_iterator<string::iterator> endOfTokens;
    regex_token_iterator<std::string::iterator> tokens(definitionString.begin(), definitionString.end(), statePattern);
    while (tokens != endOfTokens)
    {
        string tokenString = *tokens++;
        if (name == "")
            name = tokenString;
        else
            states.push_back(tokenString);
    }
}

string Component::getString()
{
    std::stringstream ss;
    ss << name;
    for (int i=0; i<states.size(); i++)
    {
        ss << "~" << states[i];
    }

    return ss.str();
}

Molecule::Molecule()
{
}

Molecule::Molecule(string definition)
{
    regex moleculePattern("^\\s*(?:\\d*\\s+)?(\\w+)\\((.*)\\)$");
    std::smatch match;
    if (std::regex_match(definition, match, moleculePattern) && match.size() == 3)
    {
        name = match[1].str();

        string componentString = match[2].str();

        // Parse the component string.
        regex componentPattern("([^ ,]+)");
        regex_token_iterator<string::iterator> endOfTokens;
        regex_token_iterator<std::string::iterator> tokens(componentString.begin(), componentString.end(), componentPattern);
        while (tokens != endOfTokens)
        {
            components.push_back(Component(*tokens++));
        }
    }
}

Molecule::Molecule(string name, vector<Component> components)
:name(name),components(components)
{
}

string Molecule::getName()
{
    return name;
}

string Molecule::getString()
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

}
}
