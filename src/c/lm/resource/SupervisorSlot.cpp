/*
 * SupervisorSlot.cpp
 *
 *  Created on: Jan 26, 2014
 *      Author: tel
 */
#include "lm/MPI.h"
#include "lm/resource/SupervisorSlot.h"
#include "lm/work/Work.pb.h"

namespace lm {
namespace resource {

SupervisorSlot::SupervisorSlot(vector<int> ids): Slot(ids), staticDataBuffer(NULL)
{
	MPI_EXCEPTION_CHECK(MPI_Alloc_mem(lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE, MPI_INFO_NULL, &staticDataBuffer));
}

SupervisorSlot::~SupervisorSlot()
{
	MPI_EXCEPTION_CHECK(MPI_Free_mem(staticDataBuffer));
}

vector<int> SupervisorSlot::alloc(lm::work::Work & work)
{
	int msgSize = work.ByteSize();
	if (msgSize > lm::MPI::OUTPUT_DATA_STATIC_MAX_SIZE) throw Exception("Message exceeded buffer size. Message tag:", lm::MPI::MSG_WORK_UNIT);
	work.SerializeToArray(staticDataBuffer, msgSize);
	MPI_EXCEPTION_CHECK(MPI_Send(staticDataBuffer, msgSize, MPI_BYTE, work.pid(), lm::MPI::MSG_WORK_UNIT, MPI_COMM_WORLD));

	vector<int> slotIds(2);
	slotIds.push_back(work.pid());
	slotIds.push_back(work.sid());
	return slotIds;
}

}
}




