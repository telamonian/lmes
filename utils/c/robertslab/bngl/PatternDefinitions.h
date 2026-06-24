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

#include <map>
#include <string>
#include <vector>

#include "robertslab/bngl/InstanceDefinitions.h"
#include "robertslab/graph/Graph.h"

using std::map;
using std::string;
using std::vector;

using robertslab::graph::GraphMapping;

namespace robertslab {
namespace bngl {

class ComplexPattern;
class MoleculePattern;
class ComponentClass;
class MoleculeClass;

class ComponentPattern
{
public:
    ComponentPattern();
    ComponentPattern(MoleculePattern* molecule, string definition);
    bool isValid();
    string getState();
    string getString();
    ComponentPattern* getBond();

public:
    ComponentClass* type;
    MoleculePattern* molecule;
    bool valid;
    string name;
    string state;
    string bondName;
    string bondPattern;
    ComponentPattern* bond;

    friend class ComplexPattern;
    friend class MoleculePattern;
};

class MoleculePattern : public Vertex
{
public:
    MoleculePattern();
    MoleculePattern(string definition, map<string,MoleculeClass*> moleculeClasses);
    bool isValid();
    bool isNull();
    string getName();
    int getNumberComponents();
    ComponentPattern* getComponent(int i);
    bool matches(MoleculePattern* instance);
    bool matches(MoleculeInstance* instance);

public:
    virtual int getMaxNumberEdges();
    virtual Vertex* getEdge(int i);
    virtual void addEdge(int sourceIndex, Vertex* dest, int destIndex);
    virtual void removeEdge(int i);
    virtual bool matches(Vertex* v2);
    virtual string getString();

protected:
    MoleculeClass* type;
    bool valid;
    bool null;
    string name;
    vector<ComponentPattern*> components;

    friend class ComplexPattern;
};

class ComplexPattern : public Graph
{
public:
    ComplexPattern();
    ComplexPattern(string definition, map<string,MoleculeClass*> moleculeClasses);
    bool isValid();
    bool matchesTo(ComplexInstance instance);

public:
    virtual int getNumberVertices();
    virtual Vertex* getVertex(int i);
    virtual string getString();

protected:
    bool valid;
    vector<MoleculePattern*> molecules;
};

class ReactantPattern : public Graph
{
public:
    ReactantPattern();
    ReactantPattern(string definition, map<string,MoleculeClass*> moleculeClasses);
    bool isValid();
    int getNumberReactants();
    ComplexPattern* getReactant(int index);

public:
    virtual int getNumberVertices();
    virtual Vertex* getVertex(int i);
    virtual string getString();

protected:
    bool valid;
    vector<ComplexPattern*> reactants;
};

class ReactionPattern
{
public:
    ReactionPattern();
    ReactionPattern(string lhs, string rhs, double rate, map<string,MoleculeClass*> moleculeClasses);
    bool isValid();
    string getString(bool includeRate=false);
    ReactantPattern* getSubstrates();
    ReactantPattern* getProducts();
    double getRate();
    GraphMapping getSubstrateToProductMapping();

protected:
    bool valid;
    ReactantPattern* substrates;
    ReactantPattern* products;
    double rate;
    GraphMapping substrateToProductMapping;
};


}
}

#endif // MOLECULE_H
