/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
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
#include "lm/io/SpatialModel.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/oparam/OParams.h"
#include "lm/option/SimulationParameters.h"
#include "lm/tiling/Tilings.h"

using std::map;
using std::string;

namespace lm {
namespace input {

struct Input
{
public:
    Input();
    Input(bool hasDiffusionModel,bool hasOrderParameters,bool hasReactionModel,bool hasTilings,lm::io::DiffusionModel& diffusionModelBuf,lm::io::OrderParameters& orderParametersBuf,lm::oparam::OParams& oparams,lm::io::ReactionModel& reactionModelBuf, lm::option::SimulationParameters& simulationParameters, lm::io::Tilings& tilingsBuf,lm::tiling::Tilings& tilings):
        hasDiffusionModel(hasDiffusionModel),
        hasOrderParameters(hasOrderParameters),
        hasReactionModel(hasReactionModel),
        hasTilings(hasTilings),
        diffusionModelBuf(diffusionModelBuf),
        orderParametersBuf(orderParametersBuf),
        oparams(oparams),
        reactionModelBuf(reactionModelBuf),
        simulationParameters(simulationParameters),
        tilingsBuf(tilingsBuf),
        tilings(tilings) {}

    bool hasDiffusionModel;
    bool hasOrderParameters;
    bool hasReactionModel;
    bool hasTilings;
    lm::io::DiffusionModel& diffusionModelBuf;
    lm::io::OrderParameters& orderParametersBuf;
    lm::oparam::OParams& oparams;
    lm::io::ReactionModel& reactionModelBuf;
    lm::option::SimulationParameters& simulationParameters;
    lm::io::Tilings& tilingsBuf;
    lm::tiling::Tilings& tilings;
};

//class Input
//{
//public:
//    Input();
//    Input(lm::io::hdf5::Hdf5File& file);
//    virtual ~Input();
//
//    // has methods
//    virtual bool hasBoundaryGradient();
//    virtual bool hasDiffusionModel();
//    virtual bool hasOrderParameters();
//    virtual bool hasReactionModel();
//    virtual bool hasTilings();
//
//    // get protobuf methods
//    virtual lm::io::BoundaryConditions* getBoundaryGradientBuf();
//    virtual lm::io::DiffusionModel* getDiffusionModelBuf();
//    virtual lm::io::SimulationParameters* getParametersBuf();
//    virtual lm::io::OrderParameters* getOrderParametersBuf();
//    virtual lm::io::ReactionModel* getReactionModelBuf();
//    virtual lm::io::SpatialModel* getSpatialModelBuf();
//    virtual lm::io::Tilings* getTilingsBuf();
//
//    // get protobuf methods (load-into-pointer style)
//    virtual void getBoundaryGradientBuf(lm::io::BoundaryConditions* bcBuf);
//    virtual void getDiffusionModelBuf(lm::io::DiffusionModel* diffusionModelBuf);
//    virtual void getParametersBuf(lm::io::SimulationParameters* parametersBuf);
//    virtual void getOrderParametersBuf(lm::io::OrderParameters* orderParametersBuf);
//    virtual void getReactionModelBuf(lm::io::ReactionModel* reactionModelBuf);
//    virtual void getSpatialModelBuf(lm::io::SpatialModel* modelBuf);
//    virtual void getTilingsBuf(lm::io::Tilings* tilingsBuf);
//
//    // get wrapper methods
//    virtual map<string,string> getParameters();
//    virtual string getParameter(string key, string defaultValue="");
//    virtual lm::oparam::OParam* getOrderParameter(uint id);
//    virtual lm::tiling::Tiling* getTiling(uint id)
//
//    // set protobuf methods
//    virtual void getBoundaryGradientBuf(lm::io::BoundaryConditions* bcBuf);
//    virtual void setDiffusionModelBuf(lm::io::DiffusionModel& diffusionModelBuf);
//    virtual void setOrderParametersBuf(lm::io::OrderParameters& orderParametersBuf);
//    virtual void setParametersBuf(lm::io::SimulationParameters& parametersBuf);
//    virtual void setReactionModelBuf(lm::io::ReactionModel& reactionModelBuf);
//    virtual void setSpatialModelBuf(lm::io::SpatialModel& modelBuf);
//    virtual void setTilingsBuf(lm::io::Tilings& tilingsBuf);
//
//    // set wrapper methods
//    virtual void setParameter(string key, string value);
//
//protected:
//    // load from file methods
//    virtual void _loadBoundaryGradientBuf(lm::io::BoundaryConditions* bc);
//    virtual void _loadDiffusionModelBuf(lm::io::DiffusionModel* diffusionModel);
//    virtual void _loadParametersBuf(lm::io::SimulationParameters* parameters);
//    virtual void _loadOrderParametersBuf(lm::io::OrderParameters* orderParameters);
//    virtual void _loadReactionModelBuf(lm::io::ReactionModel* reactionModel);
//    virtual void _loadSpatialModelBuf(lm::io::SpatialModel* model);
//    virtual void _loadTilingsBuf(lm::io::Tilings* tilings);
//
//private:
//    lm::io::hdf5::Hdf5File& file;
//    lm::message::Message msgBuf;
//};

}
}
#endif // LM_INPUT_INPUT_H
