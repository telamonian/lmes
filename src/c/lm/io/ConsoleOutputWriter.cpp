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

#include <zlib.h>

#include "lm/ClassFactory.h"
#include "lm/Print.h"
#include "lm/io/LatticeTimeSeries.pb.h"
#include "lm/io/ConsoleOutputWriter.h"
#include "lm/io/OutputWriter.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/types/ArrayOrdering.pb.h"
#include "lm/types/Lattice.pb.h"

namespace lm {
namespace io {


bool ConsoleOutputWriter::registered=ConsoleOutputWriter::registerClass();

bool ConsoleOutputWriter::registerClass()
{
    lm::ClassFactory::getInstance().registerClass("lm::io::OutputWriter","lm::io::ConsoleOutputWriter",&ConsoleOutputWriter::allocateObject);
    return true;
}

void* ConsoleOutputWriter::allocateObject()
{
    return new ConsoleOutputWriter();
}

ConsoleOutputWriter::ConsoleOutputWriter()
:buffer(new char[BUFFER_SIZE+1])
{
}

ConsoleOutputWriter::~ConsoleOutputWriter()
{
    if (buffer != NULL) delete[] buffer; buffer = NULL;
}

void ConsoleOutputWriter::initialize()
{
    OutputWriter::initialize();
}

void ConsoleOutputWriter::checkpoint()
{
}

void ConsoleOutputWriter::flush()
{
}

void ConsoleOutputWriter::processGenericMessage(const google::protobuf::Message& data)
{
    memset(buffer, 0, BUFFER_SIZE+1);

    int offset=snprintf(buffer,BUFFER_SIZE,"--------------------------------------------------------------------------------\n");
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset, data.DebugString().c_str());
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"--------------------------------------------------------------------------------");

    Print::printf(Print::INFO, "ConsoleOutputWriter received %s:\n%s", data.GetDescriptor()->name().c_str(), buffer);
}

void ConsoleOutputWriter::processFirstPassageTimes(const lm::io::FirstPassageTimes& data)
{
    // Print the output into the buffer.
    memset(buffer, 0, BUFFER_SIZE+1);
    int offset=snprintf(buffer,BUFFER_SIZE,"--------------------------------------------------------------------------------\n");
    for (int i=0; i<data.number_entries(); i++)
    {
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%5d: %10.3f\n",data.species_count(i),data.first_passage_time(i));
    }
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"--------------------------------------------------------------------------------");

    // Print the output to stdout.
    Print::printf(Print::INFO, "ConsoleOutputWriter received first passage times for trajectory %d and species %d:\n%s",data.trajectory_id(),data.species(),buffer);
}

void ConsoleOutputWriter::processSpeciesCounts(const lm::io::SpeciesCounts& data)
{
    // Print the output into the buffer.
    memset(buffer, 0, BUFFER_SIZE+1);
    int offset=snprintf(buffer,BUFFER_SIZE,"--------------------------------------------------------------------------------\n");
    for (int i=0, index=0; i<data.number_entries(); i++)
    {
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%10.3f:",data.time(i));
        for (int j=0; j<data.number_species(); j++, index++)
            offset+=snprintf(buffer+offset,BUFFER_SIZE-offset," %5d",data.species_count(index));
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"\n");
    }
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"--------------------------------------------------------------------------------");

    // Print the output to stdout.
    Print::printf(Print::INFO, "ConsoleOutputWriter received species counts for trajectory %d:\n%s",data.trajectory_id(),buffer);
}

void ConsoleOutputWriter::processSpeciesTimeSeries(const lm::io::SpeciesTimeSeries& data)
{
    // Print the output into the buffer.
    memset(buffer, 0, BUFFER_SIZE+1);
    int offset=snprintf(buffer,BUFFER_SIZE,"--------------------------------------------------------------------------------\n");
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Trajectory: %lld\n", data.trajectory_id());
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Counts: NDArray<type=%d> (", data.counts().data_type());
    for (int i=0; i<data.counts().shape_size(); i++)
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%d,",data.counts().shape(i));
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,") size=%d\n",(int)data.counts().data().size());
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Times: NDArray<type=%d> (", data.times().data_type());
    for (int i=0; i<data.times().shape_size(); i++)
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%d,",data.times().shape(i));
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,") size=%d\n",(int)data.times().data().size());

    // Extract the data.
    int32_t* counts=NULL;
    if (data.counts().compressed_deflate())
    {
        size_t countsSize = data.counts().shape(0)*data.counts().shape(1)*sizeof(int32_t);
        counts = new int32_t[countsSize];
        const std::string& countsStr = data.counts().data();
        ZLIB_EXCEPTION_CHECK(uncompress((unsigned char *)counts, &countsSize, (unsigned char*)&(countsStr[0]), countsStr.size()));
        if (countsSize != data.counts().shape(0)*data.counts().shape(1)*sizeof(int32_t))
            throw Exception("Error during data decompression, wrong number of bytes returned.");
    }
    else
    {
        const std::string& countsStr = data.counts().data();
        counts = (int32_t*)&(countsStr[0]);
    }
    double* times=NULL;
    if (data.times().compressed_deflate())
    {
        size_t timesSize = data.times().shape(0)*sizeof(double);
        times = new double[timesSize];
        const std::string& timesStr = data.times().data();
        ZLIB_EXCEPTION_CHECK(uncompress((unsigned char *)times, &timesSize, (unsigned char*)&(timesStr[0]), timesStr.size()));
        if (timesSize != data.times().shape(0)*sizeof(double))
            throw Exception("Error during data decompression, wrong number of bytes returned.");
    }
    else
    {
        const std::string& timesStr = data.times().data();
        times = (double*)&(timesStr[0]);
    }

    // Print the counts.
    for (int i=0, index=0; i<data.counts().shape(0); i++)
    {
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%10.3f:",times[i]);
        for (int j=0; j<data.counts().shape(1); j++, index++)
            offset+=snprintf(buffer+offset,BUFFER_SIZE-offset," %5d",counts[index]);
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"\n");
    }

    if (data.counts().compressed_deflate())
        delete[] counts;
    if (data.times().compressed_deflate())
        delete[] times;

    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"--------------------------------------------------------------------------------");

    // Print the output to stdout.
    Print::printf(Print::INFO, "ConsoleOutputWriter received species time series for trajectory %d:\n%s",data.trajectory_id(),buffer);

}

void ConsoleOutputWriter::processLatticeTimeSeries(const lm::io::LatticeTimeSeries& data)
{
    // Print the output into the buffer.
    memset(buffer, 0, BUFFER_SIZE+1);
    int offset=snprintf(buffer,BUFFER_SIZE,"--------------------------------------------------------------------------------\n");
    for (int i=0; i<data.number_entries(); i++)
    {
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Time: %10.3f\n",data.time(i));

        const lm::types::Lattice& l = data.lattice(i);
        const std::string& particles = l.particles();
        for (int z=0; z<l.lattice_z_size(); z++)
        {
            for (int x=0; x<l.lattice_x_size(); x++)
            {
                for (int y=0; y<l.lattice_y_size(); y++)
                {
                    for (int p=0; p<l.particles_per_site(); p++)
                    {
                        int i;
                        if (l.particles_ordering() == lm::types::ROW_MAJOR)
                        {
                            i = x*l.lattice_y_size()*l.lattice_z_size()*l.particles_per_site() + y*l.lattice_z_size()*l.particles_per_site() + z*l.particles_per_site() + p;
                            offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%2d%c",particles[i],p<l.particles_per_site()-1?',':' ');
                        }
                        else if (l.particles_ordering() == lm::types::COLUMN_MAJOR)
                        {
                            i = p*l.lattice_x_size()*l.lattice_y_size()*l.lattice_z_size() + z*l.lattice_x_size()*l.lattice_y_size() + y*l.lattice_x_size() + x;
                            offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%2d%c",particles[i],p<l.particles_per_site()-1?',':' ');
                        }
                    }
                }
                offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"\n");
            }
            offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"---------------\n");
        }
    }
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"--------------------------------------------------------------------------------");

    // Print the output to stdout.
    Print::printf(Print::INFO, "ConsoleOutputWriter received lattice time series for trajectory %d:\n%s",data.trajectory_id(),buffer);
}

}
}
