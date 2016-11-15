#ifndef SBMLIMPORTERL3V1_H
#define SBMLIMPORTERL3V1_H

#include <map>
#include <string>
#include <sbml/SBMLDocument.h>

#include "lm/io/ReactionModel.pb.h"

using std::map;
using std::string;

namespace robertslab {
namespace sbml {

class SBMLImporterL3V1
{
public:
    SBMLImporterL3V1(SBMLDocument* document, bool stopOnError, map<string,double> userParameters, map<string,string> userExpressions);
    virtual void import();

protected:
    virtual string getDescription();
    virtual void expandFunctionDefinitions();
    virtual void importGlobalExpressions();
    virtual void importGlobalParameters();
    virtual void importCompartments();
    virtual void importSpecies();

protected:
    virtual double convertVolumeToLiters(double size, string units);
    virtual int convertSubstanceToParticles(double value, string units);

protected:
    virtual void normalizeASTExpression(ASTNode_t* node);
    virtual void sortASTExpression(ASTNode_t* node);
    virtual void simplifyASTExpression(ASTNode_t* node, map<string,double>& parameterValues);
    virtual bool areAllASTChildrenNumeric(ASTNode_t* node);
    virtual double evaluateASTOperator(const ASTNode_t * node);
    virtual double evaluateASTFunction(const ASTNode_t * node);
    virtual bool compareASTNodes(ASTNode_t* formula, ASTNode_t* propensityFormula);

protected:
    SBMLDocument* sbmlDocument;
    Model* sbmlModel;
    bool stopOnError;
    bool verbose;
    map<string,double> userParameters;
    map<string,string> userExpressions;
    map<string,double> globalParameters;
    map<string,ASTNode_t*> globalExpressions;
    map<string,double> compartmentSizes;
    lm::io::ReactionModel reactionModel;
    map<string,int> speciesIndex;
    map<int,bool> isSpeciesConst;
};

}
}

#endif // SBMLIMPORTERL3V1_H
