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
#include "lm/replicates/TrajectoryList.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/work/ReadOnly.pb.h"
#include "lm/work/Result.pb.h"

namespace lm {
namespace resource {

TrajectoryList::TrajectoryList(int numberTrajectories, double maxTime, long long maxStep)
    : numberTrajectories(numberTrajectories),maxTime(maxTime),maxStep(maxStep)
{

}


//void TrajectoryAllocator::initialize()
//{
//	//stub for creation of work unit for testing purposes
//	lm::io::ReactionModel_Reaction * react1(reactionModel.add_reaction());
//	react1->set_type(2);
//	react1->add_rate_constant(1);
//	reactionModel.set_number_species(1);
//	reactionModel.set_number_reactions(1);
//	reactionModel.add_initial_species_count(2);
//	reactionModel.add_stoichiometric_matrix(1);
//	reactionModel.add_dependency_matrix(1);

//	//simulationParameters

//	readOnly.set_allocated_reactionmodel(&reactionModel);
//	readOnly.set_allocated_simulationparameters(&simulationParameters);

//	readWrite.set_step(0);
//	readWrite.set_time(0);
//	readWrite.add_species_count(10);

////	simulationParameters = file->getSimulationParameters();
////	reactionModel = file->getReactionModel();
////	readOnly = file->getReadOnly();
////	readWrite = file->getReadWrite();
////	// Get the simulation time limit.
////	maxTime = atof(simulationParameters["maxTime"].c_str());
////	maxSteps = atof(simulationParameters["maxSteps"].c_str());
//}

//int TrajectoryAllocator::createTid()
//{
//	return tidCounter++;
//}

//void TrajectoryAllocator::initTrajectory()
//{
//	int tid = createTid();
//	trajectories.insert(std::make_pair(tid, createTrajectory(tid)));
//}

//void TrajectoryAllocator::initTrajectories(int n)
//{
//	for (int i=0; i<n; ++i)
//	{
//		initTrajectory();
//	}
//}

//TrajectoryAllocator::Trajectory TrajectoryAllocator::createTrajectory(int tid)
//{
//	lm::work::Work work;
//	work.set_tid(tid);
//	work.set_pid(-1);
//	work.set_sid(-1);
//	work.set_wallclocktime(3.0);
//	work.set_allocated_readonly(&readOnly);
//	work.set_allocated_readwrite(&readWrite);
//	return Trajectory(work, 10, 10);
//}

//void TrajectoryAllocator::eraseTrajectory(map<int, Trajectory>::iterator traj_it)
//{
//	trajectories.erase(traj_it);
//}

//lm::work::Work & TrajectoryAllocator::Trajectory::getWork(vector<int> slotIds)
//{
//	work.set_pid(slotIds[0]);
//	work.set_sid(slotIds[1]);
//	return work;
//}

//void TrajectoryAllocator::Trajectory::update(lm::work::Result & result)
//{
//	work.set_allocated_readwrite(result.mutable_readwrite(result.readwrite_size() - 1));
//	work.set_pid(-1);
//	work.set_sid(-1);
//	status = check();
//}

//TrajectoryAllocator::trajectoryStatus TrajectoryAllocator::Trajectory::check()
//{
//	const lm::work::ReadWrite& readWrite = work.readwrite();
//	if (readWrite.step() > maxStep && readWrite.time() > maxTime)
//	{
//		return FINISHED;
//	}
//	else
//	{
//		return CONTINUE;
//	}
//}

}
}
