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
#include "lm/io/hdf5/SimulationFile.h"
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
    TrajectoryAllocator(lm::io::hdf5::Hdf5File * file, bool needsReactionModel, bool needsDiffusionModel):
    	tidCounter(0), file(file), needsReactionModel(needsReactionModel), needsDiffusionModel(needsDiffusionModel) {initialize();}
    virtual ~TrajectoryAllocator();

    virtual void initialize();
    virtual int createTid();
    virtual void initTrajectory();
    virtual Trajectory createTrajectory(int tid);
    virtual void destoryTrajectory();
    virtual void update(int tid, lm::work::Result & Result) {trajectories[tid].update()}

    int tidCounter;			//equal to next trajectory ID to be created
    bool reusableInitDone;	//true if the reusable portion of the trajectory initialization procedure has been assembled at least once
    lm::io::hdf5::Hdf5File * file;
    lm::io::SimulationParameters simulationParameters;
    lm::io::ReactionModel reactionModel;
    lm::work::ReadWrite readWrite;
    bool needsReactionModel;
    bool needsDiffusionModel;
    map<int, Trajectory> trajectories;
};

}
}

#endif /* THREADALLOCATOR_H_ */
