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
#ifndef LIMITCHECKING
#define LIMITCHECKING

// standalone version of the limit checking code
// for investigating the assembly produced by various compilers

#include <exception>
using std::exception;

enum LimitType {NONE,
    SPECIES,
    ORDER_PARAMETER};

enum StoppingCondition {MIN,
    MAX};

enum Endpoint {EXCLUDED,
    INCLUDED};

// specializations of check_limit with regards to stoppingCondition and includeEndpoint for the basic min/max limits
#define check_limit_MIN_false(val, limitVal, checkBool) checkBool = (val < limitVal);
#define check_limit_MIN_true(val, limitVal, checkBool) checkBool = (val <= limitVal);
#define check_limit_MAX_false(val, limitVal, checkBool) checkBool = (val > limitVal);
#define check_limit_MAX_true(val, limitVal, checkBool) checkBool = (val >= limitVal);

struct TrajectoryLimit
{
    LimitType type;
    StoppingCondition stoppingCondition;
    bool includeEndpoint;
    int limitID;

    int valueID;
    int ivalue;
    double dvalue;
    int uvalue;
};

// globals
TrajectoryLimit* tl;
int* speciesCounts;
double* orderParameterValues;

TrajectoryLimit* setup()
{
    TrajectoryLimit* tl = new TrajectoryLimit[2];

    tl[0].type = SPECIES;
    tl[0].stoppingCondition = MIN;
    tl[0].includeEndpoint = true;
    tl[0].limitID = 0;
    tl[0].valueID = 0;
    tl[0].ivalue = 19;

    tl[1].type = ORDER_PARAMETER;
    tl[1].stoppingCondition = MAX;
    tl[1].includeEndpoint = false;
    tl[1].limitID = 1;
    tl[1].valueID = 0;
    tl[1].dvalue = 2.9;

    speciesCounts = new int[2];
    speciesCounts[0] = 234;
    speciesCounts[1] = 4;

    orderParameterValues = new double[1];
    orderParameterValues[0] = 1.23;
}

bool isTrajectoryOutsideLimits()
{
    bool limitReached;
    for (int i=0; i<2; i++)
    {
        TrajectoryLimit* limits = setup();
        TrajectoryLimit& l = limits[i];
        limitReached = false;

        switch (l.type)
        {
        case NONE: throw exception(); break;

        case SPECIES:
            switch (l.stoppingCondition)
            {
            case MIN:
                if (l.includeEndpoint)
                {
                    check_limit_MIN_true(speciesCounts[l.valueID], l.ivalue, limitReached)
                }
                else
                {
                    check_limit_MIN_false(speciesCounts[l.valueID], l.ivalue, limitReached)
                }
                break;
            case MAX:
                if (l.includeEndpoint)
                {
                    check_limit_MAX_true(speciesCounts[l.valueID], l.ivalue, limitReached)
                }
                else
                {
                    check_limit_MAX_false(speciesCounts[l.valueID], l.ivalue, limitReached)
                }
                break;
            } break;

        case ORDER_PARAMETER:
            switch (l.stoppingCondition)
            {
            case MIN:
                if (l.includeEndpoint)
                {
                    check_limit_MIN_true(orderParameterValues[l.valueID], l.dvalue, limitReached)
                }
                else
                {
                    check_limit_MIN_false(orderParameterValues[l.valueID], l.dvalue, limitReached)
                }
                break;
            case MAX:
                if (l.includeEndpoint)
                {
                    check_limit_MAX_true(orderParameterValues[l.valueID], l.dvalue, limitReached)
                }
                else
                {
                    check_limit_MAX_false(orderParameterValues[l.valueID], l.dvalue, limitReached)
                }
                break;
            }
            break;
        default:
            break;
        }
    }
    return limitReached;
}

#endif /* LIMITCHECKING */