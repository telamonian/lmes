/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
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
 * Author(s): Elijah Roberts, Max Klein
 */
#ifndef LM_TILING_TILING
#define LM_TILING_TILING

#include "lm/io/Tilings.pb.h"
#include "lm/Types.h"

namespace lm {
namespace tiling {

typedef google::protobuf::RepeatedField<double>::const_iterator EdgeIterator;

class Tiling
{
public:
    Tiling();
    virtual ~Tiling();
    virtual void init(const lm::io::Tilings::Tiling& tilingRef);

    // iterators (over the edges)
    EdgeIterator begin() {return tilingBuf->edges().begin();}
    EdgeIterator end() {return tilingBuf->edges().end();}

    // getters
    lm::io::Tilings::Arrangement getArrangement() const;
    double getEdge(uint edgeIndex);
    uint getEdgesCount() const {return tilingBuf->edges_size();}
    uint getID() {return tilingBuf->id();}
    uint getOrderParameterID() {return tilingBuf->order_parameter_id();}
    uint getTileIndex(double opVal);    // get the index of the tile for making a histogram based on the tiling
//    double getAscendingLimit(uint edgeIndex);
//    double getDescendingLimit(uint edgeIndex);

    void reverse();

    // setters
    void setArrangement(lm::io::Tilings::Arrangement arrangement);
    void setOrderParameterID(uint opID) {tilingBuf->set_order_parameter_id(opID);}

protected:
    lm::io::Tilings::Tiling* tilingBuf;
};

class TilingAxial : public Tiling
{
public:
    static bool registered;
    static bool registerClass();
    static void* allocateObject();

    TilingAxial();
    virtual ~TilingAxial() {}
    virtual void init(const lm::io::Tilings::Tiling& tilingRef);
};

}
}

#endif /* LM_TILING_TILING */
