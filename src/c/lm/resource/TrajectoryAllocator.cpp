/*
 * ThreadAllocator.cpp
 *
 *  Created on: Jan 19, 2014
 *      Author: tel
 */

void TrajectoryAllocator::createTid()
{
	return tidCounter++;
}

void TrajectoryAllocator::initTrajectory()
{
	int tid = createTid();
	trajectories[tid] = createTrajectory();
}

void TrajectoryAllocator::createTrajectory()
{

}

void TrajectoryAllocator::destoryTrajectory()
{

}
