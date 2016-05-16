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
 * Copasi always sets species concentrations (instead of amount) in exported sbml, so fix that
 */
bool isCopasi = false;

/**
 * Parameters specified by the user.
 */
map<string, double> userParameterValues;

void importSBMLModel(Hdf5File * lmFile, string sbmlFilename) throw(Exception);
void importSBMLModelL3V1(ReactionModel * lmModel, Model * sbmlModel) throw(Exception);
void importSBMLModelL3V1Kinetics(Reaction * reaction, uint reactionIndex, KineticLaw * kinetics, ReactionModel * lmModel, uint * D, map<string,uint> & speciesIndices, uint numberReactions, vector<string> & globalParameters, map<string,double> & globalParameterValues) throw(Exception);
int matchKineticsWithPropensityFunction(Reaction * reaction, uint reactionIndex, KineticLaw * kinetics, map<string,double>& parameterValues);
bool isZerothOrderReaction(const ASTNode * root, vector<string> & parameters, map<string,uint> & speciesIndices);
void importZerothOrderReaction(const ASTNode * root, vector<string> & parameters, map<string,double> & parameterValues, uint reactionIndex, uint numberReactions, ReactionModel * lmModel, uint * D, map<string,uint> & speciesIndices);
bool isFirstOrderReaction(const ASTNode * root, vector<string> & parameters, map<string,uint> & speciesIndices);
void importFirstOrderReaction(const ASTNode * root, vector<string> & parameters, map<string,double> & parameterValues, uint reactionIndex, uint numberReactions, ReactionModel * lmModel, uint * D, map<string,uint> & speciesIndices);
bool isSecondOrderReaction(const ASTNode * root, vector<string> & parameters, map<string,uint> & speciesIndices);
void importSecondOrderReaction(const ASTNode * root, vector<string> & parameters, map<string,double> & parameterValues, uint reactionIndex, uint numberReactions, ReactionModel * lmModel, uint * D, map<string,uint> & speciesIndices);
bool isSecondOrderSelfReaction(const ASTNode * root, vector<string> & parameters, map<string,uint> & speciesIndices);
void importSecondOrderSelfReaction(const ASTNode * root, vector<string> & parameters, map<string,double> & parameterValues, uint reactionIndex, uint numberReactions, ReactionModel * lmModel, uint * D, map<string,uint> & speciesIndices);
void importUnsupportedReaction(const ASTNode * root, vector<string> & parameters, map<string,double> & parameterValues, uint reactionIndex, uint numberReactions, ReactionModel * lmModel, uint * D, map<string,uint> & speciesIndices);

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
		    printf("Done.\n");
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
    factory->printRegisteredFunctions(Print::INFO);

    // Read in the SBML document.
    SBMLReader reader;
    std::auto_ptr<SBMLDocument> sbmlDocument(reader.readSBML(sbmlFilename));
    if (sbmlDocument->getNumErrors() > 0)
    {
        printf("\nProblems detected while parsing the SBML file %s\n",sbmlFilename.c_str());
        printf("-----------------------------------\n");
        sbmlDocument->printErrors(std::cout);
        printf("-----------------------------------\n");

        bool criticalErrrors = false;
        for (int i=0; i<sbmlDocument->getNumErrors(); i++)
        {
            const SBMLError* error = sbmlDocument->getError(i);
            if (error->getSeverity() >= LIBSBML_SEV_ERROR)
                criticalErrrors = true;
        }

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
            printf("\nProblems detected while expanding function definitions in the SBML file %s\n",sbmlFilename.c_str());
            printf("-----------------------------------\n");
            sbmlDocument->printErrors(std::cout);
            printf("-----------------------------------\n");

            if (!ignoreErrors)
                throw Exception("There were critical errors detected while expanding function definitions in the SBML file. Either fix the errors or execute the command again with the --ignore-errors flag set.");
        }

        // Build the reaction model from the SBML model.
        ReactionModel lmModel;
        Model * sbmlModel = sbmlDocument->getModel();
        importSBMLModelL3V1(&lmModel, sbmlModel);
        lmFile->setReactionModel(&lmModel);
    }
    else
        throw Exception("Unsupported SBML format", sbmlDocument->getLevel(), sbmlDocument->getVersion());
}

void importSBMLModelL3V1(ReactionModel * lmModel, Model * sbmlModel) throw(Exception)
{
    vector<string> globalParameters;
    map<string,double> globalParameterValues;

    // Get the units for the model.
    string modelSubstanceUnits = sbmlModel->getSubstanceUnits();

    // Process any global parameters.
    for (uint i=0; i<sbmlModel->getNumParameters(); i++)
    {
        globalParameters.push_back(sbmlModel->getParameter(i)->getId());
        globalParameterValues[sbmlModel->getParameter(i)->getId()] = sbmlModel->getParameter(i)->getValue();
    }

    // Process the compartments.
    for (uint i=0; i<sbmlModel->getNumCompartments(); i++)
    {
    	globalParameters.push_back(sbmlModel->getCompartment(i)->getId());
    	globalParameterValues[sbmlModel->getCompartment(i)->getId()] = sbmlModel->getCompartment(i)->getSize();
    }

    // Process the species.
    printf("Setting initial species counts:\n");

    map<string,uint> speciesIndices;
    uint numberSpecies = sbmlModel->getNumSpecies();
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
        // Make sure we can process the species.
        if (species->getConstant()) throw Exception("Unsupported species property", "constant must be false");
        if (species->isSetConversionFactor()) throw Exception("Unsupported species property", "conversionFactor must not be set");

        // Get the initial count for the species.
        if (species->isSetInitialAmount())
        {
            lmModel->add_initial_species_count((uint)lround(species->getInitialAmount()));
        }
        else if (species->isSetInitialConcentration())
        {
            if (isCopasi)
            {
                uint initialConcentration = (uint)lround(sbmlModel->getCompartment(0)->getVolume() * species->getInitialConcentration());
                printf("%s -> %d\n", species->getName().c_str(), initialConcentration);
                lmModel->add_initial_species_count(initialConcentration);
            }
            else
            {
                throw Exception("Unsupported species property", "initialConcentration must not be set");
            }
        }
        else
            throw Exception("Unsupported species property", "initialAmount or initialConcentration must be set");
    }
    printf("\n");

    // Process the reactions.
    printf("Setting rate laws:\n");

    uint numberReactions = sbmlModel->getNumReactions();
    lmModel->set_number_reactions(numberReactions);
    int * S = new int[numberSpecies*numberReactions];
    uint * D = new uint[numberSpecies*numberReactions];
    for (uint i=0; i<numberSpecies*numberReactions; i++)
    {
        S[i] = 0;
        D[i] = 0;
    }
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

            // Make sure we can process the reactant.
            if (!reactant->isSetStoichiometry()) throw Exception("Unsupported reaction property", "stoichiometry for reactants must be set");
            if (!reactant->getConstant()) throw Exception("Unsupported reaction property", "stoichiometry for reactants must be constant");

            // Make the proper entry in the S matrix.
            S[speciesIndex*numberReactions+i] -= reactant->getStoichiometry();
        }

        // Go through the list of products.
        for (uint j=0; j<reaction->getNumProducts(); j++)
        {
            SpeciesReference * product = reaction->getProduct(j);
            uint speciesIndex = speciesIndices[product->getSpecies()];

            // Make sure we can process the product.
            if (!product->isSetStoichiometry()) throw Exception("Unsupported reaction property", "stoichiometry for products must be set");
            if (!product->getConstant()) throw Exception("Unsupported reaction property", "stoichiometry for products must be constant");

            // Make the proper entry in the S matrix.
            S[speciesIndex*numberReactions+i] += product->getStoichiometry();
        }

        // Process the kinetic law.
        lmModel->add_reaction();
        KineticLaw * kinetics = reaction->getKineticLaw();
        importSBMLModelL3V1Kinetics(reaction, i, kinetics, lmModel, D, speciesIndices, numberReactions, globalParameters, globalParameterValues);
    }

    // Fill in the S and D matrices.
    for (uint i=0; i<numberSpecies*numberReactions; i++)
    {
        lmModel->add_stoichiometric_matrix(S[i]);
        lmModel->add_dependency_matrix(D[i]);
    }
}

void importSBMLModelL3V1Kinetics(Reaction * reaction, uint reactionIndex, KineticLaw * kinetics, ReactionModel * lmModel, uint * D, map<string,uint> & speciesIndices, uint numberReactions, vector<string> & globalParameters, map<string,double> & globalParameterValues) throw(Exception)
{
    vector<string> localParameters;
    map<string,double> localParameterValues;

	// Bring the global parameters into the local scope.
    for (vector<string>::iterator it = globalParameters.begin(); it != globalParameters.end(); it++)
    {
        localParameters.push_back(*it);
        localParameterValues[*it] = globalParameterValues[*it];
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
            localParameters.push_back(localParameter->getId());
            localParameterValues[localParameter->getId()] = localParameter->getValue();
        //}
    }

    // Go through all of the propensity functions and see if we can find a match.
    int propensityFunctionId = matchKineticsWithPropensityFunction(reaction, reactionIndex, kinetics, localParameterValues);

    if (propensityFunctionId != -1)
    {
    }



/*
    string reactionTypeString;
    char formattedOutput[100];

    sprintf(formattedOutput, "%s -> %s", kinetics->getParentSBMLObject()->getName().c_str(), SBML_formulaToString(math));
    printf("%-100s", formattedOutput);
    if (isZerothOrderReaction(math, localParameters, speciesIndices))
    {
        importZerothOrderReaction(math, localParameters, localParameterValues, reactionIndex, numberReactions, lmModel, D, speciesIndices);
        reactionTypeString = "ZEROTH ORDER";
    }
    else if (isFirstOrderReaction(math, localParameters, speciesIndices))
    {
        importFirstOrderReaction(math, localParameters, localParameterValues, reactionIndex, numberReactions, lmModel, D, speciesIndices);
        reactionTypeString = "FIRST ORDER";
    }
    else if (isSecondOrderReaction(math, localParameters, speciesIndices))
    {
        importSecondOrderReaction(math, localParameters, localParameterValues, reactionIndex, numberReactions, lmModel, D, speciesIndices);
        reactionTypeString = "SECOND ORDER";
    }
    else if (isSecondOrderSelfReaction(math, localParameters, speciesIndices))
    {
        importSecondOrderSelfReaction(math, localParameters, localParameterValues, reactionIndex, numberReactions, lmModel, D, speciesIndices);
        reactionTypeString = "SECOND ORDER SELF";
    }
    else
        if (isCopasi)
        {
            importUnsupportedReaction(math, localParameters, localParameterValues, reactionIndex, numberReactions, lmModel, D, speciesIndices);
            reactionTypeString = "UNSUPPORTED KINETIC LAW";
        }
        else
        {
            throw Exception("Unsupported kinetic law", SBML_formulaToString(math));
        }
    printf("[%s]\n", reactionTypeString.c_str());
    */
}

void printASTNode(const ASTNode_t* node, int depth=0);
void normalizeASTExpression(ASTNode_t* node);
void simplifyASTExpression(ASTNode_t* node, map<string,double>& parameterValues);
bool areAllASTChildrenNumeric(ASTNode_t* node);
double evaluateASTOperator(const ASTNode_t * node);
bool compareASTNodes(ASTNode_t* formula, ASTNode_t* propensityFormula);

int matchKineticsWithPropensityFunction(Reaction * reaction, uint reactionIndex, KineticLaw * kinetics, map<string,double>& parameterValues)
{
    // Get the kinetic expression.
    const ASTNode_t* originalFormula = SBML_parseL3Formula(SBML_formulaToL3String(kinetics->getMath()));
//    printf("original: %s\n", SBML_formulaToL3String(kinetics->getMath()));
//    printf("formula: %s\n", SBML_formulaToL3String(originalFormula));
//    printASTNode(originalFormula);

    // Put the formula into normal form.
    ASTNode_t* normalizedFormula = originalFormula->deepCopy();
    normalizeASTExpression(normalizedFormula);
//    printf("normalized: %s\n", SBML_formulaToL3String(normalizedFormula));
//    printASTNode(normalizedFormula);

    // Simplify the formula by substituting parameters.
    ASTNode_t* simplifiedFormula = normalizedFormula->deepCopy();
    simplifyASTExpression(simplifiedFormula, parameterValues);
//    printf("simplified: %s\n", SBML_formulaToL3String(simplifiedFormula));
//    printASTNode(simplifiedFormula);



    // Iterate through each propensity function and see if it matches.
    map<uint,lm::me::PropensityFunctionDefinition> functions = factory->getFunctions();
    for (std::map<uint,lm::me::PropensityFunctionDefinition>::const_iterator it=functions.begin(); it != functions.end(); it++)
    {
        uint id = it->first;
        lm::me::PropensityFunctionDefinition p = it->second;
        if (p.expression.length() > 0)
        {
            ASTNode_t* propensityFormula = SBML_parseL3Formula(p.expression.c_str());
            ASTNode_t* normalizedPropensityFormula = propensityFormula->deepCopy();
            normalizeASTExpression(normalizedPropensityFormula);
            if (compareASTNodes(simplifiedFormula, normalizedPropensityFormula))
            {
                Print::printf(Print::INFO, "Matched kinetic formula in %s to %s: [%s] == [%s]", reaction->getName().c_str(), p.name.c_str(), SBML_formulaToL3String(simplifiedFormula), SBML_formulaToL3String(normalizedPropensityFormula));
                Print::printf(Print::DEBUG, "                                         Original form:   [%s]", SBML_formulaToL3String(kinetics->getMath()));
                Print::printf(Print::DEBUG, "                                         Normalized form: [%s]", SBML_formulaToL3String(normalizedFormula));
                return id;
            }
            else
            {
                Print::printf(Print::DEBUG, "No match [%s] to [%s]: %s", SBML_formulaToL3String(simplifiedFormula), SBML_formulaToL3String(normalizedPropensityFormula), p.name.c_str());
            }
        }
    }

    // Print out some messages to help the user figure out why there wasn't a match.
    Print::printf(Print::ERROR, "FAILED to match kinetic formula in %s at line %d to a propensity function: [%s] ", reaction->getName().c_str(), kinetics->getLine(), SBML_formulaToL3String(kinetics->getMath()));
    Print::printf(Print::ERROR, "                                         Normalized form: [%s]", SBML_formulaToL3String(normalizedFormula));
    Print::printf(Print::ERROR, "                                         Simplified form: [%s]", SBML_formulaToL3String(simplifiedFormula));

    delete simplifiedFormula;
    return -1;
}

void printASTNode(const ASTNode_t* node, int depth)
{
    if (depth == 0) printf("------------------------\n");
    for (int i=0; i<depth; i++) printf("  ");
    if (node->getType() == AST_NAME)
        printf("%s\n", node->getName());
    else if (node->getType() == AST_TIMES)
        printf("%s\n", "TIMES");
    else if (node->getType() == AST_DIVIDE)
        printf("%s\n", "DIVIDE");
    else if (node->getType() == AST_PLUS)
        printf("%s\n", "PLUS");
    else if (node->getType() == AST_MINUS)
        printf("%s\n", "MINUS");
    else if (node->getType() == AST_INTEGER)
        printf("%ld\n", node->getInteger());
    else if (node->getType() == AST_REAL)
        printf("%0.4e\n", node->getReal());
    else
        printf("AST Type:%d\n", node->getType());
    for (int i=0; i<node->getNumChildren(); i++)
        printASTNode(node->getChild(i), depth+1);
    if (depth == 0) printf("------------------------\n");
}

void normalizeASTExpression(ASTNode_t* node)
{
    // Normalize the child nodes.
    for (int i=0; i<node->getNumChildren(); i++)
        normalizeASTExpression(node->getChild(i));

    if (node->getType() == AST_TIMES)
    {
        // If a child is times, remove it and bring its children up a level.
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
    if (node->getType() == AST_TIMES)
    {
        // If a child is divide and its first child is times, merge the child values.
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
            while (node->getNumChildren())
                node->removeChild(0);

            // Set ourselves as the division node, with the original divsion nodes' two child lsits.
            node->setType(AST_DIVIDE);
            node->addChild(divisionChild->getChild(0));
            node->addChild(divisionChild->getChild(1));
        }
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
            else if (node->getChild(i)->getType() == AST_REAL)
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
        if (node->getChild(i)->getType() != AST_INTEGER && node->getChild(i)->getType() != AST_REAL)
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
        if (node->getNumChildren() != 2) throw Exception("Unsupported division operator format.");
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
        if (node->getNumChildren() != 2) throw Exception("Unsupported subtraction operator format.");
        return evaluateASTOperator(node->getChild(0)) - evaluateASTOperator(node->getChild(1));
    }
    else if (node->getType() == AST_INTEGER)
    {
        return (double)node->getInteger();
    }
    else if (node->getType() == AST_REAL)
    {
        return node->getReal();
    }
    else
        throw Exception("Unsupported operator type.", node->getType());
}

bool compareASTNodes(ASTNode_t* formula, ASTNode_t* propensityFormula)
{
    // If this is a number and it matches to a k, accept the match.
    if (formula->isNumber() && propensityFormula->isName() && propensityFormula->getName()[0] == 'k')
        return true;

    if (formula->getType() == propensityFormula->getType() && formula->getNumChildren() == propensityFormula->getNumChildren())
    {
        for (int i=0; i<formula->getNumChildren(); i++)
            if (!compareASTNodes(formula->getChild(i), propensityFormula->getChild(i)))
                return false;
        return true;
    }
    return false;
}

void getSpeciesUsedInExpression(vector<string> & speciesUsed, const ASTNode * node, vector<string> & parameters, map<string,uint> & speciesIndices)
{
    if (node->getType() == AST_NAME)
    {
        string name(node->getName());
        bool isParameter = false;
        for (vector<string>::iterator it = parameters.begin(); it != parameters.end(); it++)
        {
            if (*it == name)
            {
                isParameter = true;
                break;
            }
        }
        if (!isParameter)
        {
        	if (speciesIndices.find(name) != speciesIndices.end())
        		speciesUsed.push_back(name);
        	else
        		throw Exception("Unknown identifier in expression", name.c_str());
        }
    }
    for (uint i=0; i<node->getNumChildren(); i++)
    {
        getSpeciesUsedInExpression(speciesUsed, node->getChild(i), parameters, speciesIndices);
    }
}

void getOperatorsUsedInExpression(vector<string> & speciesUsed, const ASTNode * node)
{
    if (node->isOperator())
    {
        speciesUsed.push_back(string(1,node->getCharacter()));
    }
    else if (node->isName())
    {
    }
    else if (node->isNumber())
    {
    }
    else
    {
        speciesUsed.push_back(string("?"));
    }
    for (uint i=0; i<node->getNumChildren(); i++)
    {
        getOperatorsUsedInExpression(speciesUsed, node->getChild(i));
    }
}

const ASTNode * getFirstExpressionOfType(const ASTNode * node, ASTNodeType_t type)
{
    if (node->getType() == type) return node;
    for (uint i=0; i<node->getNumChildren(); i++)
    {
        const ASTNode *tmp=getFirstExpressionOfType(node->getChild(i), type);
        if (tmp != NULL) return tmp;
    }
    return NULL;
}


/*
bool isZerothOrderReaction(const ASTNode * root, vector<string> & parameters, map<string,uint> & speciesIndices)
{
    // Make sure the rate only depends on only one species.
    vector<string> speciesUsed;
    getSpeciesUsedInExpression(speciesUsed, root, parameters, speciesIndices);

    if (speciesUsed.size() != 0) return false;

    // Make sure the expression only involves multiplication and division.
    vector<string> operatorsUsed;
    getOperatorsUsedInExpression(operatorsUsed, root);
    for (vector<string>::iterator it = operatorsUsed.begin(); it != operatorsUsed.end(); it++)
    {
        if (*it != "*" && *it != "/") return false;
    }

    return true;
}

void importZerothOrderReaction(const ASTNode * root, vector<string> & parameters, map<string,double> & parameterValues, uint reactionIndex, uint numberReactions, ReactionModel * lmModel, uint * D, map<string,uint> & speciesIndices)
{
    // Set the reaction type.
    lmModel->mutable_reaction(reactionIndex)->set_type(0);

    // Get the rate constant.
    double k=calculateMultiplierInExpression(root, parameterValues, speciesIndices);
    lmModel->mutable_reaction(reactionIndex)->add_rate_constant(k);
}

bool isFirstOrderReaction(const ASTNode * root, vector<string> & parameters, map<string,uint> & speciesIndices)
{
    // Make sure the rate only depends on only one species.
    vector<string> speciesUsed;
    getSpeciesUsedInExpression(speciesUsed, root, parameters, speciesIndices);
    if (speciesUsed.size() != 1) return false;

    // Make sure the expression only involves multiplication and division.
    vector<string> operatorsUsed;
    getOperatorsUsedInExpression(operatorsUsed, root);
    for (vector<string>::iterator it = operatorsUsed.begin(); it != operatorsUsed.end(); it++)
    {
        if (*it != "*" && *it != "/") return false;
    }

    return true;
}

void importFirstOrderReaction(const ASTNode * root, vector<string> & parameters, map<string,double> & parameterValues, uint reactionIndex, uint numberReactions, ReactionModel * lmModel, uint * D, map<string,uint> & speciesIndices)
{
    // Get the index of the species.
    vector<string> speciesUsed;
    getSpeciesUsedInExpression(speciesUsed, root, parameters, speciesIndices);
    uint speciesIndex = speciesIndices[speciesUsed[0]];

    // Set the reaction type.
    lmModel->mutable_reaction(reactionIndex)->set_type(1);

    // Set the reaction dependency.
    D[speciesIndex*numberReactions+reactionIndex] = 1;

    // Get the rate constant.
    double k=calculateMultiplierInExpression(root, parameterValues, speciesIndices);
    lmModel->mutable_reaction(reactionIndex)->add_rate_constant(k);
}

bool isSecondOrderReaction(const ASTNode * root, vector<string> & parameters, map<string,uint> & speciesIndices)
{
    // Make sure the rate only depends on only two species.
    vector<string> speciesUsed;
    getSpeciesUsedInExpression(speciesUsed, root, parameters, speciesIndices);

    if (speciesUsed.size() != 2) return false;
    if (speciesUsed[0] == speciesUsed[1]) return false;

    // Make sure the expression only involves multiplication and division.
    vector<string> operatorsUsed;
    getOperatorsUsedInExpression(operatorsUsed, root);
    for (vector<string>::iterator it = operatorsUsed.begin(); it != operatorsUsed.end(); it++)
    {
        if (*it != "*" && *it != "/") return false;
    }

    return true;
}

void importSecondOrderReaction(const ASTNode * root, vector<string> & parameters, map<string,double> & parameterValues, uint reactionIndex, uint numberReactions, ReactionModel * lmModel, uint * D, map<string,uint> & speciesIndices)
{
    // Get the species used.
    vector<string> speciesUsed;
    getSpeciesUsedInExpression(speciesUsed, root, parameters, speciesIndices);

    // Set the reaction type.
    lmModel->mutable_reaction(reactionIndex)->set_type(2);

    // Set the reaction dependency.
    D[speciesIndices[speciesUsed[0]]*numberReactions+reactionIndex] = 1;
    D[speciesIndices[speciesUsed[1]]*numberReactions+reactionIndex] = 1;

    // Get the rate constant.
    double k=calculateMultiplierInExpression(root, parameterValues, speciesIndices);
    lmModel->mutable_reaction(reactionIndex)->add_rate_constant(k);
}

bool isSecondOrderSelfReaction(const ASTNode * root, vector<string> & parameters, map<string,uint> & speciesIndices)
{
    // Make sure the rate only depends on only two species.
    vector<string> speciesUsed;
    getSpeciesUsedInExpression(speciesUsed, root, parameters, speciesIndices);
    if (speciesUsed.size() != 2) return false;
    if (speciesUsed[0] != speciesUsed[1]) return false;

    // Make sure the expression only involves multiplication and division.
    vector<string> operatorsUsed;
    getOperatorsUsedInExpression(operatorsUsed, root);
    uint numMinuses=0;
    for (vector<string>::iterator it = operatorsUsed.begin(); it != operatorsUsed.end(); it++)
    {
        if (*it != "*" && *it != "/" && *it != "-")
        {
        	Print::printf(Print::VERBOSE_DEBUG, "Reaction was not second order self due to presence of a %s operator.", *it->c_str());
        	return false;
        }
        if (*it == "-") numMinuses++;
    }
    if (numMinuses != 1)
    {
    	Print::printf(Print::VERBOSE_DEBUG, "Reaction was not second order self due to presence of %d minus operators.", numMinuses);
    	return false;
    }

    // Make sure one of the species entries is species-1.
    const ASTNode * minusNode=getFirstExpressionOfType(root, AST_MINUS);
    if (minusNode == NULL)
    {
    	Print::printf(Print::VERBOSE_DEBUG, "Reaction was not second order self due to lack of minus operator.");
    	return false;
    }
    if (minusNode->getNumChildren() != 2)
    {
    	Print::printf(Print::VERBOSE_DEBUG, "Reaction was not second order self due to %d children for the minus node.", minusNode->getNumChildren());
    	return false;
    }
    if (!minusNode->getChild(0)->isName())
    {
    	Print::printf(Print::VERBOSE_DEBUG, "Reaction was not second order self due to the first minus child not being a symbol.");
    	return false;
    }
    if (string(minusNode->getChild(0)->getName()) != speciesUsed[0])
    {
    	Print::printf(Print::VERBOSE_DEBUG, "Reaction was not second order self due to the first child being an invalid symbol: %s.", minusNode->getChild(0)->getName());
    	return false;
    }
    if (!(minusNode->getChild(1)->isInteger() || minusNode->getChild(1)->isReal()))
    {
    	Print::printf(Print::VERBOSE_DEBUG, "Reaction was not second order self due to the second child being neither an integer nor a real.");
    	return false;
    }
    if (minusNode->getChild(1)->getInteger() != 1 && minusNode->getChild(1)->getReal() != 1)
    {
    	Print::printf(Print::VERBOSE_DEBUG, "Reaction was not second order self due to the second child being unequal to 1 - integer: %d real: %f.", minusNode->getChild(1)->getInteger(), minusNode->getChild(1)->getReal());
    	return false;
    }

    return true;
}

void importSecondOrderSelfReaction(const ASTNode * root, vector<string> & parameters, map<string,double> & parameterValues, uint reactionIndex, uint numberReactions, ReactionModel * lmModel, uint * D, map<string,uint> & speciesIndices)
{
    // Get the species used.
    vector<string> speciesUsed;
    getSpeciesUsedInExpression(speciesUsed, root, parameters, speciesIndices);

    // Set the reaction type.
    lmModel->mutable_reaction(reactionIndex)->set_type(3);

    // Set the reaction dependency.
    D[speciesIndices[speciesUsed[0]]*numberReactions+reactionIndex] = 1;

    // Get the rate constant.
    double k=calculateMultiplierInExpression(root, parameterValues, speciesIndices, true);
    lmModel->mutable_reaction(reactionIndex)->add_rate_constant(k);
}

void importUnsupportedReaction(const ASTNode * root, vector<string> & parameters, map<string,double> & parameterValues, uint reactionIndex, uint numberReactions, ReactionModel * lmModel, uint * D, map<string,uint> & speciesIndices)
{
    // Set the reaction type to 9999 to mark that it needs to be manually updated by the user
    lmModel->mutable_reaction(reactionIndex)->set_type(9999);

    // Set all possible dependencies to 9999 to mark that it needs to be manually updated by the user
    for (map<string,uint>::const_iterator it=speciesIndices.begin(); it!=speciesIndices.end(); it++)
    {
        D[(it->second)*numberReactions+reactionIndex]=9999;
    }
}
*/

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

        // See if the user is trying to import a sbml file that was originally exported by Copasi
        else if (strcmp(option, "--copasi") == 0)
        {
            isCopasi = true;
        }

        // See if the user is trying to specify a key value pair.
        else if (strstr(option, "=") != NULL)
        {
            char * separator=strstr(option, "=");
            if (separator != NULL && separator > option && strlen(separator) > 1)
            {
                string key(option, separator-option);
                double value = atof(separator+1);
                userParameterValues[key] = value;
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
    std::cout << "  --ignore-errors     Use this option to ignore any errors in the SBML file and attempt to import it." << std::endl;
    std::cout << "  --copasi            (EXPERIMENTAL) Use this option if you're trying to import a sbml file that was originally exported by Copasi" << std::endl;
    std::cout << std::endl;
    // TODO: the userParameterValues stuff seems to be currently unimplemented, uncomment help section below once it is
//    std::cout << "Setting simulation parameters" << std::endl;
//    std::cout << "    This program can also be used to set simulation parameters on your newly imported model. You can do this by adding \"simulation_parameter_key=value\" pairs to the end of your arguments." << std::endl;
//    std::cout << "        EXAMPLE: " << argv[0] << " genetic_toggle_switch.lm genetic_toggle_swithc.sbml writeInterval=1e-2 maxTime=1e-1" << std::endl;
//    std::cout << std::endl;
}


/**
 * double evaluateASTExpression(const ASTNode_t * node, map<string,double> & parameterValues, map<string,uint> & speciesIndices, bool ignoreSpeciesMinusOne=false)
{
    if (node->getType() == AST_TIMES)
    {
        double value=1.0;
        for (uint i=0; i<node->getNumChildren(); i++)
        {
            value *= calculateMultiplierInExpression(node->getChild(i), parameterValues, speciesIndices, ignoreSpeciesMinusOne);
        }
        return value;
    }
    else if (node->getType() == AST_DIVIDE && node->getNumChildren() == 2)
    {
        return calculateMultiplierInExpression(node->getChild(0), parameterValues, speciesIndices, ignoreSpeciesMinusOne)/calculateMultiplierInExpression(node->getChild(1), parameterValues, speciesIndices, ignoreSpeciesMinusOne);
    }
    else if (ignoreSpeciesMinusOne && node->getType() == AST_MINUS)
    {

        if (node->getNumChildren() != 2) throw Exception("Unsupported expression 1");
        if (!node->getChild(0)->isName()) throw Exception("Unsupported expression 2");
        if (parameterValues.count(node->getChild(0)->getName()) == 1) throw Exception("Unsupported expression 3");
        if (!(node->getChild(1)->isInteger() || node->getChild(1)->isReal())) throw Exception("Unsupported expression 4");
        if (node->getChild(1)->getInteger() != 1 && node->getChild(1)->getReal() != 1) throw Exception("Unsupported expression 5");
        return 1.0;
    }
    else if (node->getType() == AST_INTEGER)
    {
        return (double)node->getInteger();
    }
    else if (node->getType() == AST_REAL)
    {
        return node->getReal();
    }
    else if (node->getType() == AST_NAME)
    {
        if (parameterValues.count(node->getName()) == 1)
            return parameterValues[node->getName()];
        else if (speciesIndices.find(string(node->getName())) != speciesIndices.end())
            return 1.0;
        else
            throw Exception("Unknown identifier in expression", node->getName());
    }
    else if (node->getType() == AST_NAME_AVOGADRO)
    {
        return 6.02214179e23;
    }
    else
        throw Exception("Unsupported ast type", node->getType());
}
*/
