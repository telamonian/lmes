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

#include "lm/ClassFactory.h"
#include "lm/Print.h"
#include "lm/io/LatticeTimeSeries.pb.h"
#include "lm/io/ConsoleOutputWriter.h"
#include "lm/io/OutputWriter.h"


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

void ConsoleOutputWriter::processLatticeTimeSeries(const lm::io::LatticeTimeSeries& data)
{
    // Print the output into the buffer.
    memset(buffer, 0, BUFFER_SIZE+1);
    int offset=snprintf(buffer,BUFFER_SIZE,"--------------------------------------------------------------------------------\n");
    for (int i=0; i<data.number_entries(); i++)
    {
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Time: %10.3f\n",data.time(i));

        const lm::io::Lattice& l = data.lattice(i);
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
                        if (l.particles_ordering() == lm::io::ROW_MAJOR)
                            i = x*l.lattice_y_size()*l.lattice_z_size()*l.particles_per_site() + y*l.lattice_z_size()*l.particles_per_site() + z*l.particles_per_site() + p;
                        else if (l.particles_ordering() == lm::io::COLUMN_MAJOR)
                            i = p*l.lattice_x_size()*l.lattice_y_size()*l.lattice_z_size() + z*l.lattice_x_size()*l.lattice_y_size() + y*l.lattice_x_size() + x;
                        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%2d%c",particles[i],p<l.particles_per_site()-1?',':' ');
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

void ConsoleOutputWriter::flush()
{
}

void ConsoleOutputWriter::checkpoint()
{
}

}
}
