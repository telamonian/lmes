
#include <list>
#include <sbml/conversion/ConversionProperties.h>
#include <sbml/math/FormulaFormatter.h>
#include <sbml/SBMLDocument.h>
#include <sbml/SBMLReader.h>
#include <sbml/SBMLTypes.h>
#include <sbml/UnitDefinition.h>
#include <sbml/xml/XMLErrorLog.h>

#include "lm/Exceptions.h"
#include "lm/Print.h"
#include "lm/Types.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/me/PropensityFunction.h"
#include "robertslab/sbml/ASTHelper.h"
#include "robertslab/sbml/SBMLImporterL3V1.h"

using std::list;
using lm::Exception;
using lm::Print;
using robertslab::sbml::ASTHelper;

namespace robertslab {
namespace sbml {

SBMLImporterL3V1::SBMLImporterL3V1()
:propensityFunctions(NULL),sbmlDocument(NULL),sbmlModel(NULL),constantsUseConcentrations(false),verbose(false),reallyVerbose(false),allImportStepsSuccessful(true),numberSpecies(0),numberReactions(0),S(NULL),T(NULL),K(NULL),D(NULL)
{
    propensityFunctions = new lm::me::PropensityFunctionFactory();
}

SBMLImporterL3V1::~SBMLImporterL3V1()
{
    if (propensityFunctions != NULL) delete propensityFunctions; propensityFunctions = NULL;
    if (S != NULL) delete S; S = NULL;
    if (T != NULL) delete T; T = NULL;
    if (K != NULL) delete K; K = NULL;
    if (D != NULL) delete D; D = NULL;
}

void SBMLImporterL3V1::setOptions(bool constantsUseConcentrations, bool verbose, bool reallyVerbose, bool ignoreErrors, bool ignoreUnmatchedReactions)
{
    this->constantsUseConcentrations = constantsUseConcentrations;
    this->verbose = verbose;
    this->reallyVerbose = reallyVerbose;
    this->stopOnError = !ignoreErrors;
    this->stopOnUnmatchedReactions = !ignoreUnmatchedReactions;
}

bool SBMLImporterL3V1::import(SBMLDocument* sbmlDocument, map<string,double> userParameters, map<string,string> userExpressions)
{
    // Initialize the importer.
    this->sbmlDocument = sbmlDocument;
    sbmlModel = sbmlDocument->getModel();
    this->userParameters = userParameters;
    this->userExpressions = userExpressions;
    allImportStepsSuccessful = true;

    if (verbose)
    {
        Print::printf(Print::INFO,"-----------------------------------");
        propensityFunctions->printRegisteredFunctions(Print::INFO);
        Print::printf(Print::INFO,"-----------------------------------");
    }

    Print::printf(Print::INFO, "%s processing document.", getDescription().c_str());
    expandFunctionDefinitions();
    importGlobalParameters();
    importGlobalExpressions();
    importCompartments();
    importSpecies();
    importReactions();
    return allImportStepsSuccessful;
}

lm::io::ReactionModel* SBMLImporterL3V1::getReactionModel()
{
    return &reactionModel;
}

string SBMLImporterL3V1::getDescription()
{
    return "SBML L3V1 Importer";
}

void SBMLImporterL3V1::expandFunctionDefinitions()
{
    // Expand any user-defined functions in the reaction kinetic laws.
    ConversionProperties props;
    props.addOption("expandFunctionDefinitions");

    if (sbmlDocument->convert(props) != LIBSBML_OPERATION_SUCCESS)
    {
        Print::printf(Print::ERROR,"Problems detected while expanding function definitions in the SBML file.\n");
        Print::printf(Print::ERROR,"-----------------------------------");
        sbmlDocument->printErrors(std::cout);
        Print::printf(Print::ERROR,"-----------------------------------");

        if (stopOnError) throw Exception("There were critical errors detected while expanding function definitions in the SBML file. Either fix the errors or execute the command again with the --ignore-errors flag set.");
    }
}

void SBMLImporterL3V1::importGlobalParameters()
{
    // Process any global parameters.
    if (sbmlModel->getNumParameters())
    {
        Print::printf(Print::INFO, "Processing %d parameters.", sbmlModel->getNumParameters());
        for (int i=0; i<sbmlModel->getNumParameters(); i++)
        {
            if (sbmlModel->getParameter(i)->getConstant())
            {
                globalParameters[sbmlModel->getParameter(i)->getId()] = sbmlModel->getParameter(i)->getValue();
                Print::printf(Print::INFO, "Added parameter (%d) %s: %e (constant)", i, sbmlModel->getParameter(i)->getId().c_str(), sbmlModel->getParameter(i)->getValue());
            }
            else
            {
                throw Exception("Unsupported non-constant global parameter", sbmlModel->getParameter(i)->toSBML());
                //Print::printf(Print::INFO, "Added parameter (%d) %s: %s", i, sbmlModel->getParameter(i)->getId().c_str(), SBML_formulaToL3String(globalExpressions[sbmlModel->getParameter(i)->getId()]));
            }
        }
    }

    // Substitute any global parameters with user specified parameters.
    for (map<string,double>::iterator it=userParameters.begin(); it!=userParameters.end(); it++)
    {
        if (globalParameters.count(it->first) == 0)
        {
            globalParameters[it->first] = it->second;
            Print::printf(Print::INFO, "Added user defined parameter %s: %e", it->first.c_str(), globalParameters[it->first]);
        }
        else
        {
            globalParameters[it->first] = it->second;
            Print::printf(Print::INFO, "Overriding parameter with user assignment %s: %e", it->first.c_str(), globalParameters[it->first]);
        }
    }
}

void SBMLImporterL3V1::importGlobalExpressions()
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


    // Try to simplify any global expressions.
    for (map<string,ASTNode_t*>::iterator it=globalExpressions.begin(); it!=globalExpressions.end(); it++)
    {
        string id = it->first;
        ASTNode_t* node = it->second;
        ASTHelper::normalizeASTExpression(node);
        ASTHelper::simplifyASTExpression(node, globalParameters);
        if (verbose) Print::printf(Print::INFO, "Simplified assignment rule %s: %s", id.c_str(), SBML_formulaToL3String(globalExpressions[id]));
    }
}

void SBMLImporterL3V1::importCompartments()
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

void SBMLImporterL3V1::importSpecies()
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
        if (species->getBoundaryCondition()) throw Exception("Unsupported species property", "boundaryCondition must be false");

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
        if (isSpeciesConst[i])
            Print::printf(Print::INFO, "Added species (%d) %s with initial count: %d (constant)", i, species->getId().c_str(), initialSpeciesCount);
        else
            Print::printf(Print::INFO, "Added species (%d) %s with initial count: %d", i, species->getId().c_str(), initialSpeciesCount);
    }
}


void SBMLImporterL3V1::importReactions()
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

            // If the species is not constant, set the S matrix entry.
            if (!isSpeciesConst[speciesIndex])
            {
                // Make sure we can process the reactant.
                if (!reactant->isSetStoichiometry()) throw Exception("Unsupported reaction property", "stoichiometry for reactants must be set");
                if (!reactant->getConstant()) throw Exception("Unsupported reaction property", "stoichiometry for reactants must be constant");

                // Make the proper entry in the S matrix.
                (*S)[utuple(speciesIndex,i)] -= reactant->getStoichiometry();
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
                (*S)[utuple(speciesIndex,i)] += product->getStoichiometry();
            }
            else if (verbose)
            {
                Print::printf(Print::INFO, "Skipping entry in S matrix for reaction %d and constant species %d.", i, speciesIndex);
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
        lm::io::ReactionModel_Reaction* reaction = reactionModel.add_reaction();
        reaction->set_type((*T)[utuple(j)]);
        for (int k=0; k<10 && !isnan((*K)[utuple(j,k)]); k++)
            reaction->add_rate_constant((*K)[utuple(j,k)]);
    }
}

bool SBMLImporterL3V1::importKinetics(Reaction* reaction, int reactionIndex, KineticLaw* kinetics)
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

ASTNode_t* SBMLImporterL3V1::filterKineticExpression(ASTNode_t* expression)
{
    return expression;
}

bool SBMLImporterL3V1::importPropensityFunction(Reaction* reaction, int reactionIndex, KineticLaw* kinetics, map<string,double>& parameterValues)
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
        ASTHelper::printASTNode(substitutedFormula);
    }

    // Put the formula into normal form.
    ASTNode_t* normalizedFormula = substitutedFormula->deepCopy();
    ASTHelper::normalizeASTExpression(normalizedFormula);
    if (reallyVerbose)
    {
        printf("normalized: %s\n", SBML_formulaToL3String(normalizedFormula));
        ASTHelper::printASTNode(normalizedFormula);
    }

    // Simplify the formula by substituting parameters.
    ASTNode_t* simplifiedFormula = normalizedFormula->deepCopy();
    ASTHelper::simplifyASTExpression(simplifiedFormula, parameterValues);
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
                if (ASTHelper::compareASTNodes(simplifiedFormula, normalizedPropensityFormula))
                {
                    (*T)[utuple(reactionIndex)] = id;
                    Print::printf(Print::INFO, "Matched kinetic formula in reaction %s (%d) to %s: [%s] == [%s]", reaction->getId().c_str(), reactionIndex, p.name.c_str(), SBML_formulaToL3String(simplifiedFormula), SBML_formulaToL3String(normalizedPropensityFormula));
                    Print::printf(Print::DEBUG, "                                         Original form:   [%s]", SBML_formulaToL3String(kinetics->getMath()));
                    Print::printf(Print::DEBUG, "                                         Normalized form: [%s]", SBML_formulaToL3String(normalizedFormula));

                    // Create the entry for this formula.
                    return createPropensityFunctionEntry(reactionIndex, simplifiedFormula, normalizedPropensityFormula, p);
                }
                else
                {
                    if (verbose)
                    {
                        Print::printf(Print::INFO, "No match [%s] to [%s]: %s", SBML_formulaToL3String(simplifiedFormula), SBML_formulaToL3String(normalizedPropensityFormula), p.name.c_str());
                        if (reallyVerbose)
                        {
                            ASTHelper::printASTNode(simplifiedFormula);
                            ASTHelper::printASTNode(normalizedPropensityFormula);
                        }
                    }
                }
            }
        }
    }

    // Print out some messages to help the user figure out why there wasn't a match.
    Print::printf(Print::ERROR, "FAILED to match kinetic formula in reaction %s (%d) at line %d to a propensity function: [%s] ", reaction->getId().c_str(), reactionIndex, kinetics->getLine(), SBML_formulaToL3String(simplifiedFormula));
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

bool SBMLImporterL3V1::createPropensityFunctionEntry(int reactionIndex, ASTNode_t* formula, ASTNode_t* propensityFormula, lm::me::PropensityFunctionDefinition& propensityFunction)
{
    // If this is a number and it matches to a k, store the parameter.
    if (formula->isNumber() && propensityFormula->isName() && propensityFormula->getName()[0] == 'k')
    {
        uint parameterIndex = atoi(propensityFormula->getName()+1)-1;
        if (formula->getType() == AST_INTEGER)
        {
            (*K)[utuple(reactionIndex,parameterIndex)] = convertPropensityConstantUnits(propensityFormula->getName(), (double)formula->getInteger(), propensityFunction.getConstantUnits(parameterIndex));
            if (verbose) Print::printf(Print::INFO, "    Added parameter for reaction %d: %d = %e", reactionIndex, parameterIndex, (*K)[utuple(reactionIndex,parameterIndex)]);
            return true;
        }
        else if (formula->getType() == AST_REAL || formula->getType() == AST_REAL_E)
        {
            (*K)[utuple(reactionIndex,parameterIndex)] = convertPropensityConstantUnits(propensityFormula->getName(), formula->getReal(), propensityFunction.getConstantUnits(parameterIndex));
            if (verbose) Print::printf(Print::INFO, "    Added parameter for reaction %d: %d = %e", reactionIndex, parameterIndex, (*K)[utuple(reactionIndex,parameterIndex)]);
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
        if ((*D)[utuple(speciesIndices[formula->getName()], reactionIndex)] == 0)
        {
            (*D)[utuple(speciesIndices[formula->getName()], reactionIndex)] = atoi(propensityFormula->getName()+1);
            if (verbose) Print::printf(Print::INFO, "    Added dependency for reaction %d on species %d (%s): %d", reactionIndex, speciesIndices[formula->getName()], formula->getName(), (*D)[utuple(speciesIndices[formula->getName()], reactionIndex)]);
        }
        return true;
    }

    // Go through all of the children, if any failed return false.
    bool ret = true;
    for (int i=0; i<formula->getNumChildren(); i++)
        if (!createPropensityFunctionEntry(reactionIndex,formula->getChild(i), propensityFormula->getChild(i), propensityFunction))
            ret = false;
    return ret;
}

double SBMLImporterL3V1::convertPropensityConstantUnits(string constantName, double value, string desiredUnits)
{
    ASTNode_t* units = SBML_parseL3Formula(desiredUnits.c_str());
    convertUnits(units);
    double conversion = ASTHelper::evaluateASTOperator(units);
    if (conversion != 1.0) Print::printf(Print::INFO, "Converted kinetic rate constant %s from %e to %e %s", constantName.c_str(), value, value*conversion, desiredUnits.c_str());
    return value*conversion;
}

void SBMLImporterL3V1::convertUnits(ASTNode_t* units)
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



double SBMLImporterL3V1::convertVolumeToLiters(double value, string units)
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

double SBMLImporterL3V1::convertSubstanceToParticles(double value, string units)
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

double SBMLImporterL3V1::convertTimeToSeconds(double value, string units)
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

}
}
