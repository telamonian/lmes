/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 *               University of Illinois at Urbana-Champaign
 *               http://www.scs.uiuc.edu/~schulten
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
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
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
 * Author(s): Elijah Roberts
 */

#include <string>
#include <map>
#include "lm/Print.h"
#include "SimulationParameters.pb.h"
#include "lm/io/SimulationParameters.h"

using std::string;
using std::map;

namespace lm {
namespace io {

SimulationParameters::SimulationParameters():
parameters(),
msg(),
serialized(false)
{
}

SimulationParameters::SimulationParameters(const map<string,string> & parameters):
parameters(parameters),
msg(),
serialized(false)
{
}

SimulationParameters::SimulationParameters(const lm::message::SimulationParameters & msg):
parameters(),
msg(msg),
serialized(false)
{
}

SimulationParameters::~SimulationParameters()
{
}

string & SimulationParameters::operator[] (string s)
{
    return parameters[s];
}

int SimulationParameters::ByteSize()
{
    if (!serialized)
    {
        intoMessage();
        serialized = true;
    }
    return msg.ByteSize();
}

bool SimulationParameters::SerializeToArray(void * data, int size)
{
    if (!serialized)
        {
            intoMessage();
            serialized = true;
        }
    return msg.SerializeToArray(data, size);
}

bool SimulationParameters::ParseFromArray(const void* data, int size)
{
    msg.ParseFromArray(data, size);
    fromMessage();
}

map<string,string> SimulationParameters::getParameters()
{
    return parameters;
}

void SimulationParameters::fromMessage()
{
    for (int i=0; i<msg.key_size() && i<msg.value_size(); i++)
    {
        parameters[msg.key(i)] = msg.value(i);
    }
}

void SimulationParameters::intoMessage()
{
    msg.Clear();
    for (map<string,string>::iterator it=parameters.begin(); it != parameters.end(); it++)
    {
        msg.add_key(it->first);
        msg.add_value(it->second);
    }
}

}
}
