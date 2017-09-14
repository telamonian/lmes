/*
 * Copyright 2012-2016 Johns Hopkins University
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

#include "lm/ClassFactory.h"
#include "lm/Print.h"
#include "lm/io/LatticeTimeSeries.pb.h"
#include "lm/io/ConsoleOutputWriter.h"
#include "lm/io/OutputWriter.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "robertslab/Types.h"
#include "robertslab/pbuf/NDArraySerializer.h"

using robertslab::pbuf::NDArraySerializer;

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

void ConsoleOutputWriter::processDegreeAdvancementTimeSeries(const lm::io::DegreeAdvancementTimeSeries& data)
{
    // Print the output into the buffer.
    memset(buffer, 0, BUFFER_SIZE+1);
    int offset=snprintf(buffer,BUFFER_SIZE,"--------------------------------------------------------------------------------\n");

    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Trajectory: %lld\n", (long long int)data.trajectory_id());
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Counts: NDArray<type=%d> (", data.counts().data_type());
    for (int i=0; i<data.counts().shape_size(); i++)
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%d,",data.counts().shape(i));
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,") size=%d\n",(int)data.counts().data().size());
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Times: NDArray<type=%d> (", data.times().data_type());
    for (int i=0; i<data.times().shape_size(); i++)
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%d,",data.times().shape(i));
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,") size=%d\n",(int)data.times().data().size());
    ndarray<double>* times=NDArraySerializer::deserializeAllocate<double>(data.times());
    ndarray<uint64_t>* counts=NDArraySerializer::deserializeAllocate<uint64_t>(data.counts());
    for (uint i=0, index=0; i<times->shape[0]; i++)
    {
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%10.3f:",times->get(i));
        for (uint j=0; j<data.counts().shape(1); j++, index++)
            offset+=snprintf(buffer+offset,BUFFER_SIZE-offset," %8llu",counts->get(utuple(i,j)));
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"\n");
    }
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"--------------------------------------------------------------------------------");

    // Print the output to stdout.
    Print::printf(Print::INFO, "ConsoleOutputWriter received degree advancement time series for trajectory %d:\n%s",data.trajectory_id(),buffer);

    // Free the ndarrays.
    if (times != NULL) delete times;
    if (counts != NULL) delete counts;
}

void ConsoleOutputWriter::processFirstPassageTimes(const lm::io::FirstPassageTimes& data)
{
    // Get the data.
    ndarray<uint32_t>* counts = NDArraySerializer::deserializeAllocate<uint32_t>(data.counts());
    ndarray<double>* times = NDArraySerializer::deserializeAllocate<double>(data.first_passage_times());

    // Print the output into the buffer.
    memset(buffer, 0, BUFFER_SIZE+1);
    int offset=snprintf(buffer,BUFFER_SIZE,"--------------------------------------------------------------------------------\n");
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Trajectory: %lld\n", (long long int)data.trajectory_id());
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Species: %d> (", data.species());
    for (uint i=0; i<counts->shape[0]; i++)
    {
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%5d: %10.3f\n",counts->get(i),times->get(i));
    }
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"--------------------------------------------------------------------------------");

    // Print the output to stdout.
    Print::printf(Print::INFO, "ConsoleOutputWriter received first passage times for trajectory %d and species %d:\n%s",data.trajectory_id(),data.species(),buffer);

    // Free the ndarrays.
    if (counts != NULL) delete counts;
    if (times != NULL) delete times;
}

void ConsoleOutputWriter::processSpeciesTimeSeries(const lm::io::SpeciesTimeSeries& data)
{
    // Print the output into the buffer.
    memset(buffer, 0, BUFFER_SIZE+1);
    int offset=snprintf(buffer,BUFFER_SIZE,"--------------------------------------------------------------------------------\n");
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Trajectory: %lld\n", (long long int)data.trajectory_id());
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Counts: NDArray<type=%d> (", data.counts().data_type());
    for (int i=0; i<data.counts().shape_size(); i++)
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%d,",data.counts().shape(i));
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,") size=%d\n",(int)data.counts().data().size());
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Times: NDArray<type=%d> (", data.times().data_type());
    for (int i=0; i<data.times().shape_size(); i++)
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%d,",data.times().shape(i));
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,") size=%d\n",(int)data.times().data().size());
    ndarray<double>* times=NDArraySerializer::deserializeAllocate<double>(data.times());
    ndarray<int32_t>* counts=NDArraySerializer::deserializeAllocate<int32_t>(data.counts());
    for (uint i=0, index=0; i<times->shape[0]; i++)
    {
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%10.3f:",times->get(i));
        for (uint j=0; j<data.counts().shape(1); j++, index++)
            offset+=snprintf(buffer+offset,BUFFER_SIZE-offset," %8d",counts->get(utuple(i,j)));
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"\n");
    }
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"--------------------------------------------------------------------------------");

    // Print the output to stdout.
    Print::printf(Print::INFO, "ConsoleOutputWriter received species time series for trajectory %d:\n%s",data.trajectory_id(),buffer);

    // Free the ndarrays.
    if (counts != NULL) delete counts;
    if (times != NULL) delete times;
}

void ConsoleOutputWriter::processLatticeTimeSeries(const lm::io::LatticeTimeSeries& data)
{
    // Print the output into the buffer.
    memset(buffer, 0, BUFFER_SIZE+1);
    int offset=snprintf(buffer,BUFFER_SIZE,"--------------------------------------------------------------------------------\n");
    ndarray<double>* times=NDArraySerializer::deserializeAllocate<double>(data.times());
    for (int i=0; i<times->shape[0] && i<data.lattices_size(); i++)
    {
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"Time: %10.3f\n",times->get(i));

        ndarray<uint8_t>* particles=NDArraySerializer::deserializeAllocate<uint8_t>(data.lattices(i).particles());
        for (int z=0; z<particles->shape[2]; z++)
        {
            for (int x=0; x<particles->shape[0]; x++)
            {
                for (int y=0; y<particles->shape[1]; y++)
                {
                    for (int p=0; p<particles->shape[3]; p++)
                    {
                        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%2d%c",particles->get(utuple(x,y,z,p)),p<particles->shape[3]-1?',':' ');
                    }
                }
                offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"\n");
            }
            offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"---------------\n");
        }

        // Free the ndarray.
        if (particles != NULL) delete particles;
    }
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"--------------------------------------------------------------------------------");

    // Print the output to stdout.
    Print::printf(Print::INFO, "ConsoleOutputWriter received lattice time series for trajectory %d:\n%s",data.trajectory_id(),buffer);

    // Free the ndarray.
    if (times != NULL) delete times;
}

void ConsoleOutputWriter::processConcentrationsTimeSeries(const lm::io::ConcentrationsTimeSeries& data)
{
    // Print the output into the buffer.
    memset(buffer, 0, BUFFER_SIZE+1);
    int offset=snprintf(buffer,BUFFER_SIZE,"--------------------------------------------------------------------------------\n");
    ndarray<double>* times=NDArraySerializer::deserializeAllocate<double>(data.times());
    for (uint i=0, index=0; i<times->shape[0]; i++)
    {
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"%10.3f:",times->get(i));
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"species=%d, ndarray<%d,%d,%d>=%lu bytes",data.species_id(),data.concentrations(index).shape(0),data.concentrations(index).shape(1),data.concentrations(index).shape(2),data.concentrations(index).data().size());
        offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"\n");
    }
    delete times;
    offset+=snprintf(buffer+offset,BUFFER_SIZE-offset,"--------------------------------------------------------------------------------");

    // Print the output to stdout.
    Print::printf(Print::INFO, "ConsoleOutputWriter received concentration time series for trajectory %d:\n%s",data.trajectory_id(),buffer);

    // Free the ndarrays.
    if (times != NULL) delete times;
}


void ConsoleOutputWriter::flush()
{
}

void ConsoleOutputWriter::checkpoint()
{
}

}
}
