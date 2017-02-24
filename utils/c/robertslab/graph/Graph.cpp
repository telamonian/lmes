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
#include <sstream>

#include "robertslab/graph/Graph.h"

using std::list;
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
    mapping[sourceVertex] = targetVertex;
}

bool GraphMapping::containsSourceVertex(Vertex* sourceVertex)
{
    return (mapping.count(sourceVertex) > 0);
}

int GraphMapping::getNumberMatches()
{
    return mapping.size();
}

Vertex* GraphMapping::getSourceVertex(int index)
{
    return sourceVertices[index];
}

Vertex* GraphMapping::getTargetVertex(int index)
{
    return targetVertices[index];
}

string GraphMapping::getString(bool includeGraphs)
{
    if (sourceGraph == NULL || targetGraph == NULL) return "NULL";

    std::stringstream ss;
    for (int i=0; i<mapping.size(); i++)
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

    mapping.clear();
    for (int i=0; i<sourceVertices.size(); i++)
        mapping[sourceVertices[i]] = targetVertices[i];
}

void Vertex::setMark(string mark)
{
    marks.insert(mark);
}

bool Vertex::hasMark(string mark)
{
    return (marks.count(mark) == 1);
}

void Vertex::clearMark(string mark)
{
    marks.erase(mark);
}

void Vertex::clearAllMarks()
{
    marks.clear();
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

bool Graph::isIsomorphicSubgraph(Vertex* sourceVertex, Graph* target, Vertex* targetVertex, GraphMapping* mapping, bool firstVertex)
{
    //printf("Checking for isomorphic subgraph %s and %s, vertices %s and %s\n",getString().c_str(), target->getString().c_str(), sourceVertex->getString().c_str(), targetVertex->getString().c_str()); fflush(stdout);

    // If this is the first call, clear the marks for graphs.
    if (firstVertex) target->clearMark("isIsomorphicSubgraph");

    // Mark that we have checked this vertex.
    targetVertex->setMark("isIsomorphicSubgraph");

    // If the vertices don't match, return false.
    if (!sourceVertex->matches(targetVertex)) return false;

    // Follow each child edge.
    for (int i=0; i<targetVertex->getMaxNumberEdges(); i++)
    {
        if (targetVertex->getEdge(i) != NULL && !targetVertex->getEdge(i)->hasMark("isIsomorphicSubgraph"))
        {
            // If the source is missing a link, return false.
            if (sourceVertex->getEdge(i) == NULL) return false;

            // Recursively follow any edges.
            if (!isIsomorphicSubgraph(sourceVertex->getEdge(i), target, targetVertex->getEdge(i), mapping, false)) return false;
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
            // TODO: check to ensure that this mapping is not a duplciate of a previous mapping.

            ret.push_back(mapping);
        }
    }

    printf("Found all isomorphic subgraphs between %s and %s: %ld\n",getString().c_str(),subgraph->getString().c_str(), ret.size()); fflush(stdout);

    return ret;
}


}
}

/*
 *


bool ComplexPattern::doesSubgraphMatch()
{
    // Create a match and associate the anchor molecule in the pattern and the instance.
    ComplexPatternMatch match;
    match.addMoleculeMatch(patternAnchorMolecule,instanceAnchorMolecule);

    // Track which vertices in the pattern we have processed.
    set<int> processedPatternVertices;

    // Lop until we have processed the whole connected subgraph.
    while(true)
    {
        bool foundNewVertices = false;

        // Go through each new pattern vertex in the match.
        vector<int> patternMatches = match.getPatternMolecules();
        for (auto it=patternMatches.begin(); it != patternMatches.end(); it++)
        {
            int patternVertex = *it;
            if (processedPatternVertices.count(patternVertex) == 0)
            {
                // Mark the we had at least one new vertex.
                foundNewVertices = true;

                // See if this vertex pair matches.
                if (doesVertexMatch(match, pattern, patternVertex, instance, match.getPatternMoleculeMatch(patternVertex)))
                {
                    processedPatternVertices.insert(patternVertex);
                }
                else
                {
                    return false;
                }
            }
        }

        // If we didn't find any new vertices, we are done.
        if (!foundNewVertices) break;
    }

    return true;
}

bool ComplexPattern::doesVertexMatch(ComplexPatternMatch& match, ComplexPattern pattern, int patternVertex, ComplexInstance instance, int instanceVertex)
{
    // Go through each edge in the pattern.
    for (int i=0; i<pattern.bonds.size(); i++)
    {
        // See if the edges connects to the pattern vertex.
        BondPattern patternEdge = pattern.bonds[i];
        if (patternEdge.m1 == patternVertex || patternEdge.m2 == patternVertex)
        {
            // Go through each edge in the instance.
            for (int j=0; j<instance.bonds.size(); j++)
            {
                // See if the edge connects to the instance vertex.
                BondInstance instanceEdge = pattern.bonds[i];
                if (instanceEdge.m1 == instanceVertex || instanceEdge.m2 == instanceVertex)
                {
                    doesEdgeMatch(match,pattern,i,instance,j);
                }
            }
        }
    }
}

bool ComplexPattern::doesEdgeMatch(ComplexPatternMatch& match, ComplexPattern pattern, int patternBondIndex, ComplexInstance instance, int instanceBondIndex)
{
    BondPattern patternBond = pattern.bonds[paternBondIndex];
    BondInstance instanceBond = instance.bonds[instanceBondIndex];

    // Whether the first endpoint in the bond matches.
    bool firstEndpointMatches = false;

    // See if we already have a known match for the first molecule in the bond.
    if (match.containsPatternMolecule(patternBond.m1))
    {
        // See if the match corresponds to the first molecule in the instance bond and the components match.
        if (match.getPatternMoleculeMatch(patternBond.m1) == instanceBond.m1 && patternBond.c1 == instanceBond.c1)
        {
            firstEndpointMatches = true;
        }
    }
    else
    {
        // See if the molecule in the instance bond matches and the components match.
        if (pattern.molecules[patternBond.m1].matchesTo(instance.molecules[instanceBond.m1])  && patternBond.c1 == instanceBond.c1)
        {
            firstEndpointMatches = true;
        }
    }

    // Whether the second endpoint in the bond matches.
    bool secondEndpointMatches = false;

    // See if we already have a known match for the second molecule in the bond.
    if (match.containsPatternMolecule(patternBond.m2))
    {
        // See if the match corresponds to the second molecule in the instance bond and the components match.
        if (match.getPatternMoleculeMatch(patternBond.m2) == instanceBond.m2 && patternBond.c2 == instanceBond.c2)
        {
            firstEndpointMatches = true;
        }
    }
    else
    {
        // See if the molecule in the instance bond matches and the components match.
        if (pattern.molecules[patternBond.m2].matchesTo(instance.molecules[instanceBond.m2])  && patternBond.c2 == instanceBond.c2)
        {
            secondEndpointMatches = true;
        }
    }

    return (firstEndpointMatches && secondEndpointMatches);
}

*/
