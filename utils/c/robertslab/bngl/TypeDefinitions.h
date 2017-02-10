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

#ifndef ROBERTSLAB_BNGL_TYPEDEFINITIONS_H
#define ROBERTSLAB_BNGL_TYPEDEFINITIONS_H

#include <string>
#include <vector>

#include "robertslab/bngl/InstanceDefinitions.h"

using std::string;
using std::vector;

namespace robertslab {
namespace bngl {

class ComponentClass
{
public:
    ComponentClass();
    ComponentClass(string definition);
    ComponentClass(string name, string state);
    bool isValid();
    string getString();
    bool isInstance(ComponentInstance componentInstance);

public:
    bool valid;
    string name;
    vector<string> states;
};

class MoleculeClass
{
public:
    MoleculeClass();
    MoleculeClass(string definition);
    MoleculeClass(string name, vector<ComponentClass> components);
    bool isValid();
    string getName();
    string getString();
    bool isInstance(MoleculeInstance moleculeInstance);

public:
    bool valid;
    string name;
    vector<ComponentClass> components;
};

class ComplexClass
{
public:
    ComplexClass();
    ComplexClass(vector<MoleculeClass> molecules);
    bool isValid();
    string getString();

public:
    bool valid;
    vector<MoleculeClass> molecules;
};


}
}

#endif // ROBERTSLAB_BNGL_TYPEDEFINITIONS_H
