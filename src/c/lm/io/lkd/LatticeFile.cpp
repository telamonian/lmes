/*
 * University of Illinois Open Source License
 * Copyright 2008 Luthey-Schulten Group, 
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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

#include <cstring>
#include <iostream>
#include <vector>
#include <map>
#include "Types.h"
#include "LatticeFile.h"

using std::ostream;
using std::vector;
using std::map;

namespace lk
{

void TableOfContentsEntry::writeHeader(ostream* out, uint32 length) const throw(std::ios::failure)
{
	//Write the entry id.
	out->write((char*)&id, sizeof(id));
	
	// Write the entry length.
	out->write((char*)&length, sizeof(length));
	
	// Write the unused space.
	out->write((char*)unused_header, sizeof(unused_header));
}

TableOfContentsEntry* TableOfContentsEntry::constructFrom(std::istream* in) throw(std::ios::failure)
{
	// Read in the toc header.
	uint32 id;
	in->read((char*)&id, sizeof(id));
	uint32 length;
	in->read((char*)&length, sizeof(length));
	byte unused[8];
	in->read((char*)unused, sizeof(unused));
	
	// Allocate the object based on the id.
	TableOfContentsEntry* entry;
	if (id == FramePositions::ID)
		entry = new FramePositions();
	else if (id == LatticePositions::ID)
		entry = new LatticePositions();
	else if (id == LatticeProperties::ID)
		entry = new LatticeProperties();
	else if (id == FrameTimes::ID)
		entry = new FrameTimes();
	else if (id == LatticeConfigurationTimes::ID)
		entry = new LatticeConfigurationTimes();
	else if (id == ParticleTypes::ID)
		entry = new ParticleTypes();
	else if (id == SiteTypes::ID)
		entry = new SiteTypes();
	else
		throw std::ios::failure(std::string("Invalid file format: unknown toc entry type."));
	
	// Read and return the entry.
	entry->readFrom(in);
	return entry;
}

void FramePositions::writeTo(ostream* out) const throw(std::ios::failure)
{
	// Write the entry header.
	TableOfContentsEntry::writeHeader(out, sizeof(uint64)+(sizeof(uint64)*positions.size()));
	
	// Write the entry data.
	uint64 numberFrames = positions.size();
	out->write((char*)&numberFrames, sizeof(numberFrames));
	for (vector<int64>::const_iterator it=positions.begin(); it != positions.end(); it++)
	{
		uint64 framePosition = *it;
		out->write((char*)&framePosition, sizeof(framePosition));
	}
}

void FramePositions::readFrom(std::istream* in) throw(std::ios::failure)
{
	uint64 count;
	in->read((char*)&count, sizeof(count));
	
	for (uint64 i=0; i<count; i++)
	{
		int64 position;
		in->read((char*)&position, sizeof(position));
		positions.push_back(position);
	}
}

void LatticePositions::writeTo(ostream* out) const throw(std::ios::failure)
{
	// Write the entry header.
	TableOfContentsEntry::writeHeader(out, sizeof(uint64)+(sizeof(uint64)*positions.size()));
	
	// Write the entry data.
	uint64 numberFrames = positions.size();
	out->write((char*)&numberFrames, sizeof(numberFrames));
	for (vector<int64>::const_iterator it=positions.begin(); it != positions.end(); it++)
	{
		uint64 latticePosition = *it;
		out->write((char*)&latticePosition, sizeof(latticePosition));
	}
}

void LatticePositions::readFrom(std::istream* in) throw(std::ios::failure)
{	
	uint64 count;
	in->read((char*)&count, sizeof(count));
	
	for (uint64 i=0; i<count; i++)
	{
		int64 position;
		in->read((char*)&position, sizeof(position));
		positions.push_back(position);
	}
}

void LatticeProperties::writeTo(ostream* out) const throw(std::ios::failure)
{
	// Write the entry header.
	TableOfContentsEntry::writeHeader(out, sizeof(maxParticlesPerSite)+sizeof(maxParticleType)+sizeof(maxSiteType)+sizeof(unused_1));
	
	// Write the entry data.
	out->write((char*)&maxParticlesPerSite, sizeof(maxParticlesPerSite));
	out->write((char*)&maxParticleType, sizeof(maxParticleType));
	out->write((char*)&maxSiteType, sizeof(maxSiteType));
	out->write((char*)&unused_1, sizeof(unused_1));
}

void LatticeProperties::readFrom(std::istream* in) throw(std::ios::failure)
{	
	in->read((char*)&maxParticlesPerSite, sizeof(maxParticlesPerSite));
	in->read((char*)&maxParticleType, sizeof(maxParticleType));
	in->read((char*)&maxSiteType, sizeof(maxSiteType));
	in->read((char*)unused_1, sizeof(unused_1));
}

void FrameTimes::writeTo(ostream* out) const throw(std::ios::failure)
{
	// Write the entry header.
	TableOfContentsEntry::writeHeader(out, sizeof(uint64)+(sizeof(uint64)*times.size()));
	
	// Write the entry data.
	uint64 numberFrames = times.size();
	out->write((char*)&numberFrames, sizeof(numberFrames));
	for (vector<nstime_t>::const_iterator it=times.begin(); it != times.end(); it++)
	{
		uint64 frameTime = (uint64)*it;
		out->write((char*)&frameTime, sizeof(frameTime));
	}
}

void FrameTimes::readFrom(std::istream* in) throw(std::ios::failure)
{
	uint64 count;
	in->read((char*)&count, sizeof(count));
	
	for (uint64 i=0; i<count; i++)
	{
		uint64 time;
		in->read((char*)&time, sizeof(time));
		times.push_back((nstime_t)time);
	}
}

void LatticeConfigurationTimes::writeTo(ostream* out) const throw(std::ios::failure)
{
	// Write the entry header.
	TableOfContentsEntry::writeHeader(out, sizeof(uint64)+(sizeof(uint64)*times.size()));
	
	// Write the entry data.
	uint64 numberFrames = times.size();
	out->write((char*)&numberFrames, sizeof(numberFrames));
	for (vector<nstime_t>::const_iterator it=times.begin(); it != times.end(); it++)
	{
		uint64 frameTime = (uint64)*it;
		out->write((char*)&frameTime, sizeof(frameTime));
	}
}

void LatticeConfigurationTimes::readFrom(std::istream* in) throw(std::ios::failure)
{
	uint64 count;
	in->read((char*)&count, sizeof(count));
	
	for (uint64 i=0; i<count; i++)
	{
		uint64 time;
		in->read((char*)&time, sizeof(time));
		times.push_back((nstime_t)time);
	}
}

void ParticleTypes::writeTo(ostream* out) const throw(std::ios::failure)
{
	// Write the entry header.
	TableOfContentsEntry::writeHeader(out, sizeof(uint32)+(12*sizeof(byte))+(2*sizeof(uint64)*typeCountMap.size()));
	
	// Write the entry data.
	uint32 numberTypes = typeCountMap.size();
	out->write((char*)&numberTypes, sizeof(numberTypes));
	out->write((char*)&unused_1, sizeof(unused_1));
	for (map<uint64,uint64>::const_iterator it=typeCountMap.begin(); it != typeCountMap.end(); it++)
	{
		uint64 type = it->first;
		out->write((char*)&type, sizeof(type));
	}
	for (map<uint64,uint64>::const_iterator it=typeCountMap.begin(); it != typeCountMap.end(); it++)
	{
		uint64 count = it->second;
		out->write((char*)&count, sizeof(count));
	}
}

void ParticleTypes::readFrom(std::istream* in) throw(std::ios::failure)
{	
	uint32 numberTypes;
	in->read((char*)&numberTypes, sizeof(numberTypes));
	in->read((char*)unused_1, sizeof(unused_1));
	
	vector<uint64> types;
	for (uint64 i=0; i<numberTypes; i++)
	{
		uint64 type;
		in->read((char*)&type, sizeof(type));
		types.push_back(type);
	}
	for (uint64 i=0; i<numberTypes; i++)
	{
		uint64 count;
		in->read((char*)&count, sizeof(count));
		typeCountMap[types[i]] = count;
	}
}

void SiteTypes::writeTo(ostream* out) const throw(std::ios::failure)
{
	// Write the entry header.
	TableOfContentsEntry::writeHeader(out, sizeof(uint32)+(12*sizeof(byte))+(2*sizeof(uint64)*typeCountMap.size()));
	
	// Write the entry data.
	uint32 numberTypes = typeCountMap.size();
	out->write((char*)&numberTypes, sizeof(numberTypes));
	out->write((char*)&unused_1, sizeof(unused_1));
	for (map<uint64,uint64>::const_iterator it=typeCountMap.begin(); it != typeCountMap.end(); it++)
	{
		uint64 type = it->first;
		out->write((char*)&type, sizeof(type));
	}
	for (map<uint64,uint64>::const_iterator it=typeCountMap.begin(); it != typeCountMap.end(); it++)
	{
		uint64 count = it->second;
		out->write((char*)&count, sizeof(count));
	}
}

void SiteTypes::readFrom(std::istream* in) throw(std::ios::failure)
{	
	uint32 numberTypes;
	in->read((char*)&numberTypes, sizeof(numberTypes));
	in->read((char*)unused_1, sizeof(unused_1));
	
	vector<uint64> types;
	for (uint64 i=0; i<numberTypes; i++)
	{
		uint64 type;
		in->read((char*)&type, sizeof(type));
		types.push_back(type);
	}
	for (uint64 i=0; i<numberTypes; i++)
	{
		uint64 count;
		in->read((char*)&count, sizeof(count));
		typeCountMap[types[i]] = count;
	}
}


}

ostream& operator<<(ostream& stream, const lk::TableOfContentsEntry& entry)
{
	entry.writeTo(&stream);
	return stream;
}

ostream& operator<<(ostream& stream, const lk::TableOfContentsEntry* entry)
{
	entry->writeTo(&stream);
	return stream;
}
