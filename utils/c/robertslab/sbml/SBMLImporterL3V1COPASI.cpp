#include <sbml/conversion/ConversionProperties.h>
#include <sbml/math/FormulaFormatter.h>
#include <sbml/SBMLDocument.h>
#include <sbml/SBMLReader.h>
#include <sbml/SBMLTypes.h>
#include <sbml/xml/XMLErrorLog.h>

#include "lm/Print.h"
#include "robertslab/sbml/SBMLImporterL3V1.h"
#include "robertslab/sbml/SBMLImporterL3V1Copasi.h"


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

}
}
