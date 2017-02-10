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

#ifndef ROBERTSLAB_BNGL_PATTERNDEFINITIONS_H
#define ROBERTSLAB_BNGL_PATTERNDEFINITIONS_H

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace robertslab {
namespace bngl {

class ComponentPattern
{
public:
    ComponentPattern();
    ComponentPattern(string definition);
    bool isValid();
    string getString();

public:
    bool valid;
    string name;
    string state;
    string bond;
};

class MoleculePattern
{
public:
    MoleculePattern();
    MoleculePattern(string definition);
    bool isValid();
    bool isNull();
    string getName();
    string getString();

public:
    bool valid;
    bool null;
    string name;
    vector<ComponentPattern> components;
};

class ComplexPattern
{
public:
    ComplexPattern();
    ComplexPattern(string definition);
    bool isValid();
    string getString();

public:
    bool valid;
    vector<MoleculePattern> molecules;
};

class ReactionPattern
{
public:
    ReactionPattern();
    ReactionPattern(string lhs, string rhs, bool reversible, double forwardRate, double reverseRate=0.0);
    bool isValid();
    string getString();

public:
    vector<ComplexPattern> parseComplexes(string definition);
    bool valid;
    bool reversible;
    vector<ComplexPattern> substrates;
    vector<ComplexPattern> products;
    double forwardRate;
    double backwardRate;
};


}
}

#endif // MOLECULE_H
