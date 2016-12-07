/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 * 			     Johns Hopkins University
 * 			     http://biophysics.jhu.edu/roberts/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the Software), to deal with
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimers.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimers in the documentation
 * and/or other materials provided with the distribution.
 *
 * - Neither the names of the Roberts Group, Johns Hopkins University,
 * nor the names of its contributors may be used to endorse or
 * promote products derived from this Software without specific prior written
 * permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS WITH THE SOFTWARE.
 *
 * Author(s): Elijah Roberts, Max Klein
 */
#include <cstdio>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>

#include "lm/Print.h"
#include "lm/message/Communicator.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/slot/Slot.h"

using std::fixed;
using std::map;
using std::ostringstream;
using std::setprecision;
using std::scientific;
using std::string;
using std::setfill;
using std::setw;

typedef vector<string> HeaderVector;
typedef map<string, int> ColumnMap;

const string headerStrings[] = {"ID", "Address", "Work_units", "Work_unit_steps", "Work_unit_time", "Steps_per_sec"};
HeaderVector headers(headerStrings, headerStrings + 6);
const ColumnMap::value_type rawData[] = {
    ColumnMap::value_type("ID", 5),
    ColumnMap::value_type("Address", 40),
    ColumnMap::value_type("Work_units", 15),
    ColumnMap::value_type("Work_unit_steps", 20),
    ColumnMap::value_type("Work_unit_time", 20),
    ColumnMap::value_type("Steps_per_sec", 19)
};
const int numElems = sizeof rawData / sizeof rawData[0];
ColumnMap statsColumnMap(rawData, rawData + numElems);

namespace lm {
namespace slot {

Slot::Slot(int32_t id, lm::resource::ComputeResources resources)
:id(id),status(NOT_STARTED),resources(resources),simultaneousWorkUnits(0)
{
    resetSlotStatistics();
}

Slot::~Slot()
{
}

string Slot::getSlotStatisticsHeader()
{
    ostringstream headerStream;
    for (HeaderVector::iterator it=headers.begin(); it!=headers.end(); it++)
    {
        headerStream << setw(statsColumnMap[*it]) << *it;
    }
    return headerStream.str();
}

string Slot::getSlotStatisticsHeaderBreak()
{
    ostringstream headerBreakStream;
    headerBreakStream << setw(Slot::getSlotStatisticsHeader().size()) << setfill('-') << "";
    return headerBreakStream.str();
}

string Slot::getSlotStatistics() const
{
    ostringstream statsStream;
    statsStream << fixed << setprecision(0) << setw(statsColumnMap["ID"]) << id;
    statsStream << fixed << setprecision(0) << setw(statsColumnMap["Address"])<< resources.hostname << lm::message::Communicator::printableAddress(workUnitRunnerAddress);
    statsStream << fixed << setprecision(0) << setw(statsColumnMap["Work_units"]) << stats_workUnits;
    statsStream << scientific << setprecision(3) << setw(statsColumnMap["Work_unit_steps"]) << (double)stats_workUnitsSteps;
    statsStream << scientific << setprecision(3) << setw(statsColumnMap["Work_unit_time"]) << stats_workUnitsTime;
    statsStream << scientific << setprecision(3) << setw(statsColumnMap["Steps_per_sec"]) << stats_workUnitsSteps/stats_workUnitsTime;
    return statsStream.str();
}

void Slot::getStatsFromFinishedWorkUnit(const lm::message::FinishedWorkUnit& msg)
{
    // collect slot performance stats for getSlotStatistic
    stats_workUnits++;
    stats_workUnitsSteps += msg.steps();
    stats_workUnitsTime += msg.run_time();
}

void Slot::resetSlotStatistics()
{
    stats_workUnits = 0;
    stats_workUnitsSteps = 0;
    stats_workUnitsTime = 0.0;
}

void Slot::printSlotStatistics() const
{
    Print::printf(Print::INFO, "Slot status");
    Print::printf(Print::INFO, Slot::getSlotStatisticsHeader().c_str());
    Print::printf(Print::INFO, Slot::getSlotStatisticsHeaderBreak().c_str());
    Print::printf(Print::INFO, getSlotStatistics().c_str());
}

}
}
