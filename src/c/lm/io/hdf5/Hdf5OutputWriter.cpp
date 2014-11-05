/*
 * University of Illinois Open Source License
 * Copyright 2012-2014 Roberts Group,
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

#include <sys/stat.h>

#include <lm/ClassFactory.h>
#include <lm/Print.h>
#include "lm/io/OutputWriter.h"
#include "lm/io/hdf5/Hdf5OutputWriter.h"


namespace lm {
namespace io {
namespace hdf5 {


bool Hdf5OutputWriter::registered=Hdf5OutputWriter::registerClass();

bool Hdf5OutputWriter::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::io::OutputWriter","lm::io::hdf5::Hdf5OutputWriter",&Hdf5OutputWriter::allocateObject);
    return true;
}

void* Hdf5OutputWriter::allocateObject()
{
    return new Hdf5OutputWriter();
}

Hdf5OutputWriter::Hdf5OutputWriter()
:file(NULL)
{
}

Hdf5OutputWriter::~Hdf5OutputWriter()
{
    if (file != NULL) delete file; file = NULL;
}

void Hdf5OutputWriter::initialize()
{
    OutputWriter::initialize();

    // Make sure we have an output filename.
    if (outputFilename == "") throw Exception("Invalid output filename",outputFilename.c_str());

    // If the file doesn't exists, create it.
    struct stat fileStats;
    if (stat(outputFilename.c_str(), &fileStats) != 0)
    {
        Hdf5File::create(outputFilename);
    }

    // Open the file.
    file = new Hdf5File(outputFilename);
}

void Hdf5OutputWriter::processFirstPassageTimes(const lm::io::FirstPassageTimes& data)
{
    file->setFirstPassageTimes(data.trajectory_id(), (lm::io::FirstPassageTimes*)&data);
}

void Hdf5OutputWriter::processSpeciesCounts(const lm::io::SpeciesCounts& data)
{
    file->appendSpeciesCounts(data.trajectory_id(), (lm::io::SpeciesCounts*)&data);
}

void Hdf5OutputWriter::processLatticeTimeSeries(const lm::io::LatticeTimeSeries& data)
{
    file->appendLatticeTimeSeries(data.trajectory_id(), data);
}

void Hdf5OutputWriter::processFFluxOutput(const lm::io::FFluxOutput& data)
{
    file->appendFFluxOutput(const_cast<lm::io::FFluxOutput*>(&data));
}

void Hdf5OutputWriter::flush()
{
    file->flush();
}

void Hdf5OutputWriter::finalize()
{
    OutputWriter::finalize();

    file->close();
    delete file;
    file = NULL;
}

}
}
}
