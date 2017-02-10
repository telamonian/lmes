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

#ifndef ROBERTSLAB_BNGL_INSTANCEDEFINITIONS_H
#define ROBERTSLAB_BNGL_INSTANCEDEFINITIONS_H

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace robertslab {
namespace bngl {

class ComponentInstance
{
public:
    ComponentInstance();
    ComponentInstance(string definition);
    bool isValid();
    string getString();

protected:
    bool valid;
    string name;
    string state;
};

class MoleculeInstance
{
public:
    MoleculeInstance();
    MoleculeInstance(string definition);
    bool isValid();
    string getName();
    string getString();

protected:
    bool valid;
    string name;
    vector<ComponentInstance> components;
};

class ComplexInstance
{
public:
    ComplexInstance();
    ComplexInstance(string definition);
    bool isValid();
    string getString();

protected:
    bool valid;
    vector<MoleculeInstance> molecules;
};


}
}

#endif // MOLECULE_H
