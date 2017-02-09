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

using std::list;
using std::regex;
using lm::Exception;
using lm::Print;
using robertslab::sbml::ASTHelper;

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

    regex beginPattern("^begin\\s+(.+)\\s*$");
    regex endPattern("^end\\s+(.+)\\s*$");
    string section = "";
    list<string> sectionLines;
    int lineNumber=0;
    while (!input.eof())
    {
        lineNumber++;
        string line;
        std::getline(input,line);

        // Strip any comments.
        line = line.substr(0,line.find_first_of('#'));

        // Strip any trailing whitespace.
        line = line.substr(0,line.find_last_not_of(" \t\r\n")+1);

        // Check for section blocks.
        std::smatch match;
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
                    parseSpecies(sectionLines, false);
                }
                else if (section == "species" || section == "seed species" )
                {
                    parseSpecies(sectionLines, true);
                }
                else if (section == "reaction rules")
                {
                    parseReactions(sectionLines);
                }
                else
                {
                    Print::printf(Print::INFO, "Ignoring unsupported BNGL block: %s", section.c_str());
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

    regex parameterPattern("^\\s*(?:\\d*\\s+)?(\\S+)\\s+(\\S+)\\s*$");
    std::smatch match;
    for (list<string>::iterator it=lines.begin(); it != lines.end(); it++)
    {
        string line = *it;
        if (std::regex_match(line, match, parameterPattern) && match.size() == 3)
        {
            // Get the key and the expression.
            string key = match[1].str();
            string expression = match[2].str();

            // Simplify the formula.
            ASTNode_t* formula = SBML_parseL3Formula(expression.c_str());
            ASTHelper::substituteASTParameters(formula, parameters);
            ASTHelper::simplifyASTExpression(formula);

            // If we are printing debug info, print the AST tree.
            if (reallyVerbose)
            {
                Print::printf(Print::INFO, "Simplified parameter %s as:", key.c_str());
                ASTHelper::printASTNode(formula);
            }

            // If we got to a numeric expression, save it.
            if (ASTHelper::isASTNumeric(formula))
            {
                double value = ASTHelper::getNumericValue(formula);
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
            Print::printf(Print::WARNING, "Could not parse line from block: %s", line.c_str());
        }
    }

    /*
    // Process any global parameters.
    if (sbmlModel->getNumParameters())
    {
        Print::printf(Print::INFO, "Processing %d parameters.", sbmlModel->getNumParameters());
        for (int i=0; i<sbmlModel->getNumParameters(); i++)
        {
            if (sbmlModel->getParameter(i)->getConstant())
            {
            }
            else
            {
                if (!ignoreVariableParameters) throw Exception("Found non-constant global parameter. Either remove the parameter or execute the command again with the --ignore-variable-parameters flag set.", sbmlModel->getParameter(i)->toSBML());
                Print::printf(Print::WARNING, "Skipped variable parameter (%d) %s: %e", i, sbmlModel->getParameter(i)->getId().c_str(), sbmlModel->getParameter(i)->getValue());
            }
        }
    }
    */

}

void BNGLImporter::parseSpecies(list<string>& lines, bool initialize)
{
    if (!initialize)
        Print::printf(Print::INFO, "Parsing molecules block.");
    else
        Print::printf(Print::INFO, "Parsing species block.");
}

void BNGLImporter::parseReactions(list<string>& lines)
{
    Print::printf(Print::INFO, "Parsing reactions block.");
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
