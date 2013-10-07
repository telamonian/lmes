/*
 * ReplicateManager.h
 *
 *  Created on: Oct 4, 2013
 *      Author: tel
 */

#ifndef REPLICATEMANAGER_H_
#define REPLICATEMANAGER_H_

#include <list>
#include <map>
#include <string>
#include "lm/io/DiffusionModel.pb.h"
#include "lm/io/ReactionModel.pb.h"
#include "lm/main/ResourceAllocator.h"
#include "lm/main/ReplicateRunner.h"
#include "lm/me/MESolverFactory.h"
#include "lm/thread/Worker.h"

namespace lm {
namespace main {

class ReplicateManager : public lm::thread::Worker
{
using lm::me::MESolverFactory;
using lm::main::ReplicateRunner;
using std::list;
using std::map;
using std::string;

public:
    ReplicateManager(ResourceAllocator & resourceAllocator, MESolverFactory & solverFactory);
    ~ReplicateManager();

    virtual void wake();
    virtual void abort();
    virtual void checkpoint();

protected:
    virtual int run();
    virtual map<string,string> receiveSimulationParameters(void * staticDataBuffer);
    virtual void receiveReactionModel(void * staticDataBuffer, lm::io::ReactionModel * reactionModel);
    virtual void receiveDiffusionModel(void * staticDataBuffer, lm::io::DiffusionModel * diffusionModel, uint8_t ** lattice, size_t * latticeSize, uint8_t ** latticeSites, size_t * latticeSitesSize);
    virtual ReplicateRunner * startReplicate(int replicate, MESolverFactory solverFactory, std::map<std::string,string> & simulationParameters, lm::io::ReactionModel * reactionModel, lm::io::DiffusionModel * diffusionModel, uint8_t * lattice, size_t latticeSize, uint8_t * latticeSites, size_t latticeSitesSize, ResourceAllocator & resourceAllocator) throw(Exception,PthreadException);
    virtual ReplicateRunner * popNextFinishedReplicate(list<ReplicateRunner *> & runningReplicates, ResourceAllocator & resourceAllocator);

private:
    //variables relating to Worker behavior
    bool shouldCheckpoint;
    bool shouldAbort;

    //resourceAllocator and solverFactory are member variables since they're needed when starting replicates in run()
    ResourceAllocator & resourceAllocator;
    MESolverFactory & solverFactory;

};

}
}

#endif /* REPLICATEMANAGER_H_ */
