/*
 * Copyright 2016 Johns Hopkins University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Developed by: Roberts Group
 *               Johns Hopkins University
 *               http://biophysics.jhu.edu/roberts/
 *
 * Author(s): Elijah Roberts, Max Klein
 */

#include <deque>

#include "lm/Types.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/me/FPTDeque.h"
#include "robertslab/pbuf/NDArraySerializer.h"

using robertslab::pbuf::NDArraySerializer;

namespace lm {
namespace me {

FPTDeque::FPTDeque()
{
}

FPTDeque::~FPTDeque()
{

}

FPTDeque& FPTDeque::operator=(const FPTDeque& a)
{
    trajectoryId = a.trajectoryId;
    species = a.species;
    minValue = a.minValue;
    maxValue = a.maxValue;
    fpts = a.fpts;
    return *this;
}


void FPTDeque::insert(int value, double time)
{
    if (fpts.size() == 0)
    {
        minValue = value;
        maxValue = value;
        fpts.push_back(time);
    }
    else
    {
        while (value < minValue)
        {
            fpts.push_front(time);
            minValue--;
        }
        while (value > maxValue)
        {
            fpts.push_back(time);
            maxValue++;
        }
    }
}

void FPTDeque::deserializeFrom(const lm::io::FirstPassageTimes& fpt)
{
    trajectoryId = fpt.trajectory_id();
    species = fpt.species();

    // Deserialize the message.
    ndarray<int32_t>* counts = NDArraySerializer::deserialize<int32_t>(fpt.counts());
    ndarray<double>* times = NDArraySerializer::deserialize<double>(fpt.first_passage_times());

    if (counts->shape.len != 1 || counts->shape != times->shape)
        throw lm::Exception("first passage times must have the same number of counts and times (deserialize)");

    // Get the min and max values.
    minValue = counts->get(0);
    maxValue = counts->get(counts->size-1);

    // Create a list of the times.
    for (uint i=0; i<times->size; i++)
    {
        fpts.push_back(times->get(i));
    }

    // Free the ndarray objects.
    delete times;
    delete counts;
}

void FPTDeque::serializeInto(lm::io::FirstPassageTimes* fpt)
{
    // Clear the message.
    fpt->Clear();

    // Set the trajectory and species.
    fpt->set_trajectory_id(trajectoryId);
    fpt->set_species(species);

    // Check for consistency.
    if (fpts.size() == 0)
        throw lm::Exception("first passage times must not be empty (serialize)");
    if (maxValue-minValue+1 != (int)fpts.size())
        throw lm::Exception("first passage times must have the same number of counts and times (serialize)", minValue, maxValue, fpts.size());

    // Create ndarray objects with the counts and times.
    ndarray<int32_t> counts(utuple(fpts.size()));
    ndarray<double> times(utuple(fpts.size()));
    int32_t i=0, value=minValue;
    for (std::deque<double>::iterator it=fpts.begin(); it != fpts.end(); it++,i++,value++)
    {
        counts[i] = value;
        times[i] = *it;
    }

    // Set the ndarrays in the message.
    NDArraySerializer::serializeInto(fpt->mutable_counts(), counts);
    NDArraySerializer::serializeInto(fpt->mutable_first_passage_times(), times);
}

}
}
