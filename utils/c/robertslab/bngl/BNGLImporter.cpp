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

#include <fstream>
#include <iostream>
#include <list>
#include <regex>

#include <sbml/math/ASTNode.h>
#include <sbml/math/L3Parser.h>

#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/me/PropensityFunction.h"
#include "robertslab/sbml/ASTHelper.h"
#include "robertslab/bngl/BNGLImporter.h"
#include "robertslab/bngl/InstanceDefinitions.h"
#include "robertslab/bngl/PatternDefinitions.h"
#include "robertslab/bngl/TypeDefinitions.h"
#include "robertslab/graph/Graph.h"

using std::list;
using std::regex;
using std::regex_token_iterator;
using lm::Exception;
using lm::Print;

using robertslab::sbml::ASTHelper;
using robertslab::graph::GraphMapping;

namespace robertslab {
namespace bngl {

BNGLImporter::BNGLImporter()
:propensityFunctions(NULL),constantsUseConcentrations(false),verbose(false),reallyVerbose(false),allImportStepsSuccessful(true),numberSpecies(0)//,numberReactions(0),S(NULL),T(NULL),K(NULL),D(NULL)
{
    propensityFunctions = new lm::me::PropensityFunctionFactory();
}

BNGLImporter::~BNGLImporter()
{
    if (propensityFunctions != NULL) delete propensityFunctions; propensityFunctions = NULL;
    /*if (S != NULL) delete S; S = NULL;
    if (T != NULL) delete T; T = NULL;
    if (K != NULL) delete K; K = NULL;
    if (D != NULL) delete D; D = NULL;*/
}

void BNGLImporter::setOptions(bool constantsUseConcentrations, bool verbose, bool reallyVerbose)
{
    this->constantsUseConcentrations = constantsUseConcentrations;
    this->verbose = verbose;
    this->reallyVerbose = reallyVerbose;
}

bool BNGLImporter::import(string filename, map<string,double> userParameters)
{
    Print::printf(Print::INFO, "Processing BNGL document: %s", filename.c_str());

    // Initialize the parameters with any user specified parameters.
    for (map<string,double>::iterator it=userParameters.begin(); it!=userParameters.end(); it++)
    {
        if (parameters.count(it->first) == 0)
        {
            parameters[it->first] = it->second;
            Print::printf(Print::INFO, "Added user defined parameter %s: %e", it->first.c_str(), parameters[it->first]);
        }
    }

    // Open the file.
    std::ifstream input(filename, std::ifstream::in);

    regex beginPattern("^begin\\s+(.+)$");
    regex endPattern("^end\\s+(.+)$");
    std::smatch match;
    string section = "";
    list<string> sectionLines;
    int lineNumber=0;
    string continuedLine = "";
    while (!input.eof())
    {
        lineNumber++;
        string line;
        std::getline(input,line);

        // If the line end with a backslah, it is continued on the next line.
        if (line.back() == '\\')
        {
            continuedLine += line.substr(0,line.size()-1);
            continue;
        }
        else if (continuedLine != "")
        {
            line = continuedLine + line;
            continuedLine = "";
        }

        // Strip any comments.
        if (line.find_first_of('#') != string::npos) line = line.substr(0,line.find_first_of('#'));

        // Skip the line if it is blank.
        if (line.find_first_not_of(" \t\r\n") == string::npos) continue;

        // Strip any leading or trailing whitespace.
        size_t start = line.find_first_not_of(" \t\r\n");
        size_t end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start,end-start+1);

        // Strip any leading line numbers.
        regex lineNumberPattern("^(?:\\d+\\s+)?(\\S.*)$");
        if (std::regex_match(line, match, lineNumberPattern) && match.size() == 2)
            line = match[1].str();

        // Strip any leading line labels.
        regex lineLabelPattern("^(?:\\S+\\:\\s+)?(\\S.*)$");
        if (std::regex_match(line, match, lineLabelPattern) && match.size() == 2)
            line = match[1].str();

        // Check for section blocks.
        if (std::regex_match(line, match, beginPattern) && match.size() == 2 && match[1].str() != "model")
        {
            if (section == "")
            {
                section = match[1].str();
                sectionLines.clear();
            }
            else
            {
                Print::printf(Print::ERROR, "invalid begin section on line %d: %s",lineNumber,line.c_str());
                allImportStepsSuccessful = false;
            }
        }
        else if (std::regex_match(line, match, endPattern) && match.size() == 2 && match[1].str() != "model")
        {
            if (section == match[1].str())
            {
                // Process the section.
                if (section == "parameters")
                {
                    parseParameters(sectionLines);
                }
                else if (section == "molecule types")
                {
                    parseMoleculeTypes(sectionLines);
                }
                else if (section == "species" || section == "seed species" )
                {
                    parseSeedSpecies(sectionLines);
                }
                else if (section == "reaction rules")
                {
                    // Add any molecules that are in the species list but not in the molecule types before parsing the reactions.
                    supplementMoleculeTypesFromSeedSpecies();

                    // Parse the reactions.
                    parseReactions(sectionLines);
                }
                else
                {
                    Print::printf(Print::INFO, "Ignoring unsupported block type: %s", section.c_str());
                }

                // Reset the section.
                section = "";
            }
            else
            {
                Print::printf(Print::ERROR, "invalid end section on line %d: %s",lineNumber,line.c_str());
                allImportStepsSuccessful = false;
            }
        }
        else if (section != "" && line != "")
        {
            sectionLines.push_back(line);
        }

    }

    // If we imported the data correctly, process it.
    if (allImportStepsSuccessful)
    {
        Print::printf(Print::INFO, "Processing BNGL model: %s", filename.c_str());
        processModel();
    }

    return allImportStepsSuccessful;
}

lm::input::ReactionModel* BNGLImporter::getReactionModel()
{
    return &reactionModel;
}

string BNGLImporter::getDescription()
{
    return "BNGL Importer";
}

void BNGLImporter::parseParameters(list<string>& lines)
{
    Print::printf(Print::INFO, "Parsing parameters block.");

    regex parameterPattern("^(\\S+)\\s+(\\S+)$");
    std::smatch match;
    for (list<string>::iterator it=lines.begin(); it != lines.end(); it++)
    {
        string line = *it;
        if (std::regex_match(line, match, parameterPattern) && match.size() == 3)
        {
            // Get the key and the expression.
            string key = match[1].str();
            string expression = match[2].str();

            // If we got to a numeric expression, save it.
            double value;
            if (evaluteExpression(expression, value))
            {
                if (parameters.count(key) == 0)
                {
                    parameters[key] = value;
                    Print::printf(Print::INFO, "Added parameter %s: %e", match[1].str().c_str(), value);
                }
                else
                {
                    Print::printf(Print::INFO, "Skipped duplicate parameter %s: %e", key.c_str(), value);
                }
            }
            else
            {
                Print::printf(Print::ERROR, "Could not simplify parameter %s: %s", key.c_str(), expression.c_str());
                allImportStepsSuccessful = false;
            }
        }
        else
        {
            Print::printf(Print::WARNING, "Could not parse parameter from block: \"%s\"", line.c_str());
        }
    }
}

void BNGLImporter::parseMoleculeTypes(list<string>& lines)
{
    Print::printf(Print::INFO, "Parsing molecule types block.");

    for (list<string>::iterator it=lines.begin(); it != lines.end(); it++)
    {
        string line = *it;
        MoleculeClass* molecule = new MoleculeClass(line);
        if (molecule->isValid())
        {
            moleculeTypes[molecule->getName()] = molecule;
            Print::printf(Print::INFO, "Added molecule definition: %s", molecule->getString().c_str());
        }
        else
        {
            Print::printf(Print::WARNING, "Could not parse molecule from block: \"%s\"", line.c_str());
        }
    }
}

void BNGLImporter::parseSeedSpecies(list<string>& lines)
{
    Print::printf(Print::INFO, "Parsing species block.");

    // Add the round to the complex species list.
    complexSpecies.push_back(vector<ComplexInstance*>());

    regex parameterPattern("^(\\S+)\\s+(\\S+)$");
    std::smatch match;
    for (list<string>::iterator it=lines.begin(); it != lines.end(); it++)
    {
        string line = *it;
        if (std::regex_match(line, match, parameterPattern) && match.size() == 3)
        {
            // Get the count.
            string countExpression = match[2].str();
            double count=0.0;
            bool isValidCount = evaluteExpression(countExpression,count);

            // Get the complex.
            string complexInstanceString = match[1].str();
            ComplexInstance* complex = new ComplexInstance(complexInstanceString, count);

            // Make sure we processed a valid record.
            if (complex->isValid() && isValidCount)
            {
                complexSpecies[0].push_back(complex);
                Print::printf(Print::INFO, "Added initial count %s", complex->getString(true).c_str());
            }
            else if (!complex->isValid())
            {
                Print::printf(Print::ERROR, "Could not simplify complex %s", complexInstanceString.c_str());
                allImportStepsSuccessful = false;
            }
            else if (!isValidCount)
            {
                Print::printf(Print::ERROR, "Could not simplify initial count: \"%s\"", countExpression.c_str());
                allImportStepsSuccessful = false;
            }
        }
        else
        {
            Print::printf(Print::WARNING, "Could not parse initial count from block: \"%s\"", line.c_str());
        }
    }
}

void BNGLImporter::parseReactions(list<string>& lines)
{
    Print::printf(Print::INFO, "Parsing reactions block.");
    regex reversibleReactionPattern("^([^<->]+)\\s*<->\\s*([^<->]+)\\s+(\\S+),\\s+(\\S+)$");
    regex irreversibleReactionPattern("^([^<->]+)\\s*->\\s*([^<->]+)\\s+(\\S+)$");
    std::smatch match;
    for (list<string>::iterator it=lines.begin(); it != lines.end(); it++)
    {
        string line = *it;
        if (std::regex_match(line, match, reversibleReactionPattern) && match.size() == 5)
        {
            // Get the reaction rates.
            string rateFString = match[3].str();
            double rateF=0.0;
            bool isValidRateF = evaluteExpression(rateFString,rateF);
            string rateRString = match[4].str();
            double rateR=0.0;
            bool isValidRateR = evaluteExpression(rateRString,rateR);

            // Get the lhs and rhs of the equation.
            string lhsString = match[1].str();
            string rhsString = match[2].str();
            ReactionPattern* reactionF = new ReactionPattern(lhsString, rhsString, rateF, moleculeTypes);
            ReactionPattern* reactionR = new ReactionPattern(rhsString, lhsString, rateR, moleculeTypes);

            // Make sure we processed a valid record.
            if (isValidRateF && isValidRateR && reactionF->isValid() && reactionR->isValid())
            {
                reactions.push_back(reactionF);
                reactions.push_back(reactionR);
                Print::printf(Print::INFO, "Added forward reaction %s", reactionF->getString(true).c_str());
                if (verbose) Print::printf(Print::INFO, "Reaction substrate to product mapping:\n%s", reactionF->getSubstrateToProductMapping().getString().c_str());
                Print::printf(Print::INFO, "Added reverse reaction %s", reactionR->getString(true).c_str());
                if (verbose) Print::printf(Print::INFO, "Reaction substrate to product mapping:\n%s", reactionR->getSubstrateToProductMapping().getString().c_str());
            }
            else if (!reactionF->isValid() || !reactionR->isValid())
            {
                Print::printf(Print::ERROR, "Could not simplify reaction: \"%s\" \"%s\"", lhsString.c_str(), rhsString.c_str());
                allImportStepsSuccessful = false;
            }
            else if (!isValidRateF)
            {
                Print::printf(Print::ERROR, "Could not simplify reaction rate: \"%s\"", rateFString.c_str());
                allImportStepsSuccessful = false;
            }
            else if (!isValidRateR)
            {
                Print::printf(Print::ERROR, "Could not simplify reaction rate: \"%s\"", rateRString.c_str());
                allImportStepsSuccessful = false;
            }
        }
        else if (std::regex_match(line, match, irreversibleReactionPattern) && match.size() == 4)
        {
            // Get the reaction rate.
            string rateString = match[3].str();
            double rate=0.0;
            bool isValidRate = evaluteExpression(rateString,rate);

            // Get the lhs and rhs of the equation.
            string lhsString = match[1].str();
            string rhsString = match[2].str();
            ReactionPattern* reaction = new ReactionPattern(lhsString, rhsString, rate, moleculeTypes);

            // Make sure we processed a valid record.
            if (reaction->isValid() && isValidRate)
            {
                reactions.push_back(reaction);
                Print::printf(Print::INFO, "Added irreversible reaction %s", reaction->getString(true).c_str());
                if (verbose) Print::printf(Print::INFO, "Reaction substrate to product mapping:\n%s", reaction->getSubstrateToProductMapping().getString().c_str());
            }
            else if (!reaction->isValid())
            {
                Print::printf(Print::ERROR, "Could not simplify left reaction: \"%s\" \"%s\"", lhsString.c_str(), rhsString.c_str());
                allImportStepsSuccessful = false;
            }
            else if (!isValidRate)
            {
                Print::printf(Print::ERROR, "Could not simplify reaction rate: \"%s\"", rateString.c_str());
                allImportStepsSuccessful = false;
            }
        }
        else
        {
            Print::printf(Print::WARNING, "Could not parse reaction from block: \"%s\"", line.c_str());
        }
    }
}

bool BNGLImporter::evaluteExpression(string expression, double& value)
{
    // Simplify the formula.
    ASTNode_t* formula = SBML_parseL3Formula(expression.c_str());
    ASTHelper::substituteASTParameters(formula, parameters);
    ASTHelper::simplifyASTExpression(formula);

    // If we are printing debug info, print the AST tree.
    if (reallyVerbose)
    {
        Print::printf(Print::INFO, "Simplified expression %s as:", expression.c_str());
        ASTHelper::printASTNode(formula);
    }

    // If we got to a numeric expression, return it.
    if (ASTHelper::isNumeric(formula))
    {
        value = ASTHelper::getNumericValue(formula);
        return true;
    }

    return false;
}

void BNGLImporter::processModel()
{
    // Figure out the list of atomic species that we need.
    //enumerateMoleculeSpecies();

    // Go through the reaction patterns iteratively until the species have converged.
    for (int round=1; round<10; round++)
    {
        if (verbose) Print::printf(Print::INFO, "Processing reactions for round %d", round);
        if (!processReactions(round++)) break;
    }

    // Combine the species from each round into one list.
    for (int i=0; i<complexSpecies.size(); i++)
        for (int j=0; j<complexSpecies[i].size(); j++)
            allComplexSpecies.push_back(complexSpecies[i][j]);
    Print::printf(Print::INFO, "Added %d total complex species.", allComplexSpecies.size());

    // Print debugging information, if necessary.
    if (verbose)
    {
        for (int i=0; i<allComplexSpecies.size(); i++)
        {
            Print::printf(Print::INFO, "%s", allComplexSpecies[i]->getString().c_str());
        }
    }
}

void BNGLImporter::supplementMoleculeTypesFromSeedSpecies()
{
    // Go through the list of complexes with initial counts.
    for (int i=0; i<complexSpecies.size(); i++)
    {
        ComplexInstance* complex = complexSpecies[0][i];

        // Go through the molecules in the complex.
        for (int j=0; j<complex->getNumberMolecules(); j++)
        {
            MoleculeInstance* molecule = complex->getMolecule(j);

            // If we don't already have a molecule type with this name, add one.
            if (moleculeTypes.count(molecule->getName()) == 0)
            {
                MoleculeClass* moleculeClass = new MoleculeClass(molecule->getString());
                moleculeTypes[moleculeClass->getName()] = moleculeClass;
                Print::printf(Print::WARNING, "Added inferred molecule definition: %s", moleculeClass->getString().c_str());
            }
        }
    }
}

void BNGLImporter::enumerateMoleculeSpecies()
{
    // We need one species for each combination of states for every molecule.
    for (auto it=moleculeTypes.begin(); it != moleculeTypes.end(); it++)
    {
        MoleculeClass* molecule = it->second;
        vector<string> stateCombinations = molecule->getStateCombinations();
        for (int j=0; j<stateCombinations.size(); j++)
        {
            moleculeSpecies.push_back(new MoleculeInstance(molecule->getName()+"("+stateCombinations[j]+")"));
        }
        Print::printf(Print::INFO, "Added species to represent possible states for molecule %s: %d species", molecule->getName().c_str(), stateCombinations.size());
    }
    Print::printf(Print::INFO, "Added %d total molecular species.", moleculeSpecies.size());

    // Print debugging information, if necessary.
    if (reallyVerbose)
    {
        for (int i=0; i<moleculeSpecies.size(); i++)
        {
            Print::printf(Print::INFO, "%s", moleculeSpecies[i]->getString().c_str());
        }
    }
}

bool BNGLImporter::processReactions(int round)
{
    // And the round to the complex species.
    if (complexSpecies.size() != round) throw Exception("inconsistent complex species list size",round,complexSpecies.size());
    complexSpecies.push_back(vector<ComplexInstance*>());

    // Loop through each reaction.
    for (int i=0; i<reactions.size(); i++)
    {
        ReactionPattern* reaction = reactions[i];
        processReaction(round, reaction);
    }

    Print::printf(Print::INFO, "Added complex species for round %d: %d species", round, complexSpecies[round].size());

    // Return whether or not we added any new complexes this round.
    return complexSpecies[round].size() != 0;
}

void BNGLImporter::processReaction(int round, ReactionPattern* reaction)
{
    // Process the reaction according to its order.
    if (reaction->getSubstrates()->getNumberReactants() == 0)
        processReactionZerothOrder(round, reaction);
    else if (reaction->getSubstrates()->getNumberReactants() == 1)
        processReactionFirstOrder(round, reaction);
    else if (reaction->getSubstrates()->getNumberReactants() == 2)
        processReactionSecondOrder(round, reaction);
    else
        throw Exception("unsupported reaction order",reaction->getSubstrates()->getNumberReactants());
}

void BNGLImporter::processReactionZerothOrder(int round, ReactionPattern* reaction)
{

}

void BNGLImporter::processReactionFirstOrder(int round, ReactionPattern* reaction)
{
    // Get the substrate pattern.
    ComplexPattern* substratePattern = reaction->getSubstrates()->getReactant(0);

    // Go through every complex species from the previous round and see if they match the pattern.
    for (int i=0; i<complexSpecies[round-1].size(); i++)
    {
        // Find all the matches.
        ComplexInstance* substrate = complexSpecies[round-1][i];
        list<GraphMapping> matches = substrate->findAllIsomorphicSubgraphs(substratePattern);

        // Go through each match.
        for (auto it=matches.begin(); it != matches.end(); it++)
        {
            GraphMapping substrateMapping = *it;
            if (verbose) Print::printf(Print::INFO, "Found match in round %d for reaction %s, species %s contains pattern %s",round, reaction->getString(false).c_str(), substrate->getString().c_str(), substratePattern->getString().c_str());

            // Rewrite the component states for the products.
            list<ComplexInstance*> products = rewriteSubstrateToProduct(substrate, substrateMapping, substratePattern, reaction->getSubstrateToProductMapping());


            // Create the product species.
            //if (products.size() == 1)
            //{
            //}


            // Go through each vertex in the mapping between substrates and products.


        }
    }
}

list<ComplexInstance*> BNGLImporter::rewriteSubstrateToProduct(ComplexInstance* substrate, GraphMapping substrateToSubstratePatternMapping, ComplexPattern* substratePattern, GraphMapping substratePatternToProductPatternMapping)
{
    // Create a copy of the substrate to rewrite into the products.
    ComplexInstance* products = new ComplexInstance(*substrate);

    // Create a copy of the mapping.
    GraphMapping substratePatternToProductMapping(substratePattern, products);
    for (int i=0; i<substrate->getNumberVertices(); i++)
    {
        if (substrateToSubstratePatternMapping.containsSourceVertex(substrate->getVertex(i)))
        {
            Vertex* v1 = substrateToSubstratePatternMapping.getTargetVertex(substrate->getVertex(i));
            Vertex* v2 = products->getVertex(i);
            substratePatternToProductMapping.addMapping(v1,v2);
        }
    }

    // Go through each vertext in the substrate pattern.
    for (int i=0; i<substratePattern->getNumberVertices(); i++)
    {
        Vertex* substratePatternVertex = substratePattern->getVertex(i);
        Vertex* productPatternVertex = substratePatternToProductPatternMapping.getTargetVertex(substratePatternVertex);
        printf("1: %s\n",substratePatternVertex->getString().c_str());
        printf("2: %s\n",productPatternVertex->getString().c_str());

        // Go through each edge and see if it was changed.
        for (int j=0; j<substratePatternVertex->getMaxNumberEdges(); j++)
        {
            // See if an edge was added.
            if (substratePatternVertex->getEdge(j) == NULL && productPatternVertex->getEdge(j) != NULL)
            {
                printf("Edge added\n");
            }

            // See if an edge was removed.
            else if (substratePatternVertex->getEdge(j) != NULL && productPatternVertex->getEdge(j) == NULL)
            {
                Vertex* v1 = substratePatternToProductMapping.getTargetVertex(substratePatternVertex);
                Vertex* v2 = substratePatternToProductMapping.getTargetVertex(substratePatternVertex->getEdge(j));
                if (!products->removeEdge(v1, v2)) throw Exception("could not remove the edge from the product", v1->getString().c_str(), v2->getString().c_str(), products->getString().c_str());
                printf("Edge removed %s %s: %s\n", v1->getString().c_str(), v2->getString().c_str(), products->getString().c_str());
            }
        }

        //Vertex* substrateVertex = substratePatternMapping.getSourceVertex(substratePatternVertex);
        //if (substrateVertex == NULL) throw Exception("did not have a mapping for the substrate molecule pattern", substratePatternVertex->getString().c_str());


    }

    // TODO: break apart any molecules that are no longer in a complex in the product.


    list<ComplexInstance*> ret;
    return ret;
}

void BNGLImporter::processReactionSecondOrder(int round, ReactionPattern* reaction)
{
    /*
    // Get the substrate patterns.
    ComplexPattern* substrate1Pattern = reaction->getSubstrates()->getReactant(0);
    ComplexPattern* substrate2Pattern = reaction->getSubstrates()->getReactant(1);

    // Go through every possible pair of complex species from the previous round and see if they match the patterns.
    for (int i=0; i<complexSpecies[round-1].size(); i++)
    {
        for (int j=0; j<complexSpecies[round-1].size(); j++)
        {
            ComplexInstance substrate1 = complexSpecies[round-1][i];
            ComplexInstance substrate2 = complexSpecies[round-1][j];
            if (substrate1Pattern.matchesTo(substrate1) && substrate2Pattern.matchesTo(substrate2))
            {
                if (verbose)
                {
                    Print::printf(Print::INFO, "Found reaction match in round %d: %s + %s: %s + %s",round, substrate1Pattern.getString().c_str(), substrate2Pattern.getString().c_str(), substrate1.getString().c_str(), substrate2.getString().c_str());
                }
            }
        }
    }
    */
}

/*
void BNGLImporter::importGlobalExpressions()
{
    // Process any assignment rules.
    if (sbmlModel->getNumRules() > 0)
    {
        Print::printf(Print::INFO, "Processing %d rules.", sbmlModel->getNumRules());
        for (int i=0; i<sbmlModel->getNumRules(); i++)
        {
            if (sbmlModel->getRule(i)->getTypeCode() == SBML_ASSIGNMENT_RULE)
            {
                AssignmentRule* rule = (AssignmentRule*)sbmlModel->getRule(i);
                globalExpressions[rule->getVariable()] = rule->getMath()->deepCopy();
                Print::printf(Print::INFO, "Added rule (%d) as assignment rule %s: %s", i, rule->getVariable().c_str(), SBML_formulaToL3String(globalExpressions[rule->getVariable()]));
            }
        }
    }

    // Substitute any assignment rules with user specified rules.
    for (map<string,string>::iterator it=userExpressions.begin(); it!=userExpressions.end(); it++)
    {
        if (globalExpressions.count(it->first) == 0)
        {
            globalExpressions[it->first] = SBML_parseL3Formula(it->second.c_str());
            Print::printf(Print::INFO, "Added user defined assignment rule %s: %s", it->first.c_str(), SBML_formulaToL3String(globalExpressions[it->first]));
        }
        else
        {
            globalExpressions[it->first] = SBML_parseL3Formula(it->second.c_str());
            Print::printf(Print::INFO, "Overriding assignment rule with user definition %s: %s", it->first.c_str(), SBML_formulaToL3String(globalExpressions[it->first]));
        }
    }
}

void BNGLImporter::importCompartments()
{
    if (sbmlModel->getNumCompartments() != 1) throw Exception("Must specify one and only one compartment, read ",sbmlModel->getNumCompartments());

    Print::printf(Print::INFO, "Processing %d compartments.", sbmlModel->getNumCompartments());
    for (int i=0; i<sbmlModel->getNumCompartments(); i++)
    {
        if (sbmlModel->getCompartment(i)->getSpatialDimensions() == 3)
        {
            string compartmentId = sbmlModel->getCompartment(i)->getId();
            compartments.push_back(compartmentId);
            compartmentSizes[compartmentId] = convertVolumeToLiters(sbmlModel->getCompartment(i)->getSize(), sbmlModel->getCompartment(i)->getUnits());
            Print::printf(Print::INFO, "Added compartment (%d) %s: %e L", i, compartmentId.c_str(), compartmentSizes[compartmentId]);
        }
        else
        {
            throw Exception("Unsupported compartment dimensions", sbmlModel->getCompartment(i)->getSpatialDimensions());
        }
    }
}

void BNGLImporter::importSpecies()
{
    // Process the species.
    numberSpecies = sbmlModel->getNumSpecies();
    reactionModel.set_number_species(numberSpecies);

    Print::printf(Print::INFO, "Processing %d species.", numberSpecies);
    for (int i=0; i<numberSpecies; i++)
    {
        Species* species = sbmlModel->getSpecies(i);
        speciesIndices[species->getId()] = i;

        // Make sure we can process the species.
        isSpeciesBoundary[i] = species->getBoundaryCondition();

        // Track if the species is constant.
        isSpeciesConst[i] = species->getConstant();

        // Make sure we can process the species.
        if (species->isSetConversionFactor()) throw Exception("Unsupported species property", "conversionFactor must not be set");

        // Get the initial count for the species.
        int initialSpeciesCount=0;
        if (species->isSetInitialAmount())
        {
            initialSpeciesCount=lround(convertSubstanceToParticles(species->getInitialAmount(), species->getSubstanceUnits()));
        }
        else if (species->isSetInitialConcentration())
        {
            initialSpeciesCount=lround(convertSubstanceToParticles(species->getInitialConcentration()*compartmentSizes[species->getCompartment()], species->getSubstanceUnits()));
        }
        else
        {
            throw Exception("Unknown initial count for species",species->getId().c_str());
        }

        // Add the species to the model.
        reactionModel.add_initial_species_count(initialSpeciesCount);
        Print::printf(Print::INFO, "Added species (%d) %s with initial count: %d%s%s", i, species->getId().c_str(), initialSpeciesCount, isSpeciesConst[i]?" (constant)":"", isSpeciesBoundary[i]?" (boundary)":"");
    }
}


void BNGLImporter::importReactions()
{
    // Process the reactions.
    numberReactions = sbmlModel->getNumReactions();
    reactionModel.set_number_reactions(numberReactions);

    // Initialize the stoichiometry matrix.
    S = new ndarray<int>(utuple(numberSpecies, numberReactions));
    *S=0;

    // Initialize the reaction type matrix.
    T = new ndarray<int>((utuple(numberReactions)));
    *T=9999;

    // Initialize the rate constant matrix.
    K = new ndarray<double>(utuple(numberReactions,10));
    *K=NAN;

    // Initialize the dependency matrix.
    D = new ndarray<int>(utuple(numberSpecies, numberReactions));
    *D=0;

    Print::printf(Print::INFO, "Processing %d reactions.", numberReactions);
    for (uint i=0; i<numberReactions; i++)
    {
        Reaction * reaction = sbmlModel->getReaction(i);

        // Make sure we can process the reaction.
        if (reaction->getReversible()) throw Exception("Unsupported reaction property", "reversible must be false");
        if (!reaction->isSetKineticLaw()) throw Exception("Unsupported reaction property", "must have a kinetic law");

        // Go through the list of reactants.
        for (uint j=0; j<reaction->getNumReactants(); j++)
        {
            SpeciesReference* reactant = reaction->getReactant(j);
            uint speciesIndex = speciesIndices[reactant->getSpecies()];

            // If the species is not constant and not a boundary condition, set the S matrix entry.
            if (!isSpeciesConst[speciesIndex] && !isSpeciesBoundary[speciesIndex])
            {
                // Make sure we can process the reactant.
                if (!reactant->isSetStoichiometry()) throw Exception("Unsupported reaction property", "stoichiometry for reactants must be set");
                if (!reactant->getConstant()) throw Exception("Unsupported reaction property", "stoichiometry for reactants must be constant");

                // Make the proper entry in the S matrix.
                (*S)[utuple(speciesIndex,i)] -= reactant->getStoichiometry();
            }
            else if (verbose && isSpeciesConst[speciesIndex])
            {
                Print::printf(Print::INFO, "Skipping entry in S matrix for reaction %d and constant species %d.", i, speciesIndex);
            }
            else if (verbose && isSpeciesBoundary[speciesIndex])
            {
                Print::printf(Print::INFO, "Skipping entry in S matrix for reaction %d and boundary species %d.", i, speciesIndex);
            }
        }

        // Go through the list of products.
        for (uint j=0; j<reaction->getNumProducts(); j++)
        {
            SpeciesReference * product = reaction->getProduct(j);
            uint speciesIndex = speciesIndices[product->getSpecies()];

            // If the species is not constant, set the S matrix entry.
            if (!isSpeciesConst[speciesIndex] && !isSpeciesBoundary[speciesIndex])
            {
                // Make sure we can process the product.
                if (!product->isSetStoichiometry()) throw Exception("Unsupported reaction property", "stoichiometry for products must be set");
                if (!product->getConstant()) throw Exception("Unsupported reaction property", "stoichiometry for products must be constant");

                // Make the proper entry in the S matrix.
                (*S)[utuple(speciesIndex,i)] += product->getStoichiometry();
            }
            else if (verbose && isSpeciesConst[speciesIndex])
            {
                Print::printf(Print::INFO, "Skipping entry in S matrix for reaction %d and constant species %d.", i, speciesIndex);
            }
            else if (verbose && isSpeciesBoundary[speciesIndex])
            {
                Print::printf(Print::INFO, "Skipping entry in S matrix for reaction %d and boundary species %d.", i, speciesIndex);
            }
        }

        // Process the kinetic law.
        if (!importKinetics(reaction, i, reaction->getKineticLaw()))
        {
            allImportStepsSuccessful = false;
            if (stopOnUnmatchedReactions) throw Exception("Could not match a reaction in the SBML file. Either fix the errors or execute the command again with the --ignore-unmatched flag set.");
        }
    }

    if (verbose)
    {
        Print::printf(Print::INFO, "Reaction type matrix was:");
        T->print(); printf("\n");
        Print::printf(Print::INFO, "Rate constant matrix was:");
        K->print(); printf("\n");
        Print::printf(Print::INFO, "Stoichiometry matrix was:");
        S->print(); printf("\n");
        Print::printf(Print::INFO, "Dependency matrix was:");
        D->print(); printf("\n");
    }

    // Fill in the reaction model.
    for (int i=0; i<numberSpecies; i++)
    {
        for (int j=0; j<numberReactions; j++)
        {
            reactionModel.add_stoichiometric_matrix((*S)[utuple(i,j)]);
            reactionModel.add_dependency_matrix((*D)[utuple(i,j)]);
        }
    }
    for (int j=0; j<numberReactions; j++)
    {
        lm::input::ReactionModel_Reaction* reaction = reactionModel.add_reaction();
        reaction->set_type((*T)[utuple(j)]);
        for (int k=0; k<10 && !isnan((*K)[utuple(j,k)]); k++)
            reaction->add_rate_constant((*K)[utuple(j,k)]);
    }
}

bool BNGLImporter::importKinetics(Reaction* reaction, int reactionIndex, KineticLaw* kinetics)
{
    map<string,double> localParameters;

    // Bring the global parameters into the local scope.
    for (map<string,double>::iterator it = globalParameters.begin(); it != globalParameters.end(); it++)
    {
        localParameters[it->first] = it->second;
    }

    // Get a list of the local parameters.
    for (uint i=0; i<kinetics->getNumLocalParameters(); i++)
    {
        LocalParameter * localParameter = kinetics->getLocalParameter(i);
        if (!localParameter->isSetValue()) throw Exception("Unsupported reaction property", "value for local parameters must be set");
        localParameters[localParameter->getId()] = localParameter->getValue();
    }

    // Go through all of the propensity functions and see if we can find a match.
    return importPropensityFunction(reaction, reactionIndex, kinetics, localParameters);
}

ASTNode_t* BNGLImporter::filterKineticExpression(ASTNode_t* expression)
{
    return expression;
}

bool BNGLImporter::importPropensityFunction(Reaction* reaction, int reactionIndex, KineticLaw* kinetics, map<string,double>& parameterValues)
{
    if (verbose) Print::printf(Print::INFO, "Matching kinetic formula in reaction %s (%d) at line %d to a propensity function: [%s] ", reaction->getId().c_str(), reactionIndex, kinetics->getLine(), SBML_formulaToL3String(kinetics->getMath()));

    // Get the kinetic expression.
    ASTNode_t* originalFormula = SBML_parseL3Formula(SBML_formulaToL3String(kinetics->getMath()));
//    printf("original: %s\n", SBML_formulaToL3String(kinetics->getMath()));
//    printf("formula: %s\n", SBML_formulaToL3String(originalFormula));
//    printASTNode(originalFormula);

    // Apply any subclass filtering.
    originalFormula = filterKineticExpression(originalFormula);

    // Recursively substitute expressions until we don't have any.
    ASTNode_t* substitutedFormula = originalFormula->deepCopy();
    while (ASTHelper::substituteASTExpression(substitutedFormula, globalExpressions));
    if (reallyVerbose)
    {
        printf("substituted: %s\n", SBML_formulaToL3String(substitutedFormula));
        //ASTHelper::printASTNode(substitutedFormula);
    }

    // Put the formula into normal form.
    ASTNode_t* normalizedFormula = substitutedFormula->deepCopy();
    ASTHelper::normalizeASTExpression(normalizedFormula);
    if (reallyVerbose)
    {
        printf("normalized: %s\n", SBML_formulaToL3String(normalizedFormula));
        ASTHelper::printASTNode(normalizedFormula);
    }

    // Substituting any parameters.
    ASTNode_t* parameterizedFormula = normalizedFormula->deepCopy();
    ASTHelper::substituteASTParameters(parameterizedFormula, parameterValues);
    if (reallyVerbose)
    {
        printf("parameterized: %s\n", SBML_formulaToL3String(parameterizedFormula));
        ASTHelper::printASTNode(parameterizedFormula);
    }

    // Simplify the formula.
    ASTNode_t* simplifiedFormula = parameterizedFormula->deepCopy();
    ASTHelper::simplifyASTExpression(simplifiedFormula);
    if (reallyVerbose)
    {
        printf("simplified: %s\n", SBML_formulaToL3String(simplifiedFormula));
        ASTHelper::printASTNode(simplifiedFormula);
    }

    // Iterate through each propensity function and see if it matches.
    map<uint,lm::me::PropensityFunctionDefinition> functions = propensityFunctions->getFunctions();
    for (std::map<uint,lm::me::PropensityFunctionDefinition>::const_iterator it=functions.begin(); it != functions.end(); it++)
    {
        uint id = it->first;
        lm::me::PropensityFunctionDefinition p = it->second;
        if (p.expressions.size() > 0)
        {
            for (list<string>::iterator it = p.expressions.begin(); it != p.expressions.end(); it++)
            {
                ASTNode_t* propensityFormula = SBML_parseL3Formula(it->c_str());
                ASTNode_t* normalizedPropensityFormula = propensityFormula->deepCopy();
                ASTHelper::normalizeASTExpression(normalizedPropensityFormula);
                if (ASTHelper::compareASTNodes(parameterizedFormula, normalizedPropensityFormula))
                {
                    (*T)[utuple(reactionIndex)] = id;
                    Print::printf(Print::INFO, "Matched kinetic formula in reaction %s (%d) to %s: [%s] == [%s]", reaction->getName().c_str(), reactionIndex, p.name.c_str(), SBML_formulaToL3String(parameterizedFormula), SBML_formulaToL3String(normalizedPropensityFormula));
                    Print::printf(Print::DEBUG, "                                         Original form:   [%s]", SBML_formulaToL3String(kinetics->getMath()));
                    Print::printf(Print::DEBUG, "                                         Normalized form: [%s]", SBML_formulaToL3String(normalizedFormula));

                    // Create the entry for this formula.
                    return createPropensityFunctionEntry(reactionIndex, parameterizedFormula, normalizedPropensityFormula, p);
                }
                else if (ASTHelper::compareASTNodes(simplifiedFormula, normalizedPropensityFormula))
                {
                    (*T)[utuple(reactionIndex)] = id;
                    Print::printf(Print::INFO, "Matched kinetic formula in reaction %s (%d) to %s: [%s] == [%s]", reaction->getName().c_str(), reactionIndex, p.name.c_str(), SBML_formulaToL3String(simplifiedFormula), SBML_formulaToL3String(normalizedPropensityFormula));
                    Print::printf(Print::DEBUG, "                                         Original form:   [%s]", SBML_formulaToL3String(kinetics->getMath()));
                    Print::printf(Print::DEBUG, "                                         Normalized form: [%s]", SBML_formulaToL3String(normalizedFormula));

                    // Create the entry for this formula.
                    return createPropensityFunctionEntry(reactionIndex, simplifiedFormula, normalizedPropensityFormula, p);
                }
                else
                {
                    if (verbose)
                    {
                        Print::printf(Print::INFO, "No match to [%s]: %s", SBML_formulaToL3String(normalizedPropensityFormula), p.name.c_str());
                        if (reallyVerbose)
                        {
                            ASTHelper::printASTNode(normalizedPropensityFormula);
                        }
                    }
                }
            }
        }
    }

    // Print out some messages to help the user figure out why there wasn't a match.
    Print::printf(Print::ERROR, "FAILED to match kinetic formula in reaction %s (%d) at line %d to a propensity function: [%s] ", reaction->getName().c_str(), reactionIndex, kinetics->getLine(), SBML_formulaToL3String(simplifiedFormula));
    if (verbose)
    {
        Print::printf(Print::ERROR, "                                         Normalized formula: [%s]", SBML_formulaToL3String(normalizedFormula));
        Print::printf(Print::ERROR, "                                         Original formula:   [%s]", SBML_formulaToL3String(kinetics->getMath()));
        Print::printf(Print::ERROR, "Abstract syntax tree for simplified form was:");
        ASTHelper::printASTNode(simplifiedFormula);
    }

    delete simplifiedFormula;
    return false;
}

bool BNGLImporter::createPropensityFunctionEntry(int reactionIndex, ASTNode_t* formula, ASTNode_t* propensityFormula, lm::me::PropensityFunctionDefinition& propensityFunction)
{
    // If this is a number and it matches to a k, store the parameter.
    if (formula->isNumber() && propensityFormula->isName() && propensityFormula->getName()[0] == 'k')
    {
        uint parameterIndex = atoi(propensityFormula->getName()+1)-1;
        utuple index = utuple(reactionIndex,parameterIndex);
        if (formula->getType() == AST_INTEGER)
        {
            double value = convertPropensityConstantUnits(propensityFormula->getName(), (double)formula->getInteger(), propensityFunction.getConstantUnits(parameterIndex));
            if (isnan((*K)[index]))
            {
                (*K)[index] = value;
                if (verbose) Print::printf(Print::INFO, "    Added parameter for reaction %d parameter %d: %e", reactionIndex, parameterIndex, (*K)[utuple(reactionIndex,parameterIndex)]);
                return true;
            }
            else if ((*K)[index] == value)
            {
                return true;
            }
            else
            {
                Print::printf(Print::ERROR, "FAILED to create entry for reaction %d parameter %d: the specified value did not match the previous value for this constant, %e != %e.", reactionIndex, parameterIndex, value, (*K)[index]);
                return false;
            }
        }
        else if (formula->getType() == AST_REAL || formula->getType() == AST_REAL_E)
        {
            double value = convertPropensityConstantUnits(propensityFormula->getName(), formula->getReal(), propensityFunction.getConstantUnits(parameterIndex));
            if (isnan((*K)[index]))
            {
                (*K)[index] = value;
                if (verbose) Print::printf(Print::INFO, "    Added parameter for reaction %d parameter %d: %e", reactionIndex, parameterIndex, (*K)[utuple(reactionIndex,parameterIndex)]);
                return true;
            }
            else if ((*K)[index] == value)
            {
                return true;
            }
            else
            {
                Print::printf(Print::ERROR, "FAILED to create entry for reaction %d parameter %d: the specified value did not match the previous value for this constant, %e != %e.", reactionIndex, parameterIndex, value, (*K)[index]);
                return false;
            }
        }
        Print::printf(Print::ERROR, "FAILED to create entry for reaction %d parameter %d: the value did not match a known numeric type.", reactionIndex, parameterIndex);
        return false;
    }

    // If this is a name and it matches to an x, store the dependency.
    if (formula->isName() && propensityFormula->isName() && propensityFormula->getName()[0] == 'x')
    {
        // Make sure the name is a valid species.
        if (speciesIndices.count(formula->getName()) == 0)
        {
            Print::printf(Print::ERROR, "FAILED to create entry for reaction %d: the name %s did not match a known species.", reactionIndex, formula->getName());
            return false;
        }

        int speciesIndex = speciesIndices[formula->getName()];
        utuple index = utuple(speciesIndex,reactionIndex);
        int speciesOrder = atoi(propensityFormula->getName()+1);

        // Add an entry in the D matrix.
        if ((*D)[index] == 0)
        {
            (*D)[index] = speciesOrder;
            if (verbose) Print::printf(Print::INFO, "    Added dependency for reaction %d on species %d (%s): %d", reactionIndex, speciesIndex, formula->getName(), speciesOrder);
            return true;
        }
        else if ((*D)[index] == speciesOrder)
        {
            return true;
        }
        else
        {
            Print::printf(Print::ERROR, "FAILED to create entry for reaction %d species %d: the specified order did not match the previous order for this species, %d != %d.", reactionIndex, speciesIndex, speciesOrder, (*D)[index]);
            return false;
        }
    }

    // Go through all of the children, if any failed return false.
    bool ret = true;
    for (int i=0; i<formula->getNumChildren(); i++)
        if (!createPropensityFunctionEntry(reactionIndex,formula->getChild(i), propensityFormula->getChild(i), propensityFunction))
            ret = false;
    return ret;
}

double BNGLImporter::convertPropensityConstantUnits(string constantName, double value, string desiredUnits)
{
    ASTNode_t* units = SBML_parseL3Formula(desiredUnits.c_str());
    convertUnits(units);
    double conversion = ASTHelper::evaluateASTOperator(units);
    if (conversion != 1.0) Print::printf(Print::INFO, "Converted kinetic rate constant %s from %e to %e %s", constantName.c_str(), value, value*conversion, desiredUnits.c_str());
    return value*conversion;
}

void BNGLImporter::convertUnits(ASTNode_t* units)
{
    // If this is a terminal node, figure out what type of unit this is.
    if (units->isName())
    {
        if (string(units->getName()) == "second")
        {
            units->setType(AST_REAL);
            units->setValue(convertTimeToSeconds(1.0));
        }
        else if (string(units->getName()) == "item")
        {
            // See if rate constants are given using particle counts or concentrations.
            if (!constantsUseConcentrations)
            {
                // The constants is already a particle count, so no volume adjsutment needed.
                units->setType(AST_REAL);
                units->setValue(convertSubstanceToParticles(1.0));
            }
            else
            {
                // Otherwise, the constant is a concentration, so we need to mulpiply by the compartment volume.
                units->setType(AST_REAL);
                units->setValue(convertSubstanceToParticles(1.0*compartmentSizes[compartments[0]]));
            }
        }
        else
        {
            throw Exception("Unsupported unit definition in propensity constant", units->getName());
        }
    }

    // Go through all of the children, if any failed return false.
    for (int i=0; i<units->getNumChildren(); i++)
        convertUnits(units->getChild(i));
}



double BNGLImporter::convertVolumeToLiters(double value, string units)
{
    // If we didn't get any units, use the default volume units.
    if (units == "") units = sbmlModel->getVolumeUnits();

    // See if the units are in liters.
    if (units == "litre")
    {
        return value;
    }

    // Otherwise, see if the units have a match in the unit definitions.
    else if (sbmlModel->getUnitDefinition(units) != NULL)
    {
        // Get the definition for the units.
        UnitDefinition* unitDef = sbmlModel->getUnitDefinition(units);
        if (unitDef->getNumUnits() == 1 && unitDef->getUnit(0)->getKind() == UNIT_KIND_LITRE && unitDef->getUnit(0)->getExponent() == 1)
            return value*unitDef->getUnit(0)->getMultiplier()*pow(10,unitDef->getUnit(0)->getScale());
        else
            throw Exception("Unsupported volume unit definition", unitDef->toSBML());
    }

    // Otherwise, throw an exception.
    else
    {
        throw Exception("Unsupported volume units", units.c_str());
    }
}

double BNGLImporter::convertSubstanceToParticles(double value, string units)
{
    // If we didn't get any units, use the default volume units.
    if (units == "") units = sbmlModel->getSubstanceUnits();

    // See if the units are in liters.
    if (units == "item")
    {
        return value;
    }
    else if (units == "mole")
    {
        return value*6.02214085774e23;
    }

    // Otherwise, see if the units have a match in the unit definitions.
    else if (sbmlModel->getUnitDefinition(units) != NULL)
    {
        // Get the definition for the units.
        UnitDefinition* unitDef = sbmlModel->getUnitDefinition(units);
        if (unitDef->getNumUnits() == 1 && unitDef->getUnit(0)->getKind() == UNIT_KIND_ITEM && unitDef->getUnit(0)->getExponent() == 1)
            return value*unitDef->getUnit(0)->getMultiplier()*pow(10,unitDef->getUnit(0)->getScale());
        else if (unitDef->getNumUnits() == 1 && unitDef->getUnit(0)->getKind() == UNIT_KIND_MOLE && unitDef->getUnit(0)->getExponent() == 1)
            return value*unitDef->getUnit(0)->getMultiplier()*pow(10,unitDef->getUnit(0)->getScale())*6.02214085774e23;
        else
            throw Exception("Unsupported substance unit definition", unitDef->toSBML());
    }

    // Otherwise, throw an exception.
    else
    {
        throw Exception("Unsupported substance units", units.c_str());
    }
}

double BNGLImporter::convertTimeToSeconds(double value, string units)
{
    // If we didn't get any units, use the default volume units.
    if (units == "") units = sbmlModel->getTimeUnits();

    // See if the units are in seconds.
    if (units == "second")
    {
        return value;
    }

    // Otherwise, see if the units have a match in the unit definitions.
    else if (sbmlModel->getUnitDefinition(units) != NULL)
    {
        // Get the definition for the units.
        UnitDefinition* unitDef = sbmlModel->getUnitDefinition(units);
        if (unitDef->getNumUnits() == 1 && unitDef->getUnit(0)->getKind() == UNIT_KIND_SECOND && unitDef->getUnit(0)->getExponent() == 1)
            return value*unitDef->getUnit(0)->getMultiplier()*pow(10,unitDef->getUnit(0)->getScale());
        else
            throw Exception("Unsupported volume unit definition", unitDef->toSBML());
    }

    // Otherwise, throw an exception.
    else
    {
        throw Exception("Unsupported volume units", units.c_str());
    }
}
*/

}
}
