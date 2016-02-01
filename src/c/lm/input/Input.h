/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the Software), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimers.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimers in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the names of the Roberts Group, Johns Hopkins University,
 * nor the names of its contributors may be used to endorse or
 * promote products derived from this Software without specific prior written
 * permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS WITH THE SOFTWARE.
 *
 * Author(s): Elijah Roberts, Max Klein
 */
#ifndef LM_INPUT_INPUT_H
#define LM_INPUT_INPUT_H

#include <map>
#include <string>

#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/BoundaryConditions.pb.h"
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/OrderParameters.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SimulationParameters.pb.h"
#include "lm/oparam/OParams.h"
#include "lm/tiling/Tilings.h"

using std::map;
using std::string;

namespace lm {
namespace input {

struct Input
{
public:
    Input(const lm::io::hdf5::Hdf5File& file);
    virtual ~Input();

    bool hasReactionModel() {return reactionModelPresent;}
    bool hasDiffusionModel() {return diffusionModelPresent;}
    bool hasOrderParameters() {return orderParametersPresent;}
    bool hasTilings() {return tilingsPresent;}

    const map<string,string>& getSimulationParameters() {return simulationParameters;}
    const lm::io::ReactionModel& getReactionModel() {return reactionModel;}
    const lm::io::DiffusionModel& getDiffusionModel() {return diffusionModel;}
    const lm::oparam::OParams& getOrderParameters() {return orderParameters;}
    const lm::tiling::Tilings& getTilings() {return tilings;}


protected:
    bool parseBoundaryConditions(lm::io::BoundaryConditions* bc, std::string arg);

protected:
    bool reactionModelPresent;
    bool diffusionModelPresent;
    bool orderParametersPresent;
    bool tilingsPresent;

    lm::io::SimulationParameters* simulationParametersMsg;
    map<string,string> simulationParameters;
    lm::io::ReactionModel reactionModel;
    lm::io::DiffusionModel diffusionModel;
    lm::io::OrderParameters orderParametersMsg;
    lm::oparam::OParams orderParameters;
    lm::io::Tilings tilingsMsg;
    lm::tiling::Tilings tilings;
};

//class Input
//{
//public:
//    Input();
//    Input(lm::io::hdf5::Hdf5File& file);
//    virtual ~Input();
//
//    // has methods
//
//    // get protobuf methods
//
//    // get proto methods (load-into-pointer style)
//    virtual void getBoundaryGradient(lm::io::BoundaryConditions* bc);
//    virtual void getDiffusionModel(lm::io::DiffusionModel* diffusionModel);
//    virtual void getParameters(lm::io::SimulationParameters* parameters);
//    virtual void getOrderParameters(lm::io::OrderParameters* orderParameters);
//    virtual void getReactionModel(lm::io::ReactionModel* reactionModel);
//    virtual void getSpatialModel(lm::io::SpatialModel* model);
//    virtual void getTilings(lm::io::Tilings* tilings);
//
//    // get wrapper methods
//    virtual map<string,string> getParameters();
//    virtual string getParameter(string key, string defaultValue="");
//    virtual lm::oparam::OParam* getOrderParameter(uint id);
//    virtual lm::tiling::Tiling* getTiling(uint id)
//
//    // set protobuf methods
//    virtual void getBoundaryGradient(lm::io::BoundaryConditions* bc);
//    virtual void setDiffusionModel(lm::io::DiffusionModel& diffusionModel);
//    virtual void setOrderParameters(lm::io::OrderParameters& orderParameters);
//    virtual void setParameters(lm::io::SimulationParameters& parameters);
//    virtual void setReactionModel(lm::io::ReactionModel& reactionModel);
//    virtual void setSpatialModel(lm::io::SpatialModel& model);
//    virtual void setTilings(lm::io::Tilings& tilings);
//
//    // set wrapper methods
//    virtual void setParameter(string key, string value);
//
//protected:
//    // load from file methods
//    virtual void _loadBoundaryGradient(lm::io::BoundaryConditions* bc);
//    virtual void _loadDiffusionModel(lm::io::DiffusionModel* diffusionModel);
//    virtual void _loadParameters(lm::io::SimulationParameters* parameters);
//    virtual void _loadOrderParameters(lm::io::OrderParameters* orderParameters);
//    virtual void _loadReactionModel(lm::io::ReactionModel* reactionModel);
//    virtual void _loadSpatialModel(lm::io::SpatialModel* model);
//    virtual void _loadTilings(lm::io::Tilings* tilings);
//
//private:
//    lm::io::hdf5::Hdf5File& file;
//    lm::message::Message msg;
//};

}
}
#endif // LM_INPUT_INPUT_H
