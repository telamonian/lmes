/*
 * University of Illinois Open Source License
 * Copyright 2008-2012 Luthey-Schulten Group,
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
 *
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the Software), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimers.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimers in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS WITH THE SOFTWARE.
 *
 * Author(s): Elijah Roberts, Max Klein
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sbml/conversion/ConversionProperties.h>
#include <sbml/math/FormulaFormatter.h>
#include <sbml/SBMLDocument.h>
#include <sbml/SBMLReader.h>
#include <sbml/SBMLTypes.h>
#include <sbml/xml/XMLErrorLog.h>
#include "lm/ClassFactory.h"
#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/Version.h"
#include "lm/cme/CMEPropensityFunctions.h"
#include "lm/me/PropensityFunction.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lptf/Profile.h"

void printCopyright(int argc, char** argv);
void parseArguments(int argc, char** argv);
void printUsage(int argc, char** argv);

using std::map;
using std::vector;
using std::string;
using lm::Exception;
using lm::io::ReactionModel;
using lm::io::hdf5::Hdf5File;
using lm::Print;

/**
 * The function being performed.
 */
string function = "";

/**
 * The lm file to output the model into.
 */
string outputFilename = "";

/**
 * The sbml file to import the model from.
 */
string inputFilename = "";

/**
 * Whether to ignore errors in the SBML file.
 */
bool ignoreErrors = false;

/**
 * Whether to print out more verbose messages.
 */
bool verbose = false;
bool reallyVerbose = false;

/**
 * Copasi always sets species concentrations (instead of amount) in exported sbml, so fix that
 */
bool isCopasi = false;

/**
 * Assignment rules specified by the user.
 */
map<string,string> userRules;

/**
 * Parameters specified by the user.
 */
map<string,double> userParameters;

void importSBMLModel(Hdf5File * lmFile, string sbmlFilename) throw(Exception);
bool importSBMLModelL3V1(ReactionModel * lmModel, Model * sbmlModel) throw(Exception);
bool importSBMLModelL3V1Kinetics(Reaction * reaction, uint reactionIndex, KineticLaw * kinetics, ndarray<uint>& T, ndarray<double>& K, ndarray<uint>& D, map<string,uint> & speciesIndices, uint numberReactions, map<string,double> & globalParameterValues, map<string,ASTNode_t*>& globalExpressions) throw(Exception);
bool matchL3V1KineticsWithPropensityFunction(Reaction * reaction, uint reactionIndex, KineticLaw * kinetics, ndarray<uint>& T, ndarray<double>& K, ndarray<uint>& D, map<string,uint>& speciesIndices, map<string,double>& parameterValues, map<string,ASTNode_t*>& globalExpressions);
void printASTNode(const ASTNode_t* node, int depth=0);
bool substituteASTExpression(ASTNode_t* node, map<string,ASTNode_t*>& globalExpressions);
void normalizeASTExpression(ASTNode_t* node);
void sortASTExpression(ASTNode_t* node);
void simplifyASTExpression(ASTNode_t* node, map<string,double>& parameterValues);
bool areAllASTChildrenNumeric(ASTNode_t* node);
double evaluateASTOperator(const ASTNode_t * node);
double evaluateASTFunction(const ASTNode_t * node);
bool compareASTNodes(ASTNode_t* formula, ASTNode_t* propensityFormula);
bool createReactionModelEntry(ASTNode_t* sourceFormula, ASTNode_t* propensityFormula, uint reactionIndex, ndarray<double>& K, ndarray<uint>& D, map<string,uint>& speciesIndices);


// Allocate the profile space.
PROF_ALLOC;

int main(int argc, char** argv)
{	
    PROF_INIT;
	try
	{
		printCopyright(argc, argv);
		parseArguments(argc, argv);

        lm::ClassFactory::getInstance().printRegisteredClasses();
		
		if (function == "help")
		{
			printUsage(argc, argv);
		}
		else if (function == "version")
		{
		}
		else if (function == "import")
		{
            // Make sure the input file exists.
            struct stat fileStats;
            if (stat(inputFilename.c_str(), &fileStats) != 0)
            {
                throw Exception("The specified SBML file does not exist", inputFilename.c_str());
            }

		    // If the output file doesn't exist, create it.
		    if (stat(outputFilename.c_str(), &fileStats) != 0)
		    {
		        Hdf5File::create(outputFilename);
		    }

			// Open the file.
		    Hdf5File outputFile(outputFilename);

		    // Open the sbml file.
		    importSBMLModel(&outputFile, inputFilename);

		    // Close the file.
		    outputFile.close();
		}
		else
		{
			throw lm::CommandLineArgumentException("unknown function.");
		}
		return 0;
	}
    catch (lm::CommandLineArgumentException e)
    {
        std::cout << "Invalid command line argument: " << e.what() << std::endl << std::endl;
        printUsage(argc, argv);
    }
    catch (Exception e)
    {
        std::cout << "Exception during execution: " << e.what() << std::endl;
    }
    catch (std::exception e)
    {
        std::cout << "Exception during execution: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "Unknown Exception during execution." << std::endl;
    }
    return -1;
}

lm::me::PropensityFunctionFactory *factory;

void importSBMLModel(Hdf5File * lmFile, string sbmlFilename) throw(Exception)
{
    // Print the propensity functions that are registered.
    factory = new lm::me::PropensityFunctionFactory();
    if (verbose) factory->printRegisteredFunctions(Print::INFO);

    // Read in the SBML document.
    SBMLReader reader;
    std::auto_ptr<SBMLDocument> sbmlDocument(reader.readSBML(sbmlFilename));
    if (sbmlDocument->getNumErrors() > 0)
    {
        bool criticalErrrors = false;
        for (int i=0; i<sbmlDocument->getNumErrors(); i++)
        {
            const SBMLError* error = sbmlDocument->getError(i);
            if (error->getSeverity() >= LIBSBML_SEV_ERROR)
                criticalErrrors = true;
        }

        Print::printf(criticalErrrors?Print::ERROR:Print::WARNING,"Problems detected while parsing the SBML file %s",sbmlFilename.c_str());
        Print::printf(criticalErrrors?Print::ERROR:Print::WARNING,"-----------------------------------");
        sbmlDocument->printErrors(std::cout);
        Print::printf(criticalErrrors?Print::ERROR:Print::WARNING,"-----------------------------------");

        // If there were critical errors and we are not ignoring exceptions, stop.
        if (criticalErrrors && !ignoreErrors)
            throw Exception("There were critical errors detected while parsing the SBML file. Either fix the errors or execute the command again with the --ignore-errors flag set.");
    }

    // Make sure we know how to process the document.
    if (sbmlDocument->getLevel() == 3 && sbmlDocument->getVersion() == 1)
    {
        // expand any user-defined functions in the reaction kinetic laws
        ConversionProperties props;
        props.addOption("expandFunctionDefinitions");

        if (sbmlDocument->convert(props) != LIBSBML_OPERATION_SUCCESS)
        {
            Print::printf(Print::ERROR,"Problems detected while expanding function definitions in the SBML file %s\n",sbmlFilename.c_str());
            Print::printf(Print::ERROR,"-----------------------------------");
            sbmlDocument->printErrors(std::cout);
            Print::printf(Print::ERROR,"-----------------------------------");

            // If we are not ignoring exceptions, stop.
            if (!ignoreErrors)
                throw Exception("There were critical errors detected while expanding function definitions in the SBML file. Either fix the errors or execute the command again with the --ignore-errors flag set.");
        }

        // Build the reaction model from the SBML model.
        ReactionModel lmModel;
        Model * sbmlModel = sbmlDocument->getModel();
        if (importSBMLModelL3V1(&lmModel, sbmlModel))
        {
            lmFile->setReactionModel(&lmModel);
            Print::printf(Print::INFO, "There output file was successfully genereated.");
        }
        else
        {
            if (ignoreErrors)
            {
                lmFile->setReactionModel(&lmModel);
                Print::printf(Print::WARNING, "There were errors during the import process, but the output file was still generated. However, the output file is likely not entirely correct.");
            }
            else
            {
                Print::printf(Print::ERROR, "There were errors during the import process, the output file was not generated.  Either fix the errors or execute the command again with the --ignore-errors flag set.");
            }
        }

    }
    else
        throw Exception("Unsupported SBML format", sbmlDocument->getLevel(), sbmlDocument->getVersion());
}

bool importSBMLModelL3V1(ReactionModel * lmModel, Model * sbmlModel) throw(Exception)
{
    bool allSuccessful = true;

    map<string,double> globalParameterValues;
    map<string,ASTNode_t*> globalExpressions;

    // Get the units for the model.
    string modelSubstanceUnits = sbmlModel->getSubstanceUnits();

    // Process any assignment rules.
    if (sbmlModel->getNumRules() > 0)
    {
        Print::printf(Print::INFO, "Processing %d rules.", sbmlModel->getNumRules());
        for (uint i=0; i<sbmlModel->getNumRules(); i++)
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
    for (map<string,string>::iterator it=userRules.begin(); it!=userRules.end(); it++)
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

    // Process any global parameters.
    if (sbmlModel->getNumParameters())
    {
        Print::printf(Print::INFO, "Processing %d parameters.", sbmlModel->getNumParameters());
        for (uint i=0; i<sbmlModel->getNumParameters(); i++)
        {
            if (sbmlModel->getParameter(i)->getConstant())
            {
                globalParameterValues[sbmlModel->getParameter(i)->getId()] = sbmlModel->getParameter(i)->getValue();
                Print::printf(Print::INFO, "Added parameter (%d) %s: %e (constant)", i, sbmlModel->getParameter(i)->getId().c_str(), sbmlModel->getParameter(i)->getValue());
            }
            else
            {
                Print::printf(Print::INFO, "Added parameter (%d) %s: %s", i, sbmlModel->getParameter(i)->getId().c_str(), SBML_formulaToL3String(globalExpressions[sbmlModel->getParameter(i)->getId()]));
            }
        }
    }

    // Process the compartments.
    if (sbmlModel->getNumCompartments())
    {
        Print::printf(Print::INFO, "Processing %d compartments.", sbmlModel->getNumCompartments());
        for (uint i=0; i<sbmlModel->getNumCompartments(); i++)
        {
            globalParameterValues[sbmlModel->getCompartment(i)->getId()] = sbmlModel->getCompartment(i)->getSize();
            Print::printf(Print::INFO, "Added compartment (%d) %s: %e", i, sbmlModel->getCompartment(i)->getId().c_str(), sbmlModel->getCompartment(i)->getSize());
        }
    }

    // Substitute any global parameters with user specified parameters.
    for (map<string,double>::iterator it=userParameters.begin(); it!=userParameters.end(); it++)
    {
        globalParameterValues[it->first] = it->second;
        Print::printf(Print::INFO, "Added user defined parameter %s: %e", it->first.c_str(), globalParameterValues[it->first]);
    }

    // Try to simplify any global expressions.
    for (map<string,ASTNode_t*>::iterator it=globalExpressions.begin(); it!=globalExpressions.end(); it++)
    {
        string id = it->first;
        ASTNode_t* node = it->second;
        normalizeASTExpression(node);
        simplifyASTExpression(node, globalParameterValues);
        if (verbose) Print::printf(Print::INFO, "Simplified assignment rule %s: %s", id.c_str(), SBML_formulaToL3String(globalExpressions[id]));
    }

    // Process the species.
    uint numberSpecies = sbmlModel->getNumSpecies();
    Print::printf(Print::INFO, "Processing %d species.", numberSpecies);
    map<string,uint> speciesIndices;
    map<uint,bool> isSpeciesConst;
    lmModel->set_number_species(numberSpecies);
    for (uint i=0; i<numberSpecies; i++)
    {
        Species * species = sbmlModel->getSpecies(i);
        speciesIndices[species->getId()] = i;

        // Get the units for the amount.
        string substanceUnits = modelSubstanceUnits;
        if (!isCopasi)
        {
            if (species->isSetSubstanceUnits()) substanceUnits = species->getSubstanceUnits();
            if (substanceUnits != "item")
                throw Exception("Unsupported species substance units", substanceUnits.c_str());

            // Make sure we can process the species.
            if (!species->getHasOnlySubstanceUnits()) throw Exception("Unsupported species property", "hasOnlySubstanceUnits must be true");
            if (species->getBoundaryCondition()) throw Exception("Unsupported species property", "boundaryCondition must be false");
        }

        // Track if the species is constant.
        isSpeciesConst[i] = species->getConstant();

        // Make sure we can process the species.
        if (species->isSetConversionFactor()) throw Exception("Unsupported species property", "conversionFactor must not be set");

        // Get the initial count for the species.
        uint initialSpeciesCount = 0;
        if (species->isSetInitialAmount())
        {
            initialSpeciesCount = (uint)lround(species->getInitialAmount());
        }
        else if (species->isSetInitialConcentration())
        {
            if (isCopasi)
            {
                initialSpeciesCount = (uint)lround(sbmlModel->getCompartment(0)->getVolume() * species->getInitialConcentration());
            }
            else
            {
                throw Exception("Unsupported species property", "initialConcentration must not be set");
            }
        }
        else
            throw Exception("Unsupported species property", "initialAmount or initialConcentration must be set");

        // Add the species to the model.
        lmModel->add_initial_species_count(initialSpeciesCount);
        if (isSpeciesConst[i])
            Print::printf(Print::INFO, "Added species (%d) %s with initial count: %d (constant)", i, species->getId().c_str(), initialSpeciesCount);
        else
            Print::printf(Print::INFO, "Added species (%d) %s with initial count: %d", i, species->getId().c_str(), initialSpeciesCount);
    }

    // Process the reactions.
    uint numberReactions = sbmlModel->getNumReactions();
    Print::printf(Print::INFO, "Processing %d reactions.", numberReactions);

    // Initialize the stoichiometry matrix.
    ndarray<int> S(utuple(numberSpecies, numberReactions));
    S=0;

    // Initialize the reaction type matrix.
    ndarray<uint> T((utuple(numberReactions)));
    T=9999;

    // Initialize the rate constant matrix.
    ndarray<double> K(utuple(numberReactions,10));
    K=NAN;

    // Initialize the dependency matrix.
    ndarray<uint> D(utuple(numberSpecies, numberReactions));
    D=0;

    for (uint i=0; i<numberReactions; i++)
    {
        Reaction * reaction = sbmlModel->getReaction(i);

        // Make sure we can process the reaction.
        if (reaction->getReversible()) throw Exception("Unsupported reaction property", "reversible must be false");
        if (!reaction->isSetKineticLaw()) throw Exception("Unsupported reaction property", "must have a kinetic law");

        // Go through the list of reactants.
        for (uint j=0; j<reaction->getNumReactants(); j++)
        {
            SpeciesReference * reactant = reaction->getReactant(j);
            uint speciesIndex = speciesIndices[reactant->getSpecies()];

            // If the species is not constant, set the S matrix entry.
            if (!isSpeciesConst[speciesIndex])
            {
                // Make sure we can process the reactant.
                if (!reactant->isSetStoichiometry()) throw Exception("Unsupported reaction property", "stoichiometry for reactants must be set");
                if (!reactant->getConstant()) throw Exception("Unsupported reaction property", "stoichiometry for reactants must be constant");

                // Make the proper entry in the S matrix.
                S[utuple(speciesIndex,i)] -= reactant->getStoichiometry();
            }
            else if (verbose)
            {
                Print::printf(Print::INFO, "Skipping entry in S matrix for reaction %d and constant species %d.", i, speciesIndex);
            }
        }

        // Go through the list of products.
        for (uint j=0; j<reaction->getNumProducts(); j++)
        {
            SpeciesReference * product = reaction->getProduct(j);
            uint speciesIndex = speciesIndices[product->getSpecies()];

            // If the species is not constant, set the S matrix entry.
            if (!isSpeciesConst[speciesIndex])
            {
                // Make sure we can process the product.
                if (!product->isSetStoichiometry()) throw Exception("Unsupported reaction property", "stoichiometry for products must be set");
                if (!product->getConstant()) throw Exception("Unsupported reaction property", "stoichiometry for products must be constant");

                // Make the proper entry in the S matrix.
                S[utuple(speciesIndex,i)] += product->getStoichiometry();
            }
            else if (verbose)
            {
                Print::printf(Print::INFO, "Skipping entry in S matrix for reaction %d and constant species %d.", i, speciesIndex);
            }
        }

        // Process the kinetic law.
        KineticLaw * kinetics = reaction->getKineticLaw();
        if (!importSBMLModelL3V1Kinetics(reaction, i, kinetics, T, K, D, speciesIndices, numberReactions, globalParameterValues, globalExpressions))
            allSuccessful = false;
    }

    if (verbose)
    {
        Print::printf(Print::INFO, "Reaction type matrix was:");
        T.print(); printf("\n");
        Print::printf(Print::INFO, "Rate constant matrix was:");
        K.print(); printf("\n");
        Print::printf(Print::INFO, "Stoichiometry matrix was:");
        S.print(); printf("\n");
        Print::printf(Print::INFO, "Dependency matrix was:");
        D.print(); printf("\n");
    }

    // Fill in the reaction model.
    lmModel->set_number_reactions(numberReactions);
    for (int i=0; i<numberSpecies; i++)
    {
        for (int j=0; j<numberReactions; j++)
        {
            lmModel->add_stoichiometric_matrix(S[utuple(i,j)]);
            lmModel->add_dependency_matrix(D[utuple(i,j)]);
        }
    }
    for (int j=0; j<numberReactions; j++)
    {
        lm::io::ReactionModel_Reaction* reaction = lmModel->add_reaction();
        reaction->set_type(T[utuple(j)]);
        for (int k=0; k<10 && !isnan(K[utuple(j,k)]); k++)
            reaction->add_rate_constant(K[utuple(j,k)]);
    }

    return allSuccessful;
}

bool importSBMLModelL3V1Kinetics(Reaction * reaction, uint reactionIndex, KineticLaw * kinetics, ndarray<uint>& T, ndarray<double>& K, ndarray<uint>& D, map<string,uint> & speciesIndices, uint numberReactions, map<string,double>& globalParameterValues, map<string,ASTNode_t*>& globalExpressions) throw(Exception)
{
    map<string,double> localParameterValues;

	// Bring the global parameters into the local scope.
    for (map<string,double>::iterator it = globalParameterValues.begin(); it != globalParameterValues.end(); it++)
    {
        localParameterValues[it->first] = it->second;
    }

    // Get a list of the local parameters.
    for (uint i=0; i<kinetics->getNumLocalParameters(); i++)
    {
        LocalParameter * localParameter = kinetics->getLocalParameter(i);

        // See if the user specified this parameter.
        /*if ()
        {

        }

        // Otherwise, use the value from the file.
        else
        {*/
            if (!localParameter->isSetValue()) throw Exception("Unsupported reaction property", "value for local parameters must be set");
            localParameterValues[localParameter->getId()] = localParameter->getValue();
        //}
    }

    // Go through all of the propensity functions and see if we can find a match.
    return matchL3V1KineticsWithPropensityFunction(reaction, reactionIndex, kinetics, T, K, D, speciesIndices, localParameterValues, globalExpressions);
}

bool matchL3V1KineticsWithPropensityFunction(Reaction * reaction, uint reactionIndex, KineticLaw * kinetics, ndarray<uint>& T, ndarray<double>& K, ndarray<uint>& D, map<string,uint>& speciesIndices, map<string,double>& parameterValues, map<string,ASTNode_t*>& globalExpressions)
{
    if (verbose)
        Print::printf(Print::INFO, "Matching kinetic formula in reaction (%d) %s at line %d to a propensity function: [%s] ", reactionIndex, reaction->getName().c_str(), kinetics->getLine(), SBML_formulaToL3String(kinetics->getMath()));

    // Get the kinetic expression.
    const ASTNode_t* originalFormula = SBML_parseL3Formula(SBML_formulaToL3String(kinetics->getMath()));
//    printf("original: %s\n", SBML_formulaToL3String(kinetics->getMath()));
//    printf("formula: %s\n", SBML_formulaToL3String(originalFormula));
//    printASTNode(originalFormula);

    // Recursively substitute expressions until we don't have any.
    ASTNode_t* substitutedFormula = originalFormula->deepCopy();
    while (substituteASTExpression(substitutedFormula, globalExpressions));
    if (reallyVerbose)
    {
        printf("substituted: %s\n", SBML_formulaToL3String(substitutedFormula));
        printASTNode(substitutedFormula);
    }

    // Put the formula into normal form.
    ASTNode_t* normalizedFormula = substitutedFormula->deepCopy();
    normalizeASTExpression(normalizedFormula);
    if (reallyVerbose)
    {
        printf("normalized: %s\n", SBML_formulaToL3String(normalizedFormula));
        printASTNode(normalizedFormula);
    }

    // Simplify the formula by substituting parameters.
    ASTNode_t* simplifiedFormula = normalizedFormula->deepCopy();
    simplifyASTExpression(simplifiedFormula, parameterValues);
    if (reallyVerbose)
    {
        printf("simplified: %s\n", SBML_formulaToL3String(simplifiedFormula));
        printASTNode(simplifiedFormula);
    }

    // Iterate through each propensity function and see if it matches.
    map<uint,lm::me::PropensityFunctionDefinition> functions = factory->getFunctions();
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
                normalizeASTExpression(normalizedPropensityFormula);
                if (compareASTNodes(simplifiedFormula, normalizedPropensityFormula))
                {
                    T[utuple(reactionIndex)] = id;
                    Print::printf(Print::INFO, "Matched kinetic formula in reaction (%d) %s to %s: [%s] == [%s]", reactionIndex, reaction->getName().c_str(), p.name.c_str(), SBML_formulaToL3String(simplifiedFormula), SBML_formulaToL3String(normalizedPropensityFormula));
                    Print::printf(Print::DEBUG, "                                         Original form:   [%s]", SBML_formulaToL3String(kinetics->getMath()));
                    Print::printf(Print::DEBUG, "                                         Normalized form: [%s]", SBML_formulaToL3String(normalizedFormula));

                    // Create the entry for this formula.
                    return createReactionModelEntry(simplifiedFormula, normalizedPropensityFormula, reactionIndex, K, D, speciesIndices);
                }
                else
                {
                    if (verbose)
                    {
                        Print::printf(Print::INFO, "No match [%s] to [%s]: %s", SBML_formulaToL3String(simplifiedFormula), SBML_formulaToL3String(normalizedPropensityFormula), p.name.c_str());
                        if (reallyVerbose)
                        {
                            printASTNode(simplifiedFormula);
                            printASTNode(normalizedPropensityFormula);
                        }
                    }
                }
            }
        }
    }

    // Print out some messages to help the user figure out why there wasn't a match.
    Print::printf(Print::ERROR, "FAILED to match kinetic formula in reaction (%d) %s at line %d to a propensity function: [%s] ", reactionIndex, reaction->getName().c_str(), kinetics->getLine(), SBML_formulaToL3String(simplifiedFormula));
    if (verbose)
    {
        Print::printf(Print::ERROR, "                                         Normalized formula: [%s]", SBML_formulaToL3String(normalizedFormula));
        Print::printf(Print::ERROR, "                                         Original formula:   [%s]", SBML_formulaToL3String(kinetics->getMath()));
        Print::printf(Print::ERROR, "Abstract syntax tree for simplified form was:");
        printASTNode(simplifiedFormula);
    }

    delete simplifiedFormula;
    return false;
}

void printASTNode(const ASTNode_t* node, int depth)
{
    if (depth == 0) printf("------------------------\n");
    for (int i=0; i<depth; i++) printf("  ");
    if (node->getType() == AST_NAME)
        printf("%s\n", node->getName());
    else if (node->getType() == AST_PLUS)
        printf("%s\n", "PLUS");
    else if (node->getType() == AST_MINUS)
        printf("%s\n", "MINUS");
    else if (node->getType() == AST_TIMES)
        printf("%s\n", "TIMES");
    else if (node->getType() == AST_DIVIDE)
        printf("%s\n", "DIVIDE");
    else if (node->getType() == AST_POWER)
        printf("%s\n", "POWER");
    else if (node->getType() == AST_INTEGER)
        printf("%ld\n", node->getInteger());
    else if (node->getType() == AST_REAL)
        printf("%0.4e\n", node->getReal());
    else if (node->getType() == AST_REAL_E)
        printf("%0.4e\n", node->getReal());
    else if (node->getType() == AST_FUNCTION_POWER)
        printf("FN_POWER\n");
    else if (node->getType() == AST_FUNCTION_EXP)
        printf("FN_EXP\n");
    else
        printf("AST Type:%d\n", node->getType());
    for (int i=0; i<node->getNumChildren(); i++)
        printASTNode(node->getChild(i), depth+1);
    if (depth == 0) printf("------------------------\n");
}

bool substituteASTExpression(ASTNode_t* node, map<string,ASTNode_t*>& globalExpressions)
{
    bool anySubstitutions = false;

    // Perform substitutions in any child nodes.
    for (int i=0; i<node->getNumChildren(); i++)
    {
        ASTNode_t* child = node->getChild(i);
        if (child->getType() == AST_NAME && globalExpressions.count(child->getName()))
        {
            node->removeChild(i);
            node->insertChild(i, globalExpressions[child->getName()]->deepCopy());
            anySubstitutions = true;
        }
        else
        {
            if (substituteASTExpression(child, globalExpressions))
                anySubstitutions = true;
        }
    }
    return anySubstitutions;
}

void normalizeASTExpression(ASTNode_t* node)
{
    // Normalize the child nodes.
    for (int i=0; i<node->getNumChildren(); i++)
        normalizeASTExpression(node->getChild(i));

    // If this node is times and a child is times, remove it and bring its children up a level.
    if (node->getType() == AST_TIMES)
    {
        for (int i=0; i<node->getNumChildren(); i++)
        {
            ASTNode_t* child = node->getChild(i);
            if (child->getType() == AST_TIMES)
            {
                for (int j=0; j<child->getNumChildren(); j++)
                    node->addChild(child->getChild(j));
                while (child->getNumChildren())
                    child->removeChild(0);
                node->removeChild(i);
            }
        }
    }

    // If this node is times and a child is divide and its first child is times, merge the child values.
    if (node->getType() == AST_TIMES)
    {
        ASTNode_t* divisionChild = NULL;
        for (int i=0; i<node->getNumChildren(); i++)
        {
            if (node->getChild(i)->getType() == AST_DIVIDE && node->getChild(i)->getNumChildren() == 2 && node->getChild(i)->getChild(0)->getType() == AST_TIMES)
                divisionChild = node->getChild(i);
        }
        if (divisionChild != NULL)
        {
            // Move the children to the division node.
            for (int i=node->getNumChildren()-1; i>=0; i--)
            {
                ASTNode_t* child = node->getChild(i);
                if (child != divisionChild)
                {
                    divisionChild->getChild(0)->insertChild(0,child);
                }
            }

            // Remove all the children.
            while (node->getNumChildren()) node->removeChild(0);

            // Set ourselves as the division node, with the original divsion nodes' two child lsits.
            node->setType(AST_DIVIDE);
            node->addChild(divisionChild->getChild(0));
            node->addChild(divisionChild->getChild(1));
        }
    }

    sortASTExpression(node);
}

void sortASTExpression(ASTNode_t* node)
{
    // Sort the child nodes.
    for (int i=0; i<node->getNumChildren(); i++)
        sortASTExpression(node->getChild(i));

    // If this node is times or add sort the children by the number of their children.
    if (node->getType() == AST_TIMES || node->getType() == AST_PLUS)
    {
        // Get a reordered list of children.
        int currentCount=0;
        list<ASTNode_t*> children;
        while (children.size() < node->getNumChildren())
        {
            for (int i=0; i<node->getNumChildren(); i++)
            {
                if (node->getChild(i)->getNumChildren() == currentCount)
                    children.push_back(node->getChild(i));
            }
            currentCount++;
        }

        // Remove the children.
        while (node->getNumChildren()) node->removeChild(0);

        // Add the new list.
        for (list<ASTNode_t*>::iterator it=children.begin(); it != children.end(); it++)
            node->addChild(*it);
    }
}

void simplifyASTExpression(ASTNode_t* node, map<string,double>& parameterValues)
{
    // Simplify the child nodes.
    for (int i=0; i<node->getNumChildren(); i++)
        simplifyASTExpression(node->getChild(i), parameterValues);

    // If this is an operator and all children are numbers, evaluate it.
    if (node->isOperator() && areAllASTChildrenNumeric(node))
    {
        node->setValue(evaluateASTOperator(node));
        while (node->getNumChildren())
            node->removeChild(0);
    }
    if (node->isFunction() && areAllASTChildrenNumeric(node))
    {
        node->setValue(evaluateASTFunction(node));
        while (node->getNumChildren())
            node->removeChild(0);
    }
    if (node->getType() == AST_TIMES)
    {
        // If we are multiplying, combine all numeric values into a single value.
        ASTNode_t* numericChild = NULL;
        double value=1.0;
        for (uint i=0; i<node->getNumChildren(); i++)
        {
            if (node->getChild(i)->getType() == AST_INTEGER)
            {
                value *= (double)node->getChild(i)->getInteger();
                if (numericChild == NULL)
                    numericChild = node->getChild(i);
                else
                    node->removeChild(i--);
            }
            else if (node->getChild(i)->getType() == AST_REAL || node->getChild(i)->getType() == AST_REAL_E)
            {
                value *= node->getChild(i)->getReal();
                if (numericChild == NULL)
                    numericChild = node->getChild(i);
                else
                    node->removeChild(i--);
            }
        }
        if (numericChild != NULL)
        {
            numericChild->setValue(value);
        }
    }

    // Substitute any parameter values.
    if (node->getType() == AST_NAME)
    {
        if (parameterValues.count(node->getName()) == 1)
        {
            string name = node->getName();
            node->setValue(parameterValues[name]);
            //printf("substituting %s -> %0.4e\n",name.c_str(), node->getReal());
        }
    }

    if (node->getType() == AST_NAME_AVOGADRO)
    {
        node->setValue(6.02214179e23);
    }
}

bool areAllASTChildrenNumeric(ASTNode_t* node)
{
    for (int i=0; i<node->getNumChildren(); i++)
        if (node->getChild(i)->getType() != AST_INTEGER && node->getChild(i)->getType() != AST_REAL && node->getChild(i)->getType() != AST_REAL_E)
            return false;
    return true;
}

double evaluateASTOperator(const ASTNode_t * node)
{
    if (node->getType() == AST_TIMES)
    {
        double value=1.0;
        for (uint i=0; i<node->getNumChildren(); i++)
        {
            value *= evaluateASTOperator(node->getChild(i));
        }
        return value;
    }
    else if (node->getType() == AST_DIVIDE)
    {
        if (node->getNumChildren() != 2) throw Exception("Unsupported division operator format: ", SBML_formulaToL3String(node));
        return evaluateASTOperator(node->getChild(0))/evaluateASTOperator(node->getChild(1));
    }
    if (node->getType() == AST_PLUS)
    {
        double value=0.0;
        for (uint i=0; i<node->getNumChildren(); i++)
        {
            value += evaluateASTOperator(node->getChild(i));
        }
        return value;
    }
    else if (node->getType() == AST_MINUS)
    {
        if (node->getNumChildren() == 1) return -evaluateASTOperator(node->getChild(0));
        if (node->getNumChildren() == 2) return evaluateASTOperator(node->getChild(0)) - evaluateASTOperator(node->getChild(1));
        throw Exception("Unsupported subtraction operator format: ", SBML_formulaToL3String(node));
    }
    else if (node->getType() == AST_INTEGER)
    {
        return (double)node->getInteger();
    }
    else if (node->getType() == AST_REAL || node->getType() == AST_REAL_E)
    {
        return node->getReal();
    }
    else
        throw Exception("Unsupported operator type.", node->getType());
}

double evaluateASTFunction(const ASTNode_t * node)
{
    if (node->getType() == AST_FUNCTION_POWER)
    {
        if (node->getNumChildren() != 2) throw Exception("Unsupported function power format: ", SBML_formulaToL3String(node));
        return pow(evaluateASTOperator(node->getChild(0)),evaluateASTOperator(node->getChild(1)));
    }
    if (node->getType() == AST_FUNCTION_POWER)
    {
        if (node->getNumChildren() != 1) throw Exception("Unsupported function exp format: ", SBML_formulaToL3String(node));
        return exp(evaluateASTOperator(node->getChild(0)));
    }
    else
        throw Exception("Unsupported function type.", node->getType());
}

bool compareASTNodes(ASTNode_t* formula, ASTNode_t* propensityFormula)
{
    // If this is a number and it matches to a k, accept the match.
    if (formula->isNumber() && propensityFormula->isName() && propensityFormula->getName()[0] == 'k')
        return true;

    if (formula->isNumber() && propensityFormula->isNumber())
    {
	return evaluateASTOperator(formula) == evaluateASTOperator(propensityFormula);
    }

    if (formula->getType() == propensityFormula->getType() && formula->getNumChildren() == propensityFormula->getNumChildren())
    {
        for (int i=0; i<formula->getNumChildren(); i++)
            if (!compareASTNodes(formula->getChild(i), propensityFormula->getChild(i)))
                return false;
        return true;
    }
    return false;
}

bool createReactionModelEntry(ASTNode_t* formula, ASTNode_t* propensityFormula, uint reactionIndex, ndarray<double>& K, ndarray<uint>& D, map<string,uint>& speciesIndices)
{
    // If this is a number and it matches to a k, store the parameter.
    if (formula->isNumber() && propensityFormula->isName() && propensityFormula->getName()[0] == 'k')
    {
        uint parameterIndex = atoi(propensityFormula->getName()+1)-1;
        if (formula->getType() == AST_INTEGER)
        {
            K[utuple(reactionIndex,parameterIndex)] = (double)formula->getInteger();
            if (verbose) Print::printf(Print::INFO, "    Added parameter for reaction %d: %d = %e", reactionIndex, parameterIndex, K[utuple(reactionIndex,parameterIndex)]);
            return true;
        }
        else if (formula->getType() == AST_REAL || formula->getType() == AST_REAL_E)
        {
            K[utuple(reactionIndex,parameterIndex)] = formula->getReal();
            if (verbose) Print::printf(Print::INFO, "    Added parameter for reaction %d: %d = %e", reactionIndex, parameterIndex, K[utuple(reactionIndex,parameterIndex)]);
            return true;
        }
        Print::printf(Print::ERROR, "FAILED to create entry for reaction %d: the value did not match a known numeric type.", reactionIndex);
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

        // Add an entry in the D matrix.
        if (D[utuple(speciesIndices[formula->getName()], reactionIndex)] == 0)
        {
            D[utuple(speciesIndices[formula->getName()], reactionIndex)] = atoi(propensityFormula->getName()+1);
            if (verbose) Print::printf(Print::INFO, "    Added dependency for reaction %d on species %d (%s): %d", reactionIndex, speciesIndices[formula->getName()], formula->getName(), D[utuple(speciesIndices[formula->getName()], reactionIndex)]);
        }
        return true;
    }

    // Go through all of the children, if any failed return false.
    bool ret = true;
    for (int i=0; i<formula->getNumChildren(); i++)
        if (!createReactionModelEntry(formula->getChild(i), propensityFormula->getChild(i), reactionIndex, K, D, speciesIndices))
            ret = false;
    return ret;
}

/**
 * This function prints the copyright notice.
 */
void printCopyright(int argc, char** argv)
{
	std::cout << argv[0] << " v" << VERSION_NUM << " build " << BUILD_INFO << std::endl;
	std::cout << "Copyright (C) " << COPYRIGHT_DATE << " Luthey-Schulten Group, University of Illinois at Urbana-Champaign." << std::endl;
	std::cout << "Copyright (C) " << COPYRIGHT_DATE_JHU << " Roberts Group, Johns Hopkins University." << std::endl << std::endl;
	std::cout << std::endl;
}

/**
 * Parses the command line arguments.
 */
void parseArguments(int argc, char** argv)
{
    // Parse any arguments.
    for (int i=1; i<argc; i++)
    {
        char *option = argv[i];
        while (*option == ' ') option++;
        
        // See if the user is trying to get help.
        if (strcmp(option, "-h") == 0 || strcmp(option, "--help") == 0) {
        	function = "help";
        	break;
        }
        
        // See if the user is trying to get the version info.
        else if (strcmp(option, "-v") == 0 || strcmp(option, "--version") == 0) {
        	function = "version";
        	break;
        }

        // See if the user is trying to specify an output filename.
        else if (i == 1)
        {
        	function = "import";
    		outputFilename = option;
        }
        
        // See if the user is trying to specify an input filename
        else if (i == 2)
        {
            inputFilename = option;
        }


        // See if the user is trying to import a sbml file that was originally exported by Copasi
        else if (strcmp(option, "--ignore-errors") == 0)
        {
            ignoreErrors = true;
        }

        // See if the user wants verbose messages.
        else if (strcmp(option, "--verbose") == 0)
        {
            if (!verbose)
                verbose = true;
            else
                reallyVerbose = true;
        }

        // See if the user is trying to import a sbml file that was originally exported by Copasi
        else if (strcmp(option, "--copasi") == 0)
        {
            isCopasi = true;
        }

        // See if the user is trying to specify an expression rule key value pair.
        else if (strncmp(option, "--rule:", strlen("--rule:"))==0 && strstr(option, "=") != NULL)
        {
            option += strlen("--rule:");
            char * separator=strstr(option, "=");
            if (separator != NULL && separator > option && strlen(separator) > 1)
            {
                string key(option, separator-option);
                userRules[key] = string(separator+1);
            }
        }

        // See if the user is trying to specify a parameter key value pair.
        else if (strstr(option, "=") != NULL)
        {
            char * separator=strstr(option, "=");
            if (separator != NULL && separator > option && strlen(separator) > 1)
            {
                string key(option, separator-option);
                userParameters[key] = atof(separator+1);
            }
        }

        // This must be an invalid option.
        else {
            throw lm::CommandLineArgumentException(option);
        }
    }
}

/**
 * Prints the usage for the program.
 */
void printUsage(int argc, char** argv)
{
	std::cout << "Usage: " << argv[0] << " (-h|--help)" << std::endl;
	std::cout << "Usage: " << argv[0] << " (-v|--version)" << std::endl;
	std::cout << "Usage: " << argv[0] << " lm_filename sbml_filename [OPTIONS]" << std::endl; // TODO: uncomment rest of line when userParameterValues is implemented (see below) // (simulation_parameter_key=value)+" << std::endl;
	std::cout << std::endl;
    std::cout << "OPTIONS" << std::endl;
    std::cout << "  key=double_value            Specify a new or override an existing global parameter in the SBML file." << std::endl;
    std::cout << "  --rule:key=string_value     Specify a new or override an existing assignment rule in the SBML file." << std::endl;
    std::cout << "  --ignore-errors             Use this option to ignore any errors in the SBML file and attempt to import it." << std::endl;
    std::cout << "  --verbose                   This option causes more detailed error information to be printed." << std::endl;
    std::cout << "  --copasi                    (EXPERIMENTAL) Use this option if you're trying to import a sbml file that was originally exported by Copasi" << std::endl;
    std::cout << std::endl;
}

