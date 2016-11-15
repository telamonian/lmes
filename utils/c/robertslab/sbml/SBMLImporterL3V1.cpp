
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
#include "lm/io/ReactionModel.pb.h"
#include "robertslab/sbml/SBMLImporterL3V1.h"

using std::list;
using lm::Exception;
using lm::Print;

namespace robertslab {
namespace sbml {

SBMLImporterL3V1::SBMLImporterL3V1(SBMLDocument* sbmlDocument, bool stopOnError, map<string,double> userParameters, map<string,string> userExpressions)
:sbmlDocument(sbmlDocument),sbmlModel(NULL),stopOnError(stopOnError),verbose(true),userParameters(userParameters),userExpressions(userExpressions)
{
    sbmlModel = sbmlDocument->getModel();
}

void SBMLImporterL3V1::import()
{
    Print::printf(Print::INFO, "%s processing document.", getDescription().c_str());
    expandFunctionDefinitions();
    importGlobalParameters();
    importGlobalExpressions();
    importCompartments();
    importSpecies();
}

string SBMLImporterL3V1::getDescription()
{
    return "SBML L3V1 Importer";
}

void SBMLImporterL3V1::expandFunctionDefinitions()
{
    // expand any user-defined functions in the reaction kinetic laws
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
        normalizeASTExpression(node);
        simplifyASTExpression(node, globalParameters);
        if (verbose) Print::printf(Print::INFO, "Simplified assignment rule %s: %s", id.c_str(), SBML_formulaToL3String(globalExpressions[id]));
    }
}

void SBMLImporterL3V1::importCompartments()
{
    if (sbmlModel->getNumCompartments())
    {
        Print::printf(Print::INFO, "Processing %d compartments.", sbmlModel->getNumCompartments());
        for (int i=0; i<sbmlModel->getNumCompartments(); i++)
        {
            if (sbmlModel->getCompartment(i)->getSpatialDimensions() == 3)
            {
                string compartmentId = sbmlModel->getCompartment(i)->getId();
                compartmentSizes[compartmentId] = convertVolumeToLiters(sbmlModel->getCompartment(i)->getSize(), sbmlModel->getCompartment(i)->getUnits());
                Print::printf(Print::INFO, "Added compartment (%d) %s: %e L", i, compartmentId.c_str(), compartmentSizes[compartmentId]);
            }
            else
            {
                throw Exception("Unsupported compartment dimensions", sbmlModel->getCompartment(i)->getSpatialDimensions());
            }
        }
    }
}

void SBMLImporterL3V1::importSpecies()
{
    // Process the species.
    int numberSpecies = sbmlModel->getNumSpecies();
    printf("%d\n",numberSpecies);
    reactionModel.set_number_species(numberSpecies);


    Print::printf(Print::INFO, "Processing %d species.", numberSpecies);
    for (int i=0; i<numberSpecies; i++)
    {
        Species* species = sbmlModel->getSpecies(i);
        speciesIndex[species->getId()] = i;

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
            initialSpeciesCount=convertSubstanceToParticles(species->getInitialAmount(), species->getSubstanceUnits());
        }
        else if (species->isSetInitialConcentration())
        {
            initialSpeciesCount=convertSubstanceToParticles(species->getInitialConcentration()*compartmentSizes[species->getCompartment()], species->getSubstanceUnits());
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

int SBMLImporterL3V1::convertSubstanceToParticles(double value, string units)
{
    // If we didn't get any units, use the default volume units.
    if (units == "") units = sbmlModel->getSubstanceUnits();

    // See if the units are in liters.
    if (units == "item")
    {
        return lround(value);
    }
    else if (units == "mole")
    {
        return lround(value*6.02214085774e23);
    }

    // Otherwise, see if the units have a match in the unit definitions.
    else if (sbmlModel->getUnitDefinition(units) != NULL)
    {
        // Get the definition for the units.
        UnitDefinition* unitDef = sbmlModel->getUnitDefinition(units);
        printf("%e %e %e\n",value,unitDef->getUnit(0)->getMultiplier(),pow(10,unitDef->getUnit(0)->getScale()));
        if (unitDef->getNumUnits() == 1 && unitDef->getUnit(0)->getKind() == UNIT_KIND_ITEM && unitDef->getUnit(0)->getExponent() == 1)
            return lround(value*unitDef->getUnit(0)->getMultiplier()*pow(10,unitDef->getUnit(0)->getScale()));
        else if (unitDef->getNumUnits() == 1 && unitDef->getUnit(0)->getKind() == UNIT_KIND_MOLE && unitDef->getUnit(0)->getExponent() == 1)
            return lround(value*unitDef->getUnit(0)->getMultiplier()*pow(10,unitDef->getUnit(0)->getScale())*6.02214085774e23);
        else
            throw Exception("Unsupported substance unit definition", unitDef->toSBML());
    }

    // Otherwise, throw an exception.
    else
    {
        throw Exception("Unsupported substance units", units.c_str());
    }
}










void SBMLImporterL3V1::normalizeASTExpression(ASTNode_t* node)
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

void SBMLImporterL3V1::sortASTExpression(ASTNode_t* node)
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

void SBMLImporterL3V1::simplifyASTExpression(ASTNode_t* node, map<string,double>& parameterValues)
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
        for (int i=0; i<node->getNumChildren(); i++)
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

bool SBMLImporterL3V1::areAllASTChildrenNumeric(ASTNode_t* node)
{
    for (int i=0; i<node->getNumChildren(); i++)
        if (node->getChild(i)->getType() != AST_INTEGER && node->getChild(i)->getType() != AST_REAL && node->getChild(i)->getType() != AST_REAL_E)
            return false;
    return true;
}

double SBMLImporterL3V1::evaluateASTOperator(const ASTNode_t * node)
{
    if (node->getType() == AST_TIMES)
    {
        double value=1.0;
        for (int i=0; i<node->getNumChildren(); i++)
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
        for (int i=0; i<node->getNumChildren(); i++)
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

double SBMLImporterL3V1::evaluateASTFunction(const ASTNode_t * node)
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

bool SBMLImporterL3V1::compareASTNodes(ASTNode_t* formula, ASTNode_t* propensityFormula)
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

}
}
