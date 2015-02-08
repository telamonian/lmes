/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
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
#include <list>
#include <map>
#include <string>

#include "lm/Print.h"
#include "lm/input/Input.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/Message.pb.h"
#include "lm/tiling/Tilings.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/Types.h"

using lm::io::DiffusionModel;
using lm::io::ReactionModel;
using lm::io::TrajectoryState;
using lm::tiling::Tilings;
using std::map;
using std::string;

namespace lm {
namespace trajectory {

//Trajectory::Trajectory(uint64_t id,const ReactionModel& reactionModel,const DiffusionModel& diffusionModel,map<string,string>& simulationParameters,lm::tiling::Tilings* tilings,bool reversed):
//id(-1),status(NOT_STARTED)
//{
//    initState(reactionModel, reversed);
//    setID(id);
//    initMsg(simulationParameters);
//}
//
//Trajectory::Trajectory(uint64_t id,const ReactionModel& reactionModel,const DiffusionModel& diffusionModel,map<string,string>& simulationParameters,lm::tiling::Tilings* tilings,TrajectoryState* zerothState):
//id(id),status(NOT_STARTED)
//{
//    initState(zerothState);
//    setID(id);
//    initMsg(simulationParameters);
//}

Trajectory::Trajectory(uint64_t id,lm::input::Input& input,bool reversed)
:id(-1),input(input),status(NOT_STARTED),numberWorkUnitsPerformed(0)
{
    initState(input.reactionModelBuf, reversed);
    setID(id);
    initMsg(input.simulationParametersMap);
}

Trajectory::Trajectory(uint64_t id,lm::input::Input& input,TrajectoryState* zerothState)
    :id(id),input(input),status(NOT_STARTED),numberWorkUnitsPerformed(0)
{
    initState(zerothState);
    setID(id);
    initMsg(input.simulationParametersMap);
}

Trajectory::~Trajectory()
{
}

void Trajectory::initHists()
{
    lm::io::TilingHist* tHist = getState()->mutable_cme_state()->add_tiling_hists();
    tHist->set_tiling_id(input.tilings.getCurrentTilingID());
    for (lm::tiling::EdgeIterator e_it=input.tilings.getCurrentTiling()->begin();e_it!=input.tilings.getCurrentTiling()->end();e_it++)
    {
        tHist->add_tile_vals(0);
    }
//    for (lm::tiling::TilingMap::iterator t_it=input.tilings.begin();t_it!=input.tilings.end();t_it++)
//    {
//        lm::io::TilingHist* tHist = getState()->mutable_cme_state()->add_tiling_hists();
//        tHist->set_tiling_id(t_it->second->getID());
//        for (lm::tiling::EdgeIterator e_it=t_it->second->begin();e_it!=t_it->second->end();e_it++)
//        {
//            tHist->add_tile_vals(0);
//        }
//    }
}

void Trajectory::initMsg(map<string,string>& simulationParameters)
{
    // Set the default work unit-specific limits
    int64_t maxWorkUnitSteps = atoll(simulationParameters["maxWorkUnitSteps"].c_str());
    if (maxWorkUnitSteps <= 0) maxWorkUnitSteps = 10000000;
    getRunMsg()->set_max_steps(maxWorkUnitSteps);
}

void Trajectory::initMsg(const lm::message::Message& newMsg)
{
    setMsg(newMsg);
}

void Trajectory::initState(const lm::io::ReactionModel& reactionModel,bool reversed) // this version of initState creates the zeroth state from scratch
{
    // if state has any info in it already, clear it
    getState()->Clear();
    getState()->mutable_cme_state()->mutable_species_counts()->set_number_species(reactionModel.number_species());
    getState()->mutable_cme_state()->mutable_species_counts()->set_number_entries(1);
    if (!reversed)
    {
        for (int j=0; j<(int)reactionModel.number_species(); j++)
        {
            getState()->mutable_cme_state()->mutable_species_counts()->add_species_count(reactionModel.initial_species_count(j));
        }
    }
    else
    {
        for (int j=0; j<(int)reactionModel.number_species(); j++)
        {
            getState()->mutable_cme_state()->mutable_species_counts()->add_species_count(reactionModel.initial_species_count_backward(j));  // reversed_initial_species_count is set in the input file
        }
    }
    getState()->mutable_cme_state()->mutable_species_counts()->add_time(0.0);
//    if (input.hasOrderParameters) initOPs();
    if (input.hasTilings) initHists();
}

void Trajectory::initState(lm::io::TrajectoryState* initState)
{
    // Make instance local copy of the passed state
    setState(initState);
}

// accessor definitions
uint64_t Trajectory::getID()
{
    return id;
}

lm::io::TrajectoryLimits* Trajectory::getLimits()
{
    return getRunMsg()->mutable_limits();
}

lm::message::Message* Trajectory::getMsg()
{
    return &msg;
}

lm::message::Message* Trajectory::getNextWorkUnitMsg(uint64_t nextWorkUnitID)
{
    if (getStatus()==Trajectory::NOT_STARTED || getStatus()==Trajectory::WAITING)
    {
        setStatus(Trajectory::RUNNING);
        setWorkUnitId(nextWorkUnitID);
        return getMsg();
    }
    else
    {
        return NULL;
    }
}

double Trajectory::getOPVal(uint opID)
{
	uint speciesCountSize = getSpeciesCounts()->species_count_size();
	uint* lastSpeciesCount = new uint[getSpeciesCounts()->number_species()];
	uint offset = (getSpeciesCounts()->number_entries() - 1)*(getSpeciesCounts()->number_species());
	for (int i=0; i<getSpeciesCounts()->number_species(); i++)
	{
		lastSpeciesCount[i] = getSpeciesCounts()->species_count(i + offset);
	}
	return input.oparams[opID]->calc(lastSpeciesCount);
	delete [] lastSpeciesCount;
}

lm::message::RunWorkUnit* Trajectory::getRunMsg()
{
	return msg.mutable_run_work_unit();
}

lm::io::SpeciesCounts* Trajectory::getSpeciesCounts()
{
	return getState()->mutable_cme_state()->mutable_species_counts();
}

Trajectory::status_t Trajectory::getStatus()
{
    return status;
}

lm::io::TrajectoryState* Trajectory::getState()
{
    return getRunMsg()->mutable_initial_state();
}

// mutator definitions
void Trajectory::setID(uint64_t newID)
{
    id = newID;
    getState()->set_trajectory_id(newID);
    getState()->mutable_cme_state()->mutable_species_counts()->set_trajectory_id(newID);
}

void Trajectory::setLimits(const lm::io::TrajectoryLimits* newLimits)
{
    *(getRunMsg()->mutable_limits()) = *newLimits;
}

void Trajectory::setMsg(const lm::message::Message& newMsg)
{
    msg = newMsg;
}

void Trajectory::setStarted(bool trajectoryStarted)
{
    getState()->set_trajectory_started(trajectoryStarted);
}

void Trajectory::setState(const lm::io::TrajectoryState* newState)
{
    *getState() = *newState;
}

void Trajectory::setStatus(status_t newStatus)
{
    status = newStatus;
}

void Trajectory::setWorkUnitId(uint64_t id)
{
    getRunMsg()->set_work_unit_id(id);
}

void Trajectory::incrementWorkUnitsPerformed()
{
    numberWorkUnitsPerformed++;
}

int64_t Trajectory::getWorkUnitsPerformed()
{
    return numberWorkUnitsPerformed;
}

}
}
