/*
 * Copyright 2017 Johns Hopkins University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
 *
 * Author(s): Elijah Roberts
 */

#ifndef ROBERTSLAB_BNGL_BNGLIMPORTER_H
#define ROBERTSLAB_BNGL_BNGLIMPORTER_H

#include <map>
#include <string>
#include <vector>

#include "lm/Types.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/me/PropensityFunction.h"
#include "robertslab/bngl/InstanceDefinitions.h"
#include "robertslab/bngl/TypeDefinitions.h"

using std::map;
using std::string;
using std::vector;

namespace robertslab {
namespace bngl {

class BNGLImporter
{
public:
    BNGLImporter();
    virtual ~BNGLImporter();
    virtual string getDescription();
    virtual void setOptions(bool constantsUseConcentrations, bool verbose, bool reallyVerbose);
    virtual bool import(string filename, map<string,double> userParameters);
    virtual lm::input::ReactionModel* getReactionModel();

protected:
    virtual void parseParameters(list<string>& lines);
    virtual void parseMoleculeTypes(list<string>& lines);
    virtual void parseInitialCounts(list<string>& lines);
    virtual void parseReactions(list<string>& lines);
    /*virtual void importMoleculeTypes();
    virtual void importSeedSpecies();
    virtual void importReactionRules();*/

protected:
    /*virtual double convertPropensityConstantUnits(string constant, double value, string desiredUnits);
    virtual void convertUnits(ASTNode_t* units);
    virtual double convertVolumeToLiters(double size, string units="");
    virtual double convertSubstanceToParticles(double value, string units="");
    virtual double convertTimeToSeconds(double value, string units="");*/

protected:
    lm::me::PropensityFunctionFactory *propensityFunctions;
    bool constantsUseConcentrations;
    bool verbose, reallyVerbose;
    bool allImportStepsSuccessful;
    map<string,double> parameters;
    map<string,MoleculeClass> moleculeClasses;
    lm::input::ReactionModel reactionModel;

    int numberSpecies;
    map<string,int> speciesIndices;

    /*
    int numberReactions;
    ndarray<int> *S;
    ndarray<int> *T;
    ndarray<double> *K;
    ndarray<int> *D;
    */
};

}
}

#endif // BNGLImporter_H
