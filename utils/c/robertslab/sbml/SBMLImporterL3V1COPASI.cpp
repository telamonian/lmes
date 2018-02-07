#include <sbml/conversion/ConversionProperties.h>
#include <sbml/math/FormulaFormatter.h>
#include <sbml/SBMLDocument.h>
#include <sbml/SBMLReader.h>
#include <sbml/SBMLTypes.h>
#include <sbml/xml/XMLErrorLog.h>

#include "lm/Print.h"
#include "robertslab/sbml/SBMLImporterL3V1.h"
#include "robertslab/sbml/SBMLImporterL3V1COPASI.h"


using lm::Print;

namespace robertslab {
namespace sbml {

SBMLImporterL3V1COPASI::SBMLImporterL3V1COPASI()
{
}

string SBMLImporterL3V1COPASI::getDescription()
{
    return "COPASI SBML L3V1 Importer";
}

ASTNode_t* SBMLImporterL3V1COPASI::filterKineticExpression(ASTNode_t* e)
{
    // If this expression start with a volume multiplier, remove it.
    if (e->getType() == AST_TIMES && e->getNumChildren() >= 2 && e->getChild(0)->getType() == AST_NAME && e->getChild(0)->getName() == compartments[0])
    {
        e->removeChild(0);
    }

    return e;
}

double SBMLImporterL3V1COPASI::convertSubstanceToParticles(double value, string units)
{
    // In COPASI, if the substance unit is specified as "mole", change it to the default.
    if (units == "mole") units = "";

    // Call the base converter.
    return SBMLImporterL3V1::convertSubstanceToParticles(value, units);
}


}
}
