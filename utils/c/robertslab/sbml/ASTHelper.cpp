#include "ASTHelper.h"

#include <list>
#include <map>
#include <string>
#include <sbml/SBMLTypes.h>

#include "lm/Exceptions.h"

using std::list;
using std::map;
using std::string;
using lm::Exception;


namespace robertslab {
namespace sbml {

void ASTHelper::printASTNode(const ASTNode_t* node, int depth)
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

bool ASTHelper::substituteASTExpression(ASTNode_t* node, map<string,ASTNode_t*>& globalExpressions)
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
void ASTHelper::normalizeASTExpression(ASTNode_t* node)
{
    // Normalize the child nodes.
    for (int i=0; i<node->getNumChildren(); i++)
        normalizeASTExpression(node->getChild(i));

    // If this node is FN_POWER, change it to just POWER.
    if (node->getType() == AST_FUNCTION_POWER)
    {
        node->setType(AST_POWER);
    }

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

void ASTHelper::sortASTExpression(ASTNode_t* node)
{
    // Sort the child nodes.
    for (int i=0; i<node->getNumChildren(); i++)
        sortASTExpression(node->getChild(i));

    // If this node is times or add sort the children.
    if (node->getType() == AST_TIMES || node->getType() == AST_PLUS)
    {
        list<ASTNode_t*> children;

        // Put numeric types first.
        for (int i=0; i<node->getNumChildren(); i++)
        {
            if (isNumeric(node->getChild(i)))
            {
                children.push_back(node->getChild(i));
                node->removeChild(i);
                i--;
            }
        }

        // Then named types.
        for (int i=0; i<node->getNumChildren(); i++)
        {
            if (node->getChild(i)->getType() == AST_NAME)
            {
                children.push_back(node->getChild(i));
                node->removeChild(i);
                i--;
            }
        }

        // Finally, sort by the number of children.
        int currentCount=0;
        while (node->getNumChildren() > 0)
        {
            for (int i=0; i<node->getNumChildren(); i++)
            {
                if (node->getChild(i)->getNumChildren() == currentCount)
                {
                    children.push_back(node->getChild(i));
                    node->removeChild(i);
                    i--;
                }
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

void ASTHelper::substituteASTParameters(ASTNode_t* node, map<string,double>& parameterValues)
{
    // Process the child nodes.
    for (int i=0; i<node->getNumChildren(); i++)
        substituteASTParameters(node->getChild(i), parameterValues);

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

void ASTHelper::simplifyASTExpression(ASTNode_t* node)
{
    // Simplify the child nodes.
    for (int i=0; i<node->getNumChildren(); i++)
        simplifyASTExpression(node->getChild(i));

    // If this is an operator and all children are numbers, evaluate it.
    if (node->isOperator() && node->getType() != AST_POWER && areAllASTChildrenNumeric(node))
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

    // If this node is a divide and the numerator is a times with one constant and the denominator is a constant, simplfy.
    if (node->getType() == AST_DIVIDE && node->getNumChildren() == 2 && node->getChild(0)->getType() == AST_TIMES && isNumeric(node->getChild(1)))
    {
        // See if one of the multiply children is numeric.
        int numericChild = -1;
        for (int i=0; i<node->getChild(0)->getNumChildren(); i++)
        {
            if (isNumeric(node->getChild(0)->getChild(i)))
            {
                numericChild = i;
                break;
            }
        }

        // If we found one, combine the two constant and switch the node to multiply.
        if (numericChild >= 0)
        {
            // Divide the numerator by the denominator
            double numerator = getNumericValue(node->getChild(0)->getChild(numericChild));
            double denominator = getNumericValue(node->getChild(1));
            node->getChild(0)->getChild(numericChild)->setType(AST_REAL);
            node->getChild(0)->getChild(numericChild)->setValue(numerator/denominator);

            // Get the times child.
            ASTNode_t* timesChild = node->getChild(0);

            // Remove both children.
            while (node->getNumChildren() > 0)
                node->removeChild(0);

            // Set the type to times.
            node->setType(timesChild->getType());

            // Add the times children.
            for (int i=0; i<timesChild->getNumChildren(); i++)
            {
                node->addChild(timesChild->getChild(i));
            }
        }
    }

    // If this node is multiplication, and it only has one operator child, remove the multiplication.
    if (node->getType() == AST_TIMES && node->getNumChildren() == 1 && node->getChild(0)->isOperator())
    {
        ASTNode_t* child = node->getChild(0);
        node->removeChild(0);
        for (int i=0; i<child->getNumChildren(); i++)
        {
            node->addChild(child->getChild(i));
        }
        node->setType(child->getType());
    }
}

bool ASTHelper::areAllASTChildrenNumeric(ASTNode_t* node)
{
    for (int i=0; i<node->getNumChildren(); i++)
        if (node->getChild(i)->getType() != AST_INTEGER && node->getChild(i)->getType() != AST_REAL && node->getChild(i)->getType() != AST_REAL_E)
            return false;
    return true;
}

bool ASTHelper::isNumeric(ASTNode_t* node)
{
    return (node->getType() == AST_INTEGER || node->getType() == AST_REAL || node->getType() == AST_REAL_E);
}

double ASTHelper::getNumericValue(ASTNode_t* node)
{
    if (node->getType() == AST_INTEGER)
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

double ASTHelper::evaluateASTOperator(const ASTNode_t * node)
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
    else if (node->getType() == AST_POWER)
    {
        if (node->getNumChildren() != 2) throw Exception("Unsupported power operator format: ", SBML_formulaToL3String(node));
        return pow(evaluateASTOperator(node->getChild(0)), evaluateASTOperator(node->getChild(1)));
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

double ASTHelper::evaluateASTFunction(const ASTNode_t * node)
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

bool ASTHelper::compareASTNodes(ASTNode_t* formula, ASTNode_t* propensityFormula)
{
    map<string,string> variableMap, variableMapReverse;
    return compareASTNodes(formula, propensityFormula, variableMap, variableMapReverse);
}

bool ASTHelper::compareASTNodes(ASTNode_t* formula, ASTNode_t* propensityFormula, map<string,string>& variableMap, map<string,string>& variableMapReverse)
{
    // If this is a number and it matches to a k, accept the match.
    if (formula->isNumber() && propensityFormula->isName() && propensityFormula->getName()[0] == 'k')
        return true;

    if (formula->isNumber() && propensityFormula->isNumber())
    {
        return evaluateASTOperator(formula) == evaluateASTOperator(propensityFormula);
    }

    // If this is a variable, make sure it matches with any previous uses.
    if (formula->isName() && propensityFormula->isName())
    {
        if (variableMap.count(formula->getName()) == 0 && variableMapReverse.count(propensityFormula->getName()) == 0)
        {
            variableMap[formula->getName()] = propensityFormula->getName();
            variableMapReverse[propensityFormula->getName()] = formula->getName();
            return true;
        }
        else if (variableMap.count(formula->getName()) > 0 && variableMap[formula->getName()] == propensityFormula->getName() && variableMapReverse.count(propensityFormula->getName()) > 0 && variableMapReverse[propensityFormula->getName()] == formula->getName())
        {
            return true;
        }
        return false;
    }

    if (formula->getType() == propensityFormula->getType() && formula->getNumChildren() == propensityFormula->getNumChildren())
    {
        for (int i=0; i<formula->getNumChildren(); i++)
            if (!compareASTNodes(formula->getChild(i), propensityFormula->getChild(i), variableMap, variableMapReverse))
                return false;
        return true;
    }
    return false;
}

}
}
