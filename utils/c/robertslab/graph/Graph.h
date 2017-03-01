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
    void appendMappings(const GraphMapping& additionalMapping);
    void addMapping(Vertex* sourceVertex, Vertex* targetVertex);
    bool containsSourceVertex(Vertex* sourceVertex);
    int getNumberMatches();
    Vertex* getSourceVertex(int index);
    Vertex* getTargetVertex(int index);
    Vertex* getSourceVertex(Vertex* targetVertex);
    Vertex* getTargetVertex(Vertex* sourceVertex);
    void reverse();
    string getString(bool includeGraphs=false);

private:
    Graph* sourceGraph;
    Graph* targetGraph;
    vector<Vertex*> sourceVertices;
    vector<Vertex*> targetVertices;
    map<Vertex*,Vertex*> forwardMapping;
    map<Vertex*,Vertex*> reverseMapping;
};

class Vertex
{
public:
    Vertex();

public:
    virtual int getMaxNumberEdges()=0;
    virtual Vertex* getEdge(int i)=0;
    virtual void addEdge(int sourceIndex, Vertex* dest, int destIndex)=0;
    virtual void removeEdge(int i)=0;
    virtual bool matches(Vertex* v2)=0;
    virtual string getString()=0;

public:
    virtual void setMark(string mark);
    virtual void setMarkOnConnected(string mark);
    virtual bool hasMark(string mark);
    virtual void clearMark(string mark);
    virtual void clearMarkOnConnected(string mark, string clearedMark="");
    virtual void clearAllMarks();
    virtual set<Vertex*> getConnectedVertices(bool root=true);
    virtual int findEdgeLeadingTo(Vertex* destination);

protected:
    set<string> marks;
    int nextClearedMark;
};

class Graph
{
public:
    virtual int getNumberVertices()=0;
    virtual Vertex* getVertex(int i)=0;
    virtual string getString()=0;

public:
    virtual bool addEdge(Vertex* v1, int index1, Vertex* v2, int index2);
    virtual bool removeEdge(Vertex* v1, Vertex* v2);
    virtual void clearMark(string mark);
    virtual void clearAllMarks();

    virtual bool isIsomorphic(Graph* target);
    bool isIsomorphicFromVertices(Graph* target, Vertex* sourceVertex, Vertex* targetVertex, bool permitSubgraph=false, GraphMapping* mapping=NULL, Vertex* sourceVertexOrigin=NULL, Vertex* targetVertexOrigin=NULL);

    virtual list<Vertex*> findConnectedSubgraphs();

    virtual GraphMapping findGraphMapping(Graph* target);
    virtual list<GraphMapping> findCommonSubgraphs(Graph* target);
    GraphMapping findLargestCommonSubgraph(Graph* target, string ignoreMark="");

    virtual list<GraphMapping> findAllIsomorphicSubgraphs(Graph* subgraph);
};



}
}

#endif // ROBERTSLAB_GRAPH_GRAPH_H
