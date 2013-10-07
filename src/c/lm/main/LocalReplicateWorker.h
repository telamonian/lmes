/*
 * LocalReplicateWorker.h
 *
 *  Created on: Oct 4, 2013
 *      Author: tel
 */

#ifndef LOCALREPLICATEWORKER_H_
#define LOCALREPLICATEWORKER_H_

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

class LocalReplicateWorker : public lm::thread::Worker
{
using lm::me::MESolverFactory;
using lm::main::ReplicateRunner;
using std::list;
using std::map;
using std::string;

public:
    LocalReplicateWorker(lm::io::hdf5::SimulationFile * file);
    ~LocalReplicateWorker();

    virtual void wake();
    virtual void abort();
    virtual void checkpoint();
    void broadcastSimulationParameters(void * staticDataBuffer, map<string,string> & simulationParameters);

protected:
    virtual int run();

private:
    //variables relating to Worker behavior
    bool shouldCheckpoint;
    bool shouldAbort;

    lm::io::hdf5::SimulationFile * file;

};

}
}

#endif /* LOCALREPLICATEWORKER_H_ */
