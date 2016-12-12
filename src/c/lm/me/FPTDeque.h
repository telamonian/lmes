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
#ifndef LM_ME_FPTDEQUE_H
#define LM_ME_FPTDEQUE_H

#include <deque>

#include "lm/Types.h"
#include "lm/io/FirstPassageTimes.pb.h"


namespace lm {
namespace me {


class FPTDeque
{
public:
    FPTDeque();
    virtual ~FPTDeque();
    FPTDeque& operator=(const FPTDeque& a);
    void insert(int value, double time);
    void deserializeFrom(const lm::io::FirstPassageTimes& fpt);
    void serializeInto(lm::io::FirstPassageTimes* fpt);

public:
    uint64_t trajectoryId;
    uint32_t species;
    int minValue;
    int maxValue;

protected:
    std::deque<double> fpts;
};

}
}

#endif // LM_ME_FPTDEQUE_H
