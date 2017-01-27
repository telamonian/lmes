#ifndef SBMLIMPORTERL3V1COPASI_H
#define SBMLIMPORTERL3V1COPASI_H

#include <sbml/SBMLDocument.h>
#include "robertslab/sbml/SBMLImporterL3V1.h"

namespace robertslab {
namespace sbml {

class SBMLImporterL3V1COPASI : public SBMLImporterL3V1
{
public:
    SBMLImporterL3V1COPASI();

protected:
    virtual string getDescription();
    virtual ASTNode_t* filterKineticExpression(ASTNode_t* expression);
    virtual double convertSubstanceToParticles(double value, string units);
};

}
}

#endif // SBMLIMPORTERL3V1COPASI_H
