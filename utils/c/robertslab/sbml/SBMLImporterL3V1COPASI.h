#ifndef SBMLIMPORTERL3V1COPASI_H
#define SBMLIMPORTERL3V1COPASI_H

#include <sbml/SBMLDocument.h>
#include "robertslab/sbml/SBMLImporterL3V1.h"

namespace robertslab {
namespace sbml {

class SBMLImporterL3V1COPASI : public SBMLImporterL3V1
{
public:
    SBMLImporterL3V1COPASI(SBMLDocument* document, bool stopOnError, map<string,double> userParameters, map<string,string> userExpressions);

protected:
    virtual string getDescription();
    //void processCompartments();
};

}
}

#endif // SBMLIMPORTERL3V1COPASI_H
