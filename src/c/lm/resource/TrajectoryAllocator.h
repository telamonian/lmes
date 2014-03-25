/*
 * TrajectoryAllocator.h
 *
 *  Created on: Jan 19, 2014
 *      Author: tel
 */

#ifndef TRAJECTORYALLOCATOR_H_
#define TRAJECTORYALLOCATOR_H_

#include <string>
#include <map>
#include <vector>
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/io/SimulationParameters.h"
#include "lm/thread/Thread.h"
#include "lm/work/ReadOnly.pb.h"
#include "lm/work/Result.pb.h"
#include "lm/work/Work.pb.h"
#include "lm/io/ReactionModel.pb.h"

using std::map;
using std::string;
using std::vector;
using lm::thread::PthreadException;

namespace lm {
namespace resource {

class TrajectoryAllocator
{

public:
	static enum trajectoryStatus {CONTINUE, FINISHED};
	class Trajectory
	{
	public:
		Trajectory(lm::work::Work work): work(work), status(CONTINUE) {}
		virtual ~Trajectory();

		virtual void assignSlot();
		virtual void update(lm::work::Result & result);
		virtual lm::work::Work & getWork(vector<int> slotIds);
		virtual trajectoryStatus check();
		virtual int getTid() {return work.tid();}
		virtual int getPid() {return work.pid();}
		virtual int getSid() {return work.sid();}

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
    virtual void initTrajectories(int n);
    virtual Trajectory createTrajectory(int tid);
    virtual void eraseTrajectory(map<int, Trajectory>::iterator traj_it);
    virtual void update(lm::work::Result & result) {trajectories.find(result.tid())->second.update(result);}
    virtual map<int, Trajectory>::iterator getBegin() {return trajectories.begin();}
    virtual map<int, Trajectory>::iterator getEnd() {return trajectories.end();}

    double maxTime;
    long long maxStep;
    int tidCounter;			//equal to next trajectory ID to be created
    lm::io::hdf5::Hdf5File * file;
    lm::io::SimulationParameters simulationParameters;
    lm::io::ReactionModel reactionModel;
    lm::work::ReadOnly readOnly;
    lm::work::ReadWrite readWrite;
    bool needsReactionModel;
    bool needsDiffusionModel;
    map<int, Trajectory> trajectories;
};

}
}

#endif /* TRAJECTORYALLOCATOR_H_ */
