/*
 * ThreadAllocator.cpp
 *
 *  Created on: Jan 19, 2014
 *      Author: tel
 */

#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/SimulationParameters.h"
#include "lm/work/ReadOnly"
#include "lm/work/Result.pb.h"
#include "lm/work/Work.pb.h"
#include "ReactionModel.pb.h"

void TrajectoryAllocator::initialize()
{
	simulationParameters = file->getSimulationParameters();
	reactionModel = file->getReactionModel();
	readOnly = file->getReadOnly();
}

void TrajectoryAllocator::createTid()
{
	return tidCounter++;
}

void TrajectoryAllocator::initTrajectory()
{
	int tid = createTid();
	trajectories[tid] = createTrajectory(tid);
}

Trajectory TrajectoryAllocator::createTrajectory(int tid, lm::io::hdf5::Hdf5File * file)
{
	lm::work::Work work;
	work.set_tid(tid);
	work.set_simulationParameters(simulationParameters);
	work.set_reactionModel(reactionModel);
	work.set_readOnly(readOnly);
	return Trajectory(work);
}

void TrajectoryAllocator::destoryTrajectory(int tid)
{
	trajectories.erase(tid);
}
