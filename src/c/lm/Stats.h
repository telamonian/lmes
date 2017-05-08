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
 * Author(s): Elijah Roberts, Max Klein
 */
#ifndef LM_STATS_H_
#define LM_STATS_H_

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <numeric>
#include <vector>

#include "lm/Types.h"

/*
 * - gives the index (of a sorted array) of the smallest member of the 100*a percentile
 */
inline size_t percentileIndex(double a, size_t size)
{
    return static_cast<size_t>(ceil(a * size));
}

/*
 * - classs that implements an iterative, numerically stable algorithm for calculating sample variance of a stream of samples, one at a time
 *     - has the advantage of not needing to keep every sample value in memory
 * - used as:
 *     StreamingVariance sv;
 *
 *     sv.push_back(17.0);
 *     sv.push_back(19.0);
 *     sv.push_back(24.0);
 *
 *     double variance = sv.var();
 *
 * - modified from https://www.johndcook.com/blog/standard_deviation/ and https://www.johndcook.com/blog/skewness_kurtosis/
 *     - lifted from from AOCP, Donal Knuth. Vol 2, page 232, 3rd edition
 */
class StreamingVariance
{
public:
    StreamingVariance(): _count(0),_sum(0.0),_mean(0.0),_rawVariance(0.0)
    {
    }

    void clear()
    {
        _count = 0;
        _sum = _mean = _rawVariance = 0.0;
    }

    // See Knuth TAOCP vol 2, 3rd edition, page 232 for complete description of algorithm
    void push_back(double x)
    {
        double oldMean;

        // update _count and _sum
        _count++;
        _sum += x;

        // store the old value of _mean, since we'll need it for updating _s
        oldMean = _mean;

        // update _mean
        _mean = oldMean + (x - oldMean)/_count;

        // update _rawVariance
        _rawVariance = _rawVariance + (x - oldMean)*(x - _mean);

//        double delta, delta_n, term1;
//
//        samples.push_back(x);
//        _sum += x;
//
//        long long n1 = _count;
//        _count++;
//        delta = x - _mean;
//        delta_n = delta / _count;
//        term1 = delta * delta_n * n1;
//        _mean += delta_n;
//        _rawVariance += term1;
    }

    long long count() const
    {
        return _count;
    }

    double sum() const
    {
        return _sum;
    }

    double mean() const
    {
        return _mean;
    }

    double variance() const
    {
        return _rawVariance/(_count - 1.0);
    }

    double standardDeviation() const
    {
        return sqrt(variance());
    }

    static StreamingVariance* combine(StreamingVariance* combined, const StreamingVariance& a, const StreamingVariance& b)
    {
        // update _rawVariance first, since _mean is used in the expression
        combined->_rawVariance = a._rawVariance + b._rawVariance + pow((b._mean - a._mean), 2) * a._count * b._count / (a._count + b._count);
        combined->_mean = (a._count*a._mean + b._count*b._mean) / (a._count + b._count);

        combined->_sum = a._sum + b._sum;
        combined->_count = a._count + b._count;

        return combined;

//        long long combinedCount = (a._count + b._count);
//        double delta = b._mean - a._mean;
//        double delta2 = delta*delta;
//        double delta3 = delta*delta2;
//        double delta4 = delta2*delta2;
//
//        combined->_mean = (a._count*a._mean + b._count*b._mean) / combinedCount;
//
//        combined->_rawVariance = a._rawVariance + b._rawVariance +
//                      delta2 * a._count * b._count / combinedCount;
//
//        combined->_sum = a._sum + b._sum;
//        combined->_count = combinedCount;
//        
//        return combined;
    }

    friend StreamingVariance operator+(const StreamingVariance& a, const StreamingVariance& b);
    StreamingVariance& operator+=(const StreamingVariance &rhs);

//    std::vector<double> samples;

private:
    long long _count;
    double _sum, _mean, _rawVariance;
};

inline StreamingVariance operator+(const StreamingVariance& a, const StreamingVariance& b)
{
    StreamingVariance combined;
    StreamingVariance::combine(&combined, a, b);

    return combined;
}

inline StreamingVariance& StreamingVariance::operator+=(const StreamingVariance& rhs)
{
    return *combine(this, *this, rhs);
}

class StreamingStats
{
public:
    StreamingStats(): n(0),M1(0.0),M2(0.0),M3(0.0),M4(0.0)
    {
    }

    void clear()
    {
        n = 0;
        M1 = M2 = M3 = M4 = 0.0;
    }

    void push_back(double x)
    {
        double delta, delta_n, delta_n2, term1;

        long long n1 = n;
        n++;
        delta = x - M1;
        delta_n = delta / n;
        delta_n2 = delta_n * delta_n;
        term1 = delta * delta_n * n1;
        M1 += delta_n;
        M4 += term1 * delta_n2 * (n*n - 3*n + 3) + 6 * delta_n2 * M2 - 4 * delta_n * M3;
        M3 += term1 * delta_n * (n - 2) - 3 * delta_n * M2;
        M2 += term1;
    }

    long long count() const
    {
        return n;
    }

    double mean() const
    {
        return M1;
    }

    double variance() const
    {
        return M2/(n-1.0);
    }

    double standardDeviation() const
    {
        return sqrt(variance());
    }

    double skewness() const
    {
        return sqrt(double(n)) * M3/ pow(M2, 1.5);
    }

    double kurtosis() const
    {
        return double(n)*M4 / (M2*M2) - 3.0;
    }

    friend StreamingStats operator+(const StreamingStats a, const StreamingStats b);
    StreamingStats& operator+=(const StreamingStats &rhs);

private:
    long long n;
    double M1, M2, M3, M4;
};

inline StreamingStats operator+(const StreamingStats a, const StreamingStats b)
{
    StreamingStats combined;

    combined.n = a.n + b.n;

    double delta = b.M1 - a.M1;
    double delta2 = delta*delta;
    double delta3 = delta*delta2;
    double delta4 = delta2*delta2;

    combined.M1 = (a.n*a.M1 + b.n*b.M1) / combined.n;

    combined.M2 = a.M2 + b.M2 +
                  delta2 * a.n * b.n / combined.n;

    combined.M3 = a.M3 + b.M3 +
                  delta3 * a.n * b.n * (a.n - b.n)/(combined.n*combined.n);
    combined.M3 += 3.0*delta * (a.n*b.M2 - b.n*a.M2) / combined.n;

    combined.M4 = a.M4 + b.M4 + delta4*a.n*b.n * (a.n*a.n - a.n*b.n + b.n*b.n) /
                                (combined.n*combined.n*combined.n);
    combined.M4 += 6.0*delta2 * (a.n*a.n*b.M2 + b.n*b.n*a.M2)/(combined.n*combined.n) +
                   4.0*delta*(a.n*b.M3 - b.n*a.M3) / combined.n;

    return combined;
}

inline StreamingStats& StreamingStats::operator+=(const StreamingStats& rhs)
{
    StreamingStats combined = *this + rhs;
    *this = combined;
    return *this;
}

#endif // LM_STATS_H_
