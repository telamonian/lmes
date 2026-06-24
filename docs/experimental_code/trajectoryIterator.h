/*
 * University of Illinois Open Source License
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
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
 * Author(s): Max Klein
 */
#ifndef TRAJECTORYITERATOR
#define TRAJECTORYITERATOR

// example of a custom iterator
// iterator over map of lm::trajectory::Trajectory objects that reaches inside the Trajectory instances and returns lm::io::TrajectoryState

#include <iterator>
#include <map>
#include <string>

#include "lm/input/Input.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/input/SimulationPhase.pb.h"
#include "lm/io/TrajectoryState.pb.h"
#include "lm/message/FinishedWorkUnit.pb.h"
#include "lm/message/RunWorkUnit.pb.h"
#include "lm/message/WorkUnitStatus.pb.h"
#include "lm/protowrap/Repeated.h"
#include "lm/trajectory/Trajectory.h"
#include "lm/Types.h"

using std::map;
using std::string;

namespace lm {
namespace trajectory {

typedef std::map<uint64_t,lm::trajectory::Trajectory*> TrajectoryMap;

template <typename T> class _TrajectoryStateIteratorBase : public std::iterator<std::forward_iterator_tag, lm::io::TrajectoryState>
{
public:
    _TrajectoryStateIteratorBase(T tmit): tmit(tmit) {}
    _TrajectoryStateIteratorBase(const _TrajectoryStateIteratorBase& tsit): tmit(tsit.tmit) {}
    _TrajectoryStateIteratorBase& operator++() {++tmit;return *this;}
    _TrajectoryStateIteratorBase operator++(int) {_TrajectoryStateIteratorBase tmp(*this); operator++(); return tmp;}
    bool operator==(const _TrajectoryStateIteratorBase& rhs) {return tmit==rhs.tmit;}
    bool operator!=(const _TrajectoryStateIteratorBase& rhs) {return tmit!=rhs.tmit;}
protected:
    T tmit;
};
template <typename T> class _TrajectoryStateIterator : public _TrajectoryStateIteratorBase<T>
{
public:
    _TrajectoryStateIterator(T tmit): _TrajectoryStateIteratorBase(tmit) {}
    _TrajectoryStateIterator(const _TrajectoryStateIterator& tsit): _TrajectoryStateIteratorBase(tsit.tmit) {}
    lm::io::TrajectoryState& operator*() {return *tmit->second->getStateMutable();}
};
template <typename T> class _TrajectoryStateConstIterator : public _TrajectoryStateIteratorBase<T>
{
public:
    _TrajectoryStateConstIterator(T tmit): _TrajectoryStateIteratorBase(tmit) {}
    _TrajectoryStateConstIterator(const _TrajectoryStateConstIterator& tsit): _TrajectoryStateIteratorBase(tsit.tmit) {}
    lm::io::TrajectoryState& operator*() {return tmit->second->getState();}
};
typedef _TrajectoryStateIterator<TrajectoryMap::iterator> TrajectoryStateIterator;
typedef _TrajectoryStateConstIterator<TrajectoryMap::const_iterator> TrajectoryStateConstIterator;

}
}

#endif /* TRAJECTORYITERATOR */