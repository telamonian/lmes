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
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/oparam/OParams.h"
#include "lm/option/SimulationParameters.h"
#include "lm/tiling/Tilings.h"
#include "lm/limit/TrajectoryLimits.h"

using std::list;
using std::map;
using std::string;

namespace lm {
namespace input {

class Input
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject(const lm::io::hdf5::Hdf5File& file);

public:
    Input();
    Input(const lm::io::hdf5::Hdf5File& file);
    virtual ~Input();

    // accessors
    void copyLimitsTo(lm::message::RunWorkUnit* rwuMsg);
    void copyLimitTrackingsTo(lm::message::RunWorkUnit* rwuMsg);
    const lm::tiling::Tiling& getCurrentTiling() const {return getTilings().getCurrentTiling();}

    const lm::input::DiffusionModel& getDiffusionModelMsg() const {return diffusionModel;}
    const lm::oparam::OParams& getOrderParameters() const {return orderParameters;}
    const lm::input::OrderParameters& getOrderParametersMsg() const {return orderParametersMsg;}
    const lm::input::OutputOptions& getOutputOptionsMsg() const {return outputOptionsMsg;}
    const lm::input::ReactionModel& getReactionModelMsg() const {return reactionModelMsg;}
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
    lm::limit::TrajectoryLimits* mutableTrajectoryLimits() {return &trajectoryLimits;}

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

    virtual void setFlagsOnSucess(bool result, bool* resultFlag0, bool* resultFlag1);

protected:
    bool degreeAdvancementPresent;
    bool diffusionModelPresent;
    bool reactionModelPresent;
    bool orderParametersPresent;
    bool outputOptionsPresent;
    bool tilingsPresent;
    bool trajectoryLimitsPresent;

    lm::input::DiffusionModel diffusionModel;
    lm::io::LimitTrackingList limitTrackingListMsg;
    lm::limit::LimitTrackingListWrap limitTrackingListWrap;
    lm::input::OrderParameters orderParametersMsg;
    lm::oparam::OParams orderParameters;
    lm::input::OutputOptions outputOptionsMsg;
    lm::input::ReactionModel reactionModelMsg;
    lm::input::Tilings tilingsMsg;
    lm::tiling::Tilings tilings;
    lm::limit::TrajectoryLimits trajectoryLimits;
    lm::option::SimulationParameters simulationParameters;

    bool includeEndpointInLimits;

    uint64_t partsPerWorkUnit;
    uint64_t stepsPerWorkUnit;

// template methods for parsing user input
protected:
    template <TrajLimEnums::LimitType LT>
    bool parseLimits(const string key, const string debugString, TrajLimEnums::StoppingCondition sc, bool includeEndpoint, bool* resultFlag0=NULL, bool* resultFlag1=NULL)
    {
        bool result;
        if (simulationParameters.count(key)!=0)
        {
            typename PairVector<uint, typename lm::limit::LimitElement<LT>::type>::T idLimitVec(simulationParameters.parsePairVector<uint, typename lm::limit::LimitElement<LT>::type>(key, debugString));
            for (typename PairVector<uint, typename lm::limit::LimitElement<LT>::type>::iterator it(idLimitVec.begin()); it!=idLimitVec.end(); it++)
            {
                trajectoryLimits.addLimitMsg<LT>(it->first, it->second, sc, includeEndpoint);
            }
            result = (idLimitVec.size() > 0);
        }
        else
        {
            result = false;
        }

        setFlagsOnSucess(result, resultFlag0, resultFlag1);
        return result;
    }

// Version of parseAndSet that works with options that can directly accessed through a mutable pointer
// By using template parameter inference on the pointer, this template automatically figures out what type to parse from simulationParameters
    template <typename ValT>
    bool parseAndSet(const string key, ValT* fieldPtr, bool* resultFlag0=NULL, bool* resultFlag1=NULL)
    {
        bool result;
        if (simulationParameters.count(key)!=0)
        {
            *fieldPtr = simulationParameters.parse<ValT>(key);
            result = true;
        }
        else
        {
            result = false;
        }

        setFlagsOnSucess(result, resultFlag0, resultFlag1);
        return result;
    }

// Version of parseAndSet that works with options that need to be set via a setter function
// By using template parameter inference on the setter (passed as a function pointer), this template automatically figures out what type to parse from simulationParameters
    template <typename T, typename SetterReturnT, typename ValT>
    bool parseAndSet(const string key, SetterReturnT (T::*setterFunc)(ValT), T& obj, bool* resultFlag0=NULL, bool* resultFlag1=NULL)
    {
        bool result;
        if (simulationParameters.count(key)!=0)
        {
            (obj.*setterFunc)(simulationParameters.parse<ValT>(key));
            result = true;
        }
        else
        {
            result = false;
        }

        setFlagsOnSucess(result, resultFlag0, resultFlag1);
        return result;
    }

// specialized version of parseAndSet for flag options (ie options that can be only true or false). If the flag key is present in simulationParameters then the flag is set to true (regardless of its value in simulationParameters), otherwise the flag is set to false
    inline bool parseAndSetFlag(const std::string key, bool* flagPtr, bool* resultFlag0=NULL, bool* resultFlag1=NULL)
    {
        bool result = (simulationParameters.count(key)!=0);
        *flagPtr = result;

        setFlagsOnSucess(result, resultFlag0, resultFlag1);
        return result;
    }

// same as parseAndSetFlag, but this version works with options that need to be set via a setter function
    template <typename T>
    bool parseAndSetFlag(const std::string key, void (T::*setterFunc)(bool), T& obj, bool* resultFlag0=NULL, bool* resultFlag1=NULL)
    {
        bool result = (simulationParameters.count(key)!=0);
        (obj.*setterFunc)(result);

        setFlagsOnSucess(result, resultFlag0, resultFlag1);
        return result;
    }

// Same as parseAndSet, but for options specified as lists
    template <typename T, typename AdderReturnT, typename ValT>
    bool parseAndSetList(const string key, AdderReturnT (T::*adderFunc)(ValT), T& obj, bool* resultFlag0=NULL, bool* resultFlag1=NULL)
    {
        bool result;
        if (simulationParameters.count(key)!=0)
        {
            std::vector<ValT> parsedVector(simulationParameters.parseVector<ValT>(key));
            for (typename std::vector<ValT>::const_iterator it=parsedVector.begin(); it!=parsedVector.end(); it++)
            {
                (obj.*adderFunc)(*it);
            }
            result = (parsedVector.size() > 0);
        }
        else
        {
            result = false;
        }

        setFlagsOnSucess(result, resultFlag0, resultFlag1);
        return result;
    }
    
};

}
}
#endif // LM_INPUT_INPUT_H
