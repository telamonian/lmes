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

#include "robertslab/bngl/PatternDefinitions.h"

using std::regex;
using std::regex_token_iterator;
using std::string;
using std::vector;

namespace robertslab {
namespace bngl {

ComponentPattern::ComponentPattern()
:valid(false)
{
}

ComponentPattern::ComponentPattern(string definition)
:valid(false)
{
    // Parse the component definition.
    regex statePattern("^([^~!]+)(?:~([^~!]+))?(?:!([^~!]+))?$");
    std::smatch match;
    if (std::regex_match(definition, match, statePattern) && match.size() == 4)
    {
        name = match[1].str();
        state = match[2].str();
        bond = match[3].str();
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
    if (bond != "") ss << "!" << bond;
    return ss.str();
}

MoleculePattern::MoleculePattern()
:valid(false),null(false)
{
}

MoleculePattern::MoleculePattern(string definition)
:valid(false),null(false)
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
            ComponentPattern component(componentString);
            components.push_back(component);
            if (!component.isValid()) valid = false;
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
    for (int i=0; i<components.size(); i++)
    {
        ss << (i==0?"":",") << components[i].getString();
    }
    ss << ")";

    return ss.str();
}

ComplexPattern::ComplexPattern()
:valid(false)
{
}

ComplexPattern::ComplexPattern(string definition)
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
        MoleculePattern molecule(moleculeString);
        molecules.push_back(molecule);
        if (!molecule.isValid()) valid = false;
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
        ss << (i==0?"":".") << molecules[i].getString();
    }

    return ss.str();
}

ReactionPattern::ReactionPattern()
:valid(false)
{
}

ReactionPattern::ReactionPattern(string lhs, string rhs, bool reversible, double forwardRate, double backwardRate)
:valid(false),reversible(reversible),forwardRate(forwardRate),backwardRate(backwardRate)
{
    valid = true;
    substrates = parseComplexes(lhs);
    products = parseComplexes(rhs);
}

vector<ComplexPattern> ReactionPattern::parseComplexes(string definition)
{
    // Parse the molecule definitions.
    vector<ComplexPattern> complexes;
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
            ComplexPattern complex(complexString);
            //printf(":%s:%s========%s:%d\n",tokenString.c_str(),complexString.c_str(),complex.getString().c_str(),complex.isValid());
            complexes.push_back(complex);
            if (!complex.isValid()) valid = false;
        }
        else
        {
            valid = false;
        }
    }
    return complexes;
}

bool ReactionPattern::isValid()
{
    return valid;
}

string ReactionPattern::getString()
{
    std::stringstream ss;

    for (int i=0; i<substrates.size(); i++)
        ss << (i==0?"":" + ") << substrates[i].getString();

    if (reversible)
        ss << " <-> ";
    else
        ss << " -> ";

    for (int i=0; i<products.size(); i++)
        ss << (i==0?"":" + ") << products[i].getString();

    ss << " " << forwardRate;
    if (reversible) ss << ", " << backwardRate;

    return ss.str();
}

}
}

