/*
 * ThreadAllocator.cpp
 *
 *  Created on: Jan 19, 2014
 *      Author: tel
 */
#include <string>
#include <cmath>
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/SimulationParameters.h"
#include "lm/work/ReadOnly"
#include "lm/work/Result.pb.h"
#include "lm/work/Work.pb.h"
#include "ReactionModel.pb.h"

namespace lm {
namespace resource {

void TrajectoryAllocator::initialize()
{
	simulationParameters = file->getSimulationParameters();
	reactionModel = file->getReactionModel();
	readOnly = file->getReadOnly();
	readWrite = file->getReadWrite();
	// Get the simulation time limit.
	maxTime = atof(simulationParameters["maxTime"].c_str());
	maxSteps = atof(simulationParameters["maxSteps"].c_str());
}

int TrajectoryAllocator::createTid()
{
	return tidCounter++;
}

void TrajectoryAllocator::initTrajectory()
{
	int tid = createTid();
	trajectories[tid] = createTrajectory(tid);
}

void TrajectoryAllocator::initTrajectories(int n)
{
	for (int i=0; i<n; ++i)
	{
		initTrajectory();
	}
}

TrajectoryAllocator::Trajectory TrajectoryAllocator::createTrajectory(int tid)
{
	lm::work::Work work;
	work.set_tid(tid);
	work.set_pid(-1);
	work.set_sid(-1);
	work.set_simulationParameters(simulationParameters);
	work.set_reactionModel(reactionModel);
	work.set_readOnly(readOnly);
	work.set_readWrite(readWrite);
	return Trajectory(work);
}

void TrajectoryAllocator::eraseTrajectory(map<int, Trajectory>::iterator traj_it)
{
	trajectories.erase(traj_it);
}

void TrajectoryAllocator::Trajectory::distribute(vector<int> slotIds)
{
	work.set_pid(slotIds[0]);
	work.set_sid(slotIds[1]);
	/////////
	// MPI STUFF GOES HERE
	/////////
}

void TrajectoryAllocator::Trajectory::update(lm::work::Result & result)
{
	work.set_readWrite(result.readWrite(result.readWrite_size - 1));
	work.set_pid(-1);
	work.set_sid(-1);
	status = check();
}

TrajectoryAllocator::Trajectory::trajectoryStatus TrajectoryAllocator::Trajectory::check()
{
	const lm::work::ReadWrite& readWrite = work.get_readWrite();
	if (readWrite.get_step > maxStep && readWrite.get_time > maxTime)
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
