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
#include "lm/work/ReadOnly.pb.h"
#include "lm/work/Result.pb.h"
#include "lm/work/Work.pb.h"
#include "lm/io/ReactionModel.pb.h"

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
	trajectories.insert(std::make_pair(tid, createTrajectory(tid)));
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
	work.set_wallclocktime(3.0);
	work.set_allocated_readonly(&readOnly);
	work.set_allocated_readwrite(&readWrite);
	return Trajectory(work);
}

void TrajectoryAllocator::eraseTrajectory(map<int, Trajectory>::iterator traj_it)
{
	trajectories.erase(traj_it);
}

lm::work::Work & TrajectoryAllocator::Trajectory::getWork(vector<int> slotIds)
{
	work.set_pid(slotIds[0]);
	work.set_sid(slotIds[1]);
	return work;
}

void TrajectoryAllocator::Trajectory::update(lm::work::Result & result)
{
	work.set_allocated_readWrite(result.readWrite(result.readWrite_size - 1));
	work.set_pid(-1);
	work.set_sid(-1);
	status = check();
}

TrajectoryAllocator::Trajectory::trajectoryStatus TrajectoryAllocator::Trajectory::check()
{
	const lm::work::ReadWrite& readWrite = work.readWrite();
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
