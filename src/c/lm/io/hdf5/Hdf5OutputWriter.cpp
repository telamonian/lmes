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
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/stat.h>

#include <lm/ClassFactory.h>
#include <lm/Print.h>
#include <lm/main/Globals.h>
#include "lm/io/OutputWriter.h"
#include "lm/io/hdf5/Hdf5OutputWriter.h"

namespace lm {
namespace io {
namespace hdf5 {

using std::stringstream;
using std::string;

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

void Hdf5OutputWriter::processDegreeAdvancementTimeSeries(const lm::io::DegreeAdvancementTimeSeries& data)
{
    // construct the relative path to the group we're storing the degree advancement time series dataset in
    std::stringstream ss;
    ss << "DegreeAdvancementTimeSeries";

    std::string groupRelativePath(ss.str()), valuesDatasetName("Counts"), timesDatasetName("Times");

    setNDArrayReplicate(data.trajectory_id(), groupRelativePath, valuesDatasetName, data.counts());
    setNDArrayReplicate(data.trajectory_id(), groupRelativePath, timesDatasetName, data.times());
}

void Hdf5OutputWriter::processFFluxOutput(const lm::io::FFluxOutput& data)
{
    file->setFFluxOutput(const_cast<lm::io::FFluxOutput*>(&data));
}

void Hdf5OutputWriter::processFirstPassageTimes(const lm::io::FirstPassageTimes& data)
{
    file->setFirstPassageTimes(data.trajectory_id(), (lm::io::FirstPassageTimes*)&data);
}

void Hdf5OutputWriter::processOrderParameterTimeSeries(const lm::io::OrderParameterTimeSeries& data)
{
    std::stringstream ss;
    ss << "OrderParameterTimeSeries" << "/";
    ss << std::setfill('0') << std::setw(2) << '0'; //data.id();

    std::string groupRelativePath(ss.str()), valuesDatasetName("Values"), timesDatasetName("Times");

    setNDArrayReplicate(data.trajectory_id(), groupRelativePath, valuesDatasetName, data.values());
    setNDArrayReplicate(data.trajectory_id(), groupRelativePath, timesDatasetName, data.times());
}

void Hdf5OutputWriter::processOrderParameterFirstPassageTimes(const lm::io::OrderParameterFirstPassageTimes& data)
{
    // construct the relative path to the group we're storing the opfpt datasets in
    std::stringstream ss;
    ss << "OrderParameterFirstPassageTime" << "/";
    ss << std::setfill('0') << std::setw(2) << data.order_parameter_id();

    std::string groupRelativePath(ss.str()), valuesDatasetName("Values"), timesDatasetName("Times");

    setNDArrayReplicate(data.trajectory_id(), groupRelativePath, valuesDatasetName, data.order_parameter_value());
    setNDArrayReplicate(data.trajectory_id(), groupRelativePath, timesDatasetName, data.first_passage_time());
}

void Hdf5OutputWriter::processLatticeTimeSeries(const lm::io::LatticeTimeSeries& data)
{
    file->appendLatticeTimeSeries(data.trajectory_id(), data);
}

void Hdf5OutputWriter::processLimitTracking(const lm::io::LimitTracking& data)
{
    // construct the relative path to the group we're storing the limit tracking datasets in
    std::stringstream ss;
    ss << "LimitTracking" << "/";
    ss << std::setfill('0') << std::setw(2) << data.limit_id();

    std::string groupRelativePath(ss.str());
    std::string degreeAdvancementsDatasetName("DegreeAdvancements"), orderParameterValuesDatasetName("OrderParameterValues"), speciesCountsDatasetName("SpeciesCounts"), timesDatasetName("Times");

    if (data.has_degree_advancements())
    {
        setNDArrayReplicate(data.trajectory_id(), groupRelativePath, degreeAdvancementsDatasetName, data.degree_advancements());
    }

    if (data.has_order_parameter_values())
    {
        setNDArrayReplicate(data.trajectory_id(), groupRelativePath, orderParameterValuesDatasetName, data.order_parameter_values());
    }

    setNDArrayReplicate(data.trajectory_id(), groupRelativePath, speciesCountsDatasetName, data.species_counts());
    setNDArrayReplicate(data.trajectory_id(), groupRelativePath, timesDatasetName, data.times());
}

void Hdf5OutputWriter::processSpeciesCounts(const lm::io::SpeciesCounts& data)
{
    file->appendSpeciesCounts(data.trajectory_id(), (lm::io::SpeciesCounts*)&data);
}

void Hdf5OutputWriter::processSpeciesTimeSeries(const lm::io::SpeciesTimeSeries& data)
{
    file->appendSpeciesTimeSeries(data.trajectory_id(), data);
}

void Hdf5OutputWriter::flush()
{
    file->flush();
}

void Hdf5OutputWriter::checkpoint()
{
    file->checkpoint();
}

void Hdf5OutputWriter::finalize()
{
    OutputWriter::finalize();

    file->close();
    delete file;
    file = NULL;
}

void Hdf5OutputWriter::setNDArrayReplicate(uint64_t replicate, const string& groupRelativePath, const string& datasetName, const robertslab::pbuf::NDArray& ndarray)
{
    if (condenseOutput)
    {
        file->setNDArrayReplicateCondensed(replicate, groupRelativePath, datasetName, ndarray);
    }
    else
    {
        file->setNDArrayReplicate(replicate, groupRelativePath, datasetName, ndarray);
    }
}

void Hdf5OutputWriter::setRecordNamePrefix(const std::string& newRecordNamePrefix)
{
    // call the parent class method
    OutputWriter::setRecordNamePrefix(newRecordNamePrefix);

    // call the hdf5 file class method
    file->setRecordNamePrefix(newRecordNamePrefix);
}

}
}
}
