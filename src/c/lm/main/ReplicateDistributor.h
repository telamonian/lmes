/*
 * ReplicateDistributor
 *
 *  Created on: Oct 4, 2013
 *      Author: tel
 */

#ifndef ReplicateDistributor_
#define ReplicateDistributor_

#include <list>
#include <map>
#include <string>
#include "DiffusionModel.pb.h"
#include "ReactionModel.pb.h"
#include "lm/main/Main.h"
#include "lm/resource/ResourceAllocator.h"
#include "lm/main/ReplicateRunner.h"
#include "lm/me/MESolverFactory.h"
#include "lm/Print.h"
#include "lm/thread/Worker.h"
#include "lm/thread/Thread.h"

namespace lm {
namespace main {

using lm::me::MESolverFactory;
using lm::main::ReplicateRunner;
using std::list;
using std::map;
using std::string;

class ReplicateDistributor : public lm::thread::Worker
{

public:
    ReplicateDistributor(ResourceAllocator & resourceAllocator, MESolverFactory & solverFactory) throw(PthreadException);
    virtual ~ReplicateDistributor() throw(PthreadException);

    virtual void wake() throw(PthreadException);
    virtual void abort() throw(PthreadException);
    virtual void checkpoint() throw(PthreadException);

protected:
    virtual int run();

    template <int tag>
    void receiveSizeThenBuffer(void * staticDataBuffer, int & msgSize);

    template <typename t, int tag>
    void receiveThing(void * staticDataBuffer, t * thing);

//    virtual map<string,string> receiveSimulationParameters(void * staticDataBuffer);
//    virtual void receiveReactionModel(void * staticDataBuffer, lm::io::ReactionModel * reactionModel);
//    virtual void receiveDiffusionModel(void * staticDataBuffer, lm::io::DiffusionModel * diffusionModel, uint8_t ** lattice, size_t * latticeSize, uint8_t ** latticeSites, size_t * latticeSitesSize);
    void receiveLatticeModel(uint8_t ** lattice, size_t * latticeSize, uint8_t ** latticeSites, size_t * latticeSitesSize); //TODO: finish refactoring this out of existence
    virtual void startReplicate(int replicate, MESolverFactory solverFactory, std::map<std::string,string> & simulationParameters, lm::io::ReactionModel * reactionModel, lm::io::DiffusionModel * diffusionModel, uint8_t * lattice, size_t latticeSize, uint8_t * latticeSites, size_t latticeSitesSize, ResourceAllocator & resourceAllocator) throw(Exception,PthreadException);
    virtual ReplicateRunner * popNextFinishedReplicate(list<ReplicateRunner *> & runningReplicates, ResourceAllocator & resourceAllocator);

private:
    //variables relating to Worker behavior
    void * staticDataBuffer;
    bool shouldCheckpoint;
    bool shouldAbort;

    //resourceAllocator and solverFactory are member variables since they're needed when starting replicates in run()
    ResourceAllocator & resourceAllocator;
    MESolverFactory & solverFactory;
    list<ReplicateRunner *> runningReplicates;
};

}
}

#endif /* ReplicateDistributor_ */
