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

#include <list>
#include <map>
#include <string>

#include "lm/EnumHelper.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/input/BoundaryConditions.pb.h"
#include "lm/input/DiffusionModel.pb.h"
#include "lm/input/OrderParameters.pb.h"
#include "lm/input/OutputOptions.pb.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/input/SimulationParameters.pb.h"
#include "lm/input/TrajectoryLimits.pb.h"
#include "lm/oparam/OParams.h"
#include "lm/option/SimulationParameters.h"
#include "lm/tiling/Tilings.h"
#include "lm/trajectory/TrajectoryLimits.h"

using std::list;
using std::map;
using std::string;

namespace lm {
namespace input {

class Input
{
public:
    Input(const lm::io::hdf5::Hdf5File& file);
    virtual ~Input();

    // accessors
//    const lm::tiling::Tiling& getCurrentTiling() const {return getTilings().getCurrentTiling();}
    const lm::tiling::Tiling& getCurrentTiling() const;
    const lm::input::DiffusionModel& getDiffusionModelMsg() const {return diffusionModel;}
    const lm::oparam::OParams& getOrderParameters() const {return orderParameters;}
    const lm::input::OrderParameters& getOrderParametersMsg() const {return orderParametersMsg;}
    const lm::input::OutputOptions& getOutputOptionsMsg() const {return outputOptions;}
    const lm::input::ReactionModel& getReactionModelMsg() const {return reactionModel;}
    const lm::option::SimulationParameters& getSimulationParameters() const {return simulationParameters;}
    const lm::tiling::Tilings& getTilings() const {return tilings;}
    const lm::input::Tilings& getTilingsMsg() const {return tilingsMsg;}
    const lm::input::TrajectoryLimits& getTrajectoryLimitsMsg() const {return trajectoryLimits.buf();}

    uint64_t getPartsPerWorkUnit() const {return partsPerWorkUnit;}
    uint64_t getStepsPerWorkUnit() const {return stepsPerWorkUnit;}

    bool hasDegreeAdvancement() const {return degreeAdvancementPresent;}
    bool hasReactionModel() const {return reactionModelPresent;}
    bool hasDiffusionModel() const {return diffusionModelPresent;}
    bool hasOrderParameters() const {return orderParametersPresent;}
    bool hasTilings() const {return tilingsPresent;}
    bool hasTrajectoryLimits() const {return trajectoryLimitsPresent;}
    bool hasOutputOptions() const {return outputOptionsPresent;}

    lm::oparam::OParams* mutableOrderParameters() {return &orderParameters;}
    lm::tiling::Tilings* mutableTilings() {return &tilings;}
    lm::trajectory::TrajectoryLimits* mutableTrajectoryLimits() {return &trajectoryLimits;}

protected:
    virtual void init(const lm::io::hdf5::Hdf5File& file);
    virtual void initReactionModel(const lm::io::hdf5::Hdf5File& file);
    virtual void initDiffusionModel(const lm::io::hdf5::Hdf5File& file);
    virtual void initOrderParameters(const lm::io::hdf5::Hdf5File& file);
    virtual void initTilings(const lm::io::hdf5::Hdf5File& file);
    virtual void initTrajectoryLimits(const lm::io::hdf5::Hdf5File& file);
    virtual void initOutputOptions(const lm::io::hdf5::Hdf5File& file);
    virtual void initWorkUnitParameters(const lm::io::hdf5::Hdf5File& file);

    bool parseBoundaryConditions(lm::input::BoundaryConditions* bc, std::string arg);
    template <TrajLimEnums::LimitType LT> inline bool parseLimits(const std::string key, const std::string debugString, TrajLimEnums::StoppingCondition sc, bool includeEndpoint=true);
    template <typename ValT> inline bool parseAndSet(const std::string key, ValT* fieldPtr);
    template <typename T, typename SetterReturnT, typename ValT> inline bool parseAndSet(const std::string key, SetterReturnT (T::*setterFunc)(ValT), T& obj);
    template <typename T, typename AdderReturnT, typename ValT> inline bool parseAndSetList(const std::string key, AdderReturnT (T::*adderFunc)(ValT), T& obj);

protected:
    bool degreeAdvancementPresent;
    bool diffusionModelPresent;
    bool reactionModelPresent;
    bool orderParametersPresent;
    bool outputOptionsPresent;
    bool tilingsPresent;
    bool trajectoryLimitsPresent;

    lm::input::DiffusionModel diffusionModel;
    lm::input::OrderParameters orderParametersMsg;
    lm::oparam::OParams orderParameters;
    lm::input::OutputOptions outputOptions;
    lm::input::ReactionModel reactionModel;
    lm::input::Tilings tilingsMsg;
    lm::tiling::Tilings tilings;
    lm::trajectory::TrajectoryLimits trajectoryLimits;
    lm::option::SimulationParameters simulationParameters;

    uint64_t partsPerWorkUnit;
    uint64_t stepsPerWorkUnit;
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
//    virtual lm::input::BoundaryConditions* getBoundaryGradientBuf();
//    virtual lm::input::DiffusionModel* getDiffusionModelBuf();
//    virtual lm::input::SimulationParameters* getParametersBuf();
//    virtual lm::input::OrderParameters* getOrderParametersBuf();
//    virtual lm::input::ReactionModel* getReactionModelBuf();
//    virtual lm::input::SpatialModel* getSpatialModelBuf();
//    virtual lm::input::Tilings* getTilingsBuf();
//
//    // get protobuf methods (load-into-pointer style)
//    virtual void getBoundaryGradientBuf(lm::input::BoundaryConditions* bcBuf);
//    virtual void getDiffusionModelBuf(lm::input::DiffusionModel* diffusionModelBuf);
//    virtual void getParametersBuf(lm::input::SimulationParameters* parametersBuf);
//    virtual void getOrderParametersBuf(lm::input::OrderParameters* orderParametersBuf);
//    virtual void getReactionModelBuf(lm::input::ReactionModel* reactionModelBuf);
//    virtual void getSpatialModelBuf(lm::input::SpatialModel* modelBuf);
//    virtual void getTilingsBuf(lm::input::Tilings* tilingsBuf);
//
//    // get wrapper methods
//    virtual map<string,string> getParameters();
//    virtual string getParameter(string key, string defaultValue="");
//    virtual lm::oparam::OParam* getOrderParameter(uint id);
//    virtual lm::tiling::Tiling* getTiling(uint id)
//
//    // set protobuf methods
//    virtual void getBoundaryGradientBuf(lm::input::BoundaryConditions* bcBuf);
//    virtual void setDiffusionModelBuf(lm::input::DiffusionModel& diffusionModelBuf);
//    virtual void setOrderParametersBuf(lm::input::OrderParameters& orderParametersBuf);
//    virtual void setParametersBuf(lm::input::SimulationParameters& parametersBuf);
//    virtual void setReactionModelBuf(lm::input::ReactionModel& reactionModelBuf);
//    virtual void setSpatialModelBuf(lm::input::SpatialModel& modelBuf);
//    virtual void setTilingsBuf(lm::input::Tilings& tilingsBuf);
//
//    // set wrapper methods
//    virtual void setParameter(string key, string value);
//
//protected:
//    // load from file methods
//    virtual void _loadBoundaryGradientBuf(lm::input::BoundaryConditions* bcBuf);
//    virtual void _loadDiffusionModelBuf(lm::input::DiffusionModel* diffusionModelBuf);
//    virtual void _loadParametersBuf(lm::input::SimulationParameters* parametersBuf);
//    virtual void _loadOrderParametersBuf(lm::input::OrderParameters* orderParametersBuf);
//    virtual void _loadReactionModelBuf(lm::input::ReactionModel* reactionModelBuf);
//    virtual void _loadSpatialModelBuf(lm::input::SpatialModel* modelBuf);
//    virtual void _loadTilingsBuf(lm::input::Tilings* tilingsBuf);
//
//private:
//    lm::io::hdf5::Hdf5File& file;
//    lm::message::Message msgBuf;
//};

}
}
#endif // LM_INPUT_INPUT_H
