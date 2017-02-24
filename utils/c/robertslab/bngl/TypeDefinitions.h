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

class MoleculeClass;

class ComponentClass
{
public:
    ComponentClass();
    ComponentClass(string definition);
    bool isValid();
    string getName();
    string getString();
    bool isInstance(ComponentInstance* componentInstance);

protected:
    bool valid;
    string name;
    vector<string> states;

    friend class MoleculeClass;
};

class MoleculeClass
{
public:
    MoleculeClass();
    MoleculeClass(string definition);
    bool isValid();
    string getName();
    int getNumberComponents();
    ComponentClass* getComponent(int i);
    string getString();
    bool isInstance(MoleculeInstance* moleculeInstance);
    vector<string> getStateCombinations(int componentIndex=0);    

protected:
    bool valid;
    string name;
    vector<ComponentClass*> components;
};


}
}

#endif // ROBERTSLAB_BNGL_TYPEDEFINITIONS_H
