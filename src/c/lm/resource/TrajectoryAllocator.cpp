/*
 * ThreadAllocator.cpp
 *
 *  Created on: Jan 19, 2014
 *      Author: tel
 */
#include <cmath>
#include <string>
#include <utility>
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/SimulationParameters.h"
#include "lm/MPI.h"
#include "lm/resource/TrajectoryAllocator.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/work/ReadOnly.pb.h"
#include "lm/work/Result.pb.h"
#include "lm/work/Work.pb.h"

typedef map<int, lm::resource::TrajectoryAllocator::Trajectory> TrajectoryMap;

namespace lm {
namespace resource {

void TrajectoryAllocator::initialize() {}

int TrajectoryAllocator::createTid()
{
	return tidCounter++;
}

void TrajectoryAllocator::initTrajectories(int n)
{
	for (int i=0; i<n; ++i)
	{
		initTrajectory();
	}
}

void TrajectoryAllocator::initTrajectory()
{
	int tid = createTid();
	createTrajectory(tid);
}

void TrajectoryAllocator::createTrajectory(int tid)
{
	lm::work::Work * work = new lm::work::Work;

	//// TODO: this section is here only for development and will be removed in the production version
	lm::work::ReadOnly * readOnly(work->mutable_readonly());
	lm::work::ReadWrite * readWrite(work->mutable_readwrite());
	lm::io::ReactionModel * reactionModel(readOnly->mutable_reactionmodel());
	lm::message::SimulationParameters * simulationParameters(readOnly->mutable_simulationparameters());
	lm::io::ReactionModel_Reaction * react1(reactionModel->add_reaction());

	react1->set_type(2);
	react1->add_rate_constant(1);
	reactionModel->set_number_species(1);
	reactionModel->set_number_reactions(1);
	reactionModel->add_initial_species_count(2);
	reactionModel->add_stoichiometric_matrix(1);
	reactionModel->add_dependency_matrix(1);

	simulationParameters->add_key("foo");
	simulationParameters->add_value("bar");

	readWrite->set_step(0);
	readWrite->set_time(0);
	readWrite->add_species_count(10);
	//// this section is here only for development and will be removed in the production version

	work->set_tid(tid);
	work->set_pid(-1);
	work->set_sid(-1);
	work->set_wallclocktime(3.0);
	trajectories.insert(TrajectoryMap::value_type(tid, Trajectory(work, 10, 10)));
}

void TrajectoryAllocator::eraseTrajectory(map<int, Trajectory>::iterator traj_it)
{
	trajectories.erase(traj_it);
}

lm::work::Work & TrajectoryAllocator::Trajectory::getWork(vector<int> slotIds)
{
	work->set_pid(slotIds[0]);
	work->set_sid(slotIds[1]);
	return *work;
}

void TrajectoryAllocator::Trajectory::update(lm::work::Result & result)
{
	work->set_allocated_readwrite(result.mutable_readwrite(result.readwrite_size() - 1));
	work->set_pid(-1);
	work->set_sid(-1);
	status = check();
}

TrajectoryAllocator::trajectoryStatus TrajectoryAllocator::Trajectory::check()
{
	const lm::work::ReadWrite& readWrite = work->readwrite();
	if (readWrite.step() > maxStep && readWrite.time() > maxTime)
	{
		return FINISHED;
	}
	else
	{
		return CONTINUE;
	}
}

}
}
