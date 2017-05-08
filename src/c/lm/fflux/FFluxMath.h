/*
 * University of Illinois Open Source License
 * Copyright 2008-2012 Luthey-Schulten Group,
 * Copyright 2012-2016 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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
#ifndef LM_FFLUX_MATH_H_
#define LM_FFLUX_MATH_H_

#include <cmath>
#include <iterator>
#include <limits>
#include <vector>

#include "lm/Math.h"
#include "lm/Stats.h"
#include "lm/Types.h"
#include "lm/VectorMath.h"

// calculates the variance of a Bernouli random variable based on its probability (ie their expected values)
// can also take vectors
template <typename MaybeVector>
inline MaybeVector bernouliVariance(const MaybeVector& probability)
{
    return probability * (1 - probability);
}

// exact formula for calculating the variance of a product of random variables. Based on their individual expected values and variances
inline double productVarianceExact(const std::vector<double> expected, const std::vector<double> variance)
{
//    std::vector<double> expectedSquared(expected*expected);
    std::vector<double> expected2 = pow(expected, 2);

    return prod(variance + expected2) - prod(expected2);
}

// see https://en.wikipedia.org/wiki/Binomial_proportion_confidence_interval#Agresti-Coull_Interval for more details
template <typename MaybeVector0, typename MaybeVector1>
inline MaybeVector0 bernouliCIAgrestiCoullLowerBound(const MaybeVector0 probability, const MaybeVector1 trials, double confidence)
{
    double z = normalZ(confidence);
    double z2 = pow(z, 2);
    MaybeVector1 n = trials + z2;
    MaybeVector0 p = (trials*probability + .5*z2)/n;

    // change p - z*pow... to p + z*pow for the upper bound of the confidence interval instead
    return p - z*pow((p*(1 - p)/n), .5);
};

#endif /* LM_FFLUX_MATH_H_ */