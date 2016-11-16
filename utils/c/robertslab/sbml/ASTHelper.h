#ifndef ASTHELPER_H
#define ASTHELPER_H

#include <map>
#include <string>
#include <sbml/SBMLTypes.h>

using std::map;
using std::string;

namespace robertslab {
namespace sbml {

class ASTHelper
{
public:
    static void printASTNode(const ASTNode_t* node, int depth=0);
    static bool substituteASTExpression(ASTNode_t* node, map<string,ASTNode_t*>& globalExpressions);
    static void normalizeASTExpression(ASTNode_t* node);
    static void sortASTExpression(ASTNode_t* node);
    static void simplifyASTExpression(ASTNode_t* node, map<string,double>& parameterValues);
    static bool areAllASTChildrenNumeric(ASTNode_t* node);
    static double evaluateASTOperator(const ASTNode_t * node);
    static double evaluateASTFunction(const ASTNode_t * node);
    static bool compareASTNodes(ASTNode_t* formula, ASTNode_t* propensityFormula);
};

}
}

#endif // ASTHELPER_H
