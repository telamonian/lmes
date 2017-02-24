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

#ifndef ROBERTSLAB_GRAPH_GRAPH_H
#define ROBERTSLAB_GRAPH_GRAPH_H

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

using std::list;
using std::map;
using std::set;
using std::string;
using std::vector;

namespace robertslab {
namespace graph {

class Graph;
class Vertex;

class GraphMapping
{
public:
    GraphMapping();
    GraphMapping(Graph* sourceGraph, Graph* targetGraph);
    void setGraphs(Graph* sourceGraph, Graph* targetGraph);
    void addMapping(Vertex* sourceVertex, Vertex* targetVertex);
    bool containsSourceVertex(Vertex* sourceVertex);
    int getNumberMatches();
    Vertex* getSourceVertex(int index);
    Vertex* getTargetVertex(int index);
    void reverse();
    string getString(bool includeGraphs=false);

private:
    Graph* sourceGraph;
    Graph* targetGraph;
    vector<Vertex*> sourceVertices;
    vector<Vertex*> targetVertices;
    map<Vertex*,Vertex*> mapping;
};

class Vertex
{
public:
    virtual int getMaxNumberEdges()=0;
    virtual Vertex* getEdge(int i)=0;
    virtual bool matches(Vertex* v2)=0;
    virtual string getString()=0;

public:
    virtual void setMark(string mark);
    virtual bool hasMark(string mark);
    virtual void clearMark(string mark);
    virtual void clearAllMarks();

protected:
    set<string> marks;
};

class Graph
{
public:
    virtual int getNumberVertices()=0;
    virtual Vertex* getVertex(int i)=0;
    virtual string getString()=0;

public:
    virtual void clearMark(string mark);
    virtual void clearAllMarks();

    virtual GraphMapping findGraphMapping(Graph* target);
    virtual list<GraphMapping> findCommonSubgraphs(Graph* target);
    GraphMapping findLargestCommonSubgraph(Graph* target, string ignoreMark="");
    bool isIsomorphicSubgraph(Vertex* sourceVertex, Graph* target, Vertex* targetVertex, GraphMapping* mapping, bool firstVertex=true);

    virtual list<GraphMapping> findAllIsomorphicSubgraphs(Graph* subgraph);
};



}
}

#endif // ROBERTSLAB_GRAPH_GRAPH_H
