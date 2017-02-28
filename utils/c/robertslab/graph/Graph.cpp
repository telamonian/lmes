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

#include <list>
#include <set>
#include <sstream>

#include "robertslab/graph/Graph.h"

using std::list;
using std::set;
using std::string;

namespace robertslab {
namespace graph {

GraphMapping::GraphMapping()
{
}

GraphMapping::GraphMapping(Graph* sourceGraph, Graph* targetGraph)
:sourceGraph(sourceGraph),targetGraph(targetGraph)
{
}

void GraphMapping::setGraphs(Graph* sourceGraph, Graph* targetGraph)
{
    this->sourceGraph = sourceGraph;
    this->targetGraph = targetGraph;
}

void GraphMapping::addMapping(Vertex* sourceVertex, Vertex* targetVertex)
{
    sourceVertices.push_back(sourceVertex);
    targetVertices.push_back(targetVertex);
    forwardMapping[sourceVertex] = targetVertex;
    reverseMapping[targetVertex] = sourceVertex;
}

bool GraphMapping::containsSourceVertex(Vertex* sourceVertex)
{
    return (forwardMapping.count(sourceVertex) > 0);
}

int GraphMapping::getNumberMatches()
{
    return sourceVertices.size();
}

Vertex* GraphMapping::getSourceVertex(int index)
{
    return sourceVertices[index];
}

Vertex* GraphMapping::getTargetVertex(int index)
{
    return targetVertices[index];
}

Vertex* GraphMapping::getSourceVertex(Vertex* targetVertex)
{
    return reverseMapping[targetVertex];
}

Vertex* GraphMapping::getTargetVertex(Vertex* sourceVertex)
{
    return forwardMapping[sourceVertex];
}


string GraphMapping::getString(bool includeGraphs)
{
    if (sourceGraph == NULL || targetGraph == NULL) return "NULL";

    std::stringstream ss;
    for (int i=0; i<sourceVertices.size(); i++)
    {
        if (i > 0) ss << "\n";
        if (includeGraphs) ss << "<" << sourceGraph->getString() << "> ";
        ss << sourceVertices[i]->getString().c_str();
        ss << " -> ";
        if (includeGraphs) ss << "<" << targetGraph->getString() << "> ";
        ss << targetVertices[i]->getString().c_str();
    }
    return ss.str();
}

void GraphMapping::reverse()
{
    Graph* tmp1 = sourceGraph;
    sourceGraph = targetGraph;
    targetGraph = tmp1;

    vector<Vertex*> tmp2 = sourceVertices;
    sourceVertices = targetVertices;
    targetVertices = tmp2;

    forwardMapping.clear();
    reverseMapping.clear();
    for (int i=0; i<sourceVertices.size(); i++)
    {
        forwardMapping[sourceVertices[i]] = targetVertices[i];
        reverseMapping[targetVertices[i]] = sourceVertices[i];
    }
}

Vertex::Vertex()
:nextClearedMark(0)
{

}

void Vertex::setMark(string mark)
{
    marks.insert(mark);
}

void Vertex::setMarkOnConnected(string mark)
{
    if (marks.count(mark) == 0)
    {
        marks.insert(mark);
        for (int i=0; i<getMaxNumberEdges(); i++)
        {
            Vertex* v = getEdge(i);
            if (v != NULL) v->setMarkOnConnected(mark);
        }
    }
}

bool Vertex::hasMark(string mark)
{
    return (marks.count(mark) == 1);
}

void Vertex::clearMark(string mark)
{
    marks.erase(mark);
}

void Vertex::clearMarkOnConnected(string mark, string clearedMark)
{
    // If this is the root, generate a unqie mark to use for marking vertices that have been cleared.
    if (clearedMark == "") clearedMark = "clearMarkOnConnected"+std::to_string((unsigned long long)this)+std::to_string(nextClearedMark++);

    // If this vertex hasn't been cleared yet.
    if (marks.count(clearedMark) == 0)
    {
        // Clear the mark and mark that this vertex has been cleared.
        marks.erase(mark);
        marks.insert(clearedMark);

        // Clear any connected vertices.
        for (int i=0; i<getMaxNumberEdges(); i++)
        {
            Vertex* v = getEdge(i);
            if (v != NULL) v->clearMarkOnConnected(mark, clearedMark);
        }
    }
}

void Vertex::clearAllMarks()
{
    marks.clear();
}

set<Vertex*> Vertex::getConnectedVertices(bool root)
{
    if (root) clearMarkOnConnected("getConnectedVertices");

    // Add this vertex to the set.
    set<Vertex*> ret;
    ret.insert(this);
    setMark("getConnectedVertices");

    // Add any vertices from the edges.
    for (int i=0; i<getMaxNumberEdges(); i++)
    {
        Vertex* v2 = getEdge(i);
        if (v2 != NULL && !v2->hasMark("getConnectedVertices"))
        {
            set<Vertex*> ret2=getEdge(i)->getConnectedVertices(false);
            ret.insert(ret2.begin(), ret2.end());
        }
    }
    return ret;
}

int Vertex::findEdgeLeadingTo(Vertex* destination)
{
    for (int i=0; i<getMaxNumberEdges(); i++)
        if (getEdge(i) == destination)
            return i;
    return -1;
}

bool Graph::removeEdge(Vertex* v1, Vertex* v2)
{
    // Make sure we can find both vertices.
    int vertexCount=0;
    for (int i=0; i<getNumberVertices(); i++)
    {
        if (getVertex(i) == v1) vertexCount++;
        if (getVertex(i) == v2) vertexCount++;
    }
    if (vertexCount != 2) return false;

    // Make sure the vertices are linked by an edge.
    int l1=-1;
    for (int i=0; i<v1->getMaxNumberEdges(); i++)
    {
        if (v1->getEdge(i) == v2)
        {
            l1=i;
            break;
        }
    }
    int l2=-1;
    for (int i=0; i<v2->getMaxNumberEdges(); i++)
    {
        if (v2->getEdge(i) == v1)
        {
            l2=i;
            break;
        }
    }
    if (l1 == -1 || l2 == -1) return false;

    // Remove the edges.
    v1->removeEdge(l1);
    v2->removeEdge(l2);
    return true;
}

void Graph::clearMark(string mark)
{
    for (int i=0; i<getNumberVertices(); i++)
        getVertex(i)->clearMark(mark);
}

void Graph::clearAllMarks()
{
    for (int i=0; i<getNumberVertices(); i++)
        getVertex(i)->clearAllMarks();
}

list<Vertex*> Graph::findConnectedSubgraphs()
{
    // Clear the mark from the whole graph.
    clearMark("findConnectedSubgraphs");

    // Go through each vertex.
    list<Vertex*> anchors;
    for (int i=0; i<getNumberVertices(); i++)
    {
        // If the vertex is not marked.
        Vertex* v = getVertex(i);
        if (!v->hasMark("findConnectedSubgraphs"))
        {
            // Add it to the list.
            anchors.push_back(v);

            // Mark it and anything connected to it.
            v->setMarkOnConnected("findConnectedSubgraphs");
        }
    }

    return anchors;
}

GraphMapping Graph::findGraphMapping(Graph* target)
{
    // Find all of the subgraphs in common.
    list<GraphMapping> componentSubgraphs = findCommonSubgraphs(target);

    // Create a new mapping that is the union of all the common subgraphs.
    GraphMapping mapping(this, target);
    for (auto it=componentSubgraphs.begin(); it != componentSubgraphs.end(); it++)
    {
        GraphMapping subgraphMapping = *it;
        for (int i=0; i<subgraphMapping.getNumberMatches(); i++)
            mapping.addMapping(subgraphMapping.getSourceVertex(i), subgraphMapping.getTargetVertex(i));
    }

    return mapping;
}

list<GraphMapping> Graph::findCommonSubgraphs(Graph* target)
{
    list<GraphMapping> ret;

    // Clear any marks on the graphs.
    clearMark("findCommonSubgraphs");
    target->clearMark("findCommonSubgraphs");

    // Loop until we can't find any more subgraphs in common.
    while (true)
    {
        GraphMapping nextCommonSubgraph = findLargestCommonSubgraph(target, "findCommonSubgraphs");

        // If the subgraph was empty, we are done.
        if (nextCommonSubgraph.getNumberMatches() == 0) break;

        // Mark the vertices as in use.
        for (int i=0; i<nextCommonSubgraph.getNumberMatches(); i++)
        {
            nextCommonSubgraph.getSourceVertex(i)->setMark("findCommonSubgraphs");
            nextCommonSubgraph.getTargetVertex(i)->setMark("findCommonSubgraphs");
        }

        // Add the subgraph to the list.
        ret.push_back(nextCommonSubgraph);
    }

    return ret;
}

GraphMapping Graph::findLargestCommonSubgraph(Graph* target, string ignoreMark)
{
    GraphMapping maxSubgraph;

    //printf("Finding largest common subgraph between [%s] and [%s]\n",getString().c_str(),target->getString().c_str()); fflush(stdout);

    // Go through all of the vertices in the source.
    for (int i=0; i<getNumberVertices(); i++)
    {
        // See if we should ignore this vertex.
        if (ignoreMark != "" && getVertex(i)->hasMark(ignoreMark)) continue;

        // Go through all of the vertices in the target.
        for (int j=0; j<getNumberVertices(); j++)
        {
            // See if we should ignore this vertex.
            if (ignoreMark != "" && target->getVertex(j)->hasMark(ignoreMark)) continue;

            //printf("Finding largest common subgraph between [%s] anchor=[%s] and [%s] anchor=[%s]\n",getString().c_str(),getVertex(i)->getString().c_str(),target->getString().c_str(),target->getVertex(j)->getString().c_str()); fflush(stdout);

            // Check for a mapping between these two vertices.
            GraphMapping subgraph(this, target);
            isIsomorphicSubgraph(getVertex(i), target, target->getVertex(j), &subgraph);
            if (subgraph.getNumberMatches() > maxSubgraph.getNumberMatches())
                maxSubgraph = subgraph;

            // Check for a mapping between these two vertices.
            GraphMapping reverseSubgraph(this, target);
            target->isIsomorphicSubgraph(target->getVertex(j), this, getVertex(i), &reverseSubgraph);
            if (reverseSubgraph.getNumberMatches() > maxSubgraph.getNumberMatches())
            {
                reverseSubgraph.reverse();
                maxSubgraph = reverseSubgraph;
            }
        }
    }

    //printf("Largest common subgraph was:\n%s\n", maxSubgraph.getString().c_str());

    return maxSubgraph;
}

bool Graph::isIsomorphicSubgraph(Vertex* sourceVertex, Graph* target, Vertex* targetVertex, GraphMapping* mapping, Vertex* sourceVertexOrigin, Vertex* targetVertexOrigin)
{
    //printf("Checking for isomorphic subgraph %s and %s, vertices %s and %s\n",getString().c_str(), target->getString().c_str(), sourceVertex->getString().c_str(), targetVertex->getString().c_str()); fflush(stdout);

    // If this is the first call, clear the marks for graphs.
    bool isFirstVertex = (sourceVertexOrigin == NULL && targetVertexOrigin == NULL);
    if (isFirstVertex) target->clearMark("isIsomorphicSubgraph");

    // Mark that we have visited this vertex.
    targetVertex->setMark("isIsomorphicSubgraph");

    // If the vertices don't match, return false.
    if (!sourceVertex->matches(targetVertex)) return false;

    // Make sure that the same edges take us back to the vertices we came from.
    if (!isFirstVertex && sourceVertex->findEdgeLeadingTo(sourceVertexOrigin) != targetVertex->findEdgeLeadingTo(targetVertexOrigin)) return false;

    // Follow each child edge.
    for (int i=0; i<targetVertex->getMaxNumberEdges(); i++)
    {
        if (targetVertex->getEdge(i) != NULL && !targetVertex->getEdge(i)->hasMark("isIsomorphicSubgraph"))
        {
            // If the source is missing a link, return false.
            if (sourceVertex->getEdge(i) == NULL) return false;

            // Make sure that the

            // Recursively follow any edges.
            if (!isIsomorphicSubgraph(sourceVertex->getEdge(i), target, targetVertex->getEdge(i), mapping, sourceVertex, targetVertex)) return false;
        }
    }

    //printf("    Yes, is an isomorphic subgraph %X and %X\n",sourceVertex,targetVertex); fflush(stdout);
    mapping->addMapping(sourceVertex, targetVertex);
    return true;
}

list<GraphMapping> Graph::findAllIsomorphicSubgraphs(Graph* subgraph)
{
    list<GraphMapping> ret;

    // Go through all of the vertices in the graph and search for a match to the first vertex in the target.
    for (int i=0; i<getNumberVertices(); i++)
    {
        GraphMapping mapping(this, subgraph);

        // See if the vertices match.
        if (isIsomorphicSubgraph(getVertex(i), subgraph, subgraph->getVertex(0), &mapping))
        {
            // TODO: check to ensure that this mapping is not a duplicate of a previous mapping.

            ret.push_back(mapping);
        }
    }

    //printf("Found all isomorphic subgraphs between %s and %s: %ld\n",getString().c_str(),subgraph->getString().c_str(), ret.size()); fflush(stdout);

    return ret;
}


}
}

