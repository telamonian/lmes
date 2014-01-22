/*
 * ThreadAllocator.h
 *
 *  Created on: Jan 19, 2014
 *      Author: tel
 */

#ifndef THREADALLOCATOR_H_
#define THREADALLOCATOR_H_

#include <string>
#include <vector>
#include "lm/io/SimulationParameters.h"
#include "lm/thread/Thread.h"
#include "lm/work/ReadOnly"
#include "lm/work/Result.pb.h"
#include "lm/work/Work.pb.h"
#include "ReactionModel.pb.h"

using std::string;
using std::vector;
using lm::thread::PthreadException;

namespace lm {
namespace resource {

class TrajectoryAllocator
{
//public:
//	class Trajectory
//	{
//	public:
//		Trajectory(): pid(-1), sid(-1) {}
//		virtual ~Trajectory();
//		virtual void assignSlot(int currentPid, int currentSid) {pid=currentPid; sid=currentSid}
//
//		int pid;		//process ID. Assigned for duration of work unit
//		int sid;		//slot ID. Assigned for duration of work unit
//		lm::io::SimulationParameters simParams;
//		lm::io::ReactionModel reactionModel;
//	};

public:
	class Trajectory
	{
	public:
		static enum trajectoryStatus {CONTINUE, CHECK, KILL};
		Trajectory(lm::work::Work work): work(work), status(CONTINUE) {}
		virtual ~Trajectory();

		virtual void assignSlot();
		virtual void update(lm::work::Result result);

		lm::work::Work work;
		trajectoryStatus status;
	};

public:
    TrajectoryAllocator(bool needsReactionModel, bool needsDiffusionModel);
    virtual ~TrajectoryAllocator();

    virtual int createTid();
    virtual void initTrajectory();
    virtual void createTrajectory();
    virtual void destoryTrajectory();
    virtual void update(int tid, lm::work::Result Result) {trajectories[tid].update()}

    int tidCounter;		//equal to next trajectory ID to be created
    map<int, Trajectory> trajectories;
};

}
}

#endif /* THREADALLOCATOR_H_ */
