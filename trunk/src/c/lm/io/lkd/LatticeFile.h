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

#ifndef LK_LATTICEFILE_H_
#define LK_LATTICEFILE_H_

#include <cstring>
#include <ostream>
#include <vector>
#include <map>
#include "Types.h"

namespace lk
{

// Structures for reading and writing file sections.
#define FileHeader_Size 				112
#define FileHeader_Magic 				"LKCL"
struct FileHeader {
	char	fileMagic[4];
	uint32	byteOrderMagic;
	uint32	version;
	byte	unused_1[4];
	uint64	xSize;
	uint64	ySize;
	uint64	zSize;
	uint64	siteToSiteDistance;
	uint64	tableOfContentsPosition;
	uint64	firstFramePosition;
	uint64	firstLatticePosition;
	byte	unused_2[40];
};

#define TableOfContents_Size 			16
#define TableOfContents_Magic 			"TOCS"
struct TableOfContents {
	char	magic[4];
	byte	unused_1[4];
	uint64	numEntries;
};

#define FrameHeader_Size 				112
#define FrameHeader_Magic 				"FHED"
struct FrameHeader {
	char	magic[4];
	byte	unused_1[4];
	uint64	time;
	uint32	method;
	byte	unused_2[4];
	uint64	nextFramePosition;
	uint64	tableOfContentsPosition;
	byte	unused_3[72];
};

#define FrameDataM2_Size 				16
#define FrameDataM2_Magic 				"FD02"
struct FrameDataM2 {
	char	magic[4];
	byte	unused_1[4];
	uint8	bitsPerParticleCount;
	uint8	bitsPerParticleType;
	byte	unused_2[6];
};

#define FrameDataM3_Size 				32
#define FrameDataM3_Magic 				"FD03"
struct FrameDataM3 {
	char	magic[4];
	byte	unused_1[4];
	uint64	numberSites;
	uint8	bitsPerSiteIndex;
	uint8	bitsPerParticleCount;
	uint8	bitsPerParticleType;
	byte	unused_2[13];
};

#define LatticeHeader_Size 				112
#define LatticeHeader_Magic 			"LHED"
struct LatticeHeader {
	char	magic[4];
	byte	unused_1[4];
	uint64	time;
	uint32	method;
	byte	unused_2[4];
	uint64	nextLatticePosition;
	uint64	tableOfContentsPosition;
	byte	unused_3[72];
};

#define LatticeDataM2_Size 				16
#define LatticeDataM2_Magic 			"LD02"
struct LatticeDataM2 {
	char	magic[4];
	byte	unused_1[4];
	uint8	bitsPerSiteType;
	byte	unused_2[7];
};


/* The TOC Entries.*/

// File TOC entries.

class TableOfContentsEntry {
public:
	const uint32 	id;
	byte			unused_header[8];
	TableOfContentsEntry(uint32 id):id(id) {memset(unused_header,0,sizeof(unused_header));}
	virtual ~TableOfContentsEntry() {}
	virtual void writeHeader(std::ostream* out, uint32 length) const throw(std::ios::failure);
	virtual void writeTo(std::ostream* out) const throw(std::ios::failure)=0;
	virtual void readFrom(std::istream* in) throw(std::ios::failure)=0;
	static TableOfContentsEntry* constructFrom(std::istream* in) throw(std::ios::failure);
};

class FramePositions : public TableOfContentsEntry {
public:
	static const uint32 ID = 			1;
	std::vector<int64> positions;
	FramePositions():TableOfContentsEntry(ID) {}
	virtual ~FramePositions() {}
	virtual void writeTo(std::ostream* out) const throw(std::ios::failure);
	virtual void readFrom(std::istream* in) throw(std::ios::failure);
};

class LatticePositions : public TableOfContentsEntry {
public:
	static const uint32 ID = 			2;
	std::vector<int64> positions;
	LatticePositions():TableOfContentsEntry(ID) {}
	virtual ~LatticePositions() {}
	virtual void writeTo(std::ostream* out) const throw(std::ios::failure);
	virtual void readFrom(std::istream* in) throw(std::ios::failure);
};

class LatticeProperties : public TableOfContentsEntry {
public:
	static const uint32 ID = 			3;
	uint64	maxParticlesPerSite;
	uint64	maxParticleType;
	uint64	maxSiteType;
	byte	unused_1[24];
	LatticeProperties():TableOfContentsEntry(ID),maxParticlesPerSite(0),maxParticleType(0),maxSiteType(0) {memset(unused_1, 0, sizeof(unused_1));}
	virtual ~LatticeProperties() {}
	virtual void writeTo(std::ostream* out) const throw(std::ios::failure);
	virtual void readFrom(std::istream* in) throw(std::ios::failure);
};

class FrameTimes : public TableOfContentsEntry {
public:
	static const uint32 ID = 			4;
	std::vector<nstime_t> times;
	FrameTimes():TableOfContentsEntry(ID) {}
	virtual ~FrameTimes() {}
	virtual void writeTo(std::ostream* out) const throw(std::ios::failure);
	virtual void readFrom(std::istream* in) throw(std::ios::failure);
};

class LatticeConfigurationTimes : public TableOfContentsEntry {
public:
	static const uint32 ID = 			5;
	std::vector<nstime_t> times;
	LatticeConfigurationTimes():TableOfContentsEntry(ID) {}
	virtual ~LatticeConfigurationTimes() {}
	virtual void writeTo(std::ostream* out) const throw(std::ios::failure);
	virtual void readFrom(std::istream* in) throw(std::ios::failure);
};

class ParticleTypes : public TableOfContentsEntry {
public:
	static const uint32 ID = 			101;
	std::map<uint64,uint64> typeCountMap;
	byte	unused_1[12];
	ParticleTypes():TableOfContentsEntry(ID) {memset(unused_1, 0, sizeof(unused_1));}
	virtual ~ParticleTypes() {}
	virtual void writeTo(std::ostream* out) const throw(std::ios::failure);
	virtual void readFrom(std::istream* in) throw(std::ios::failure);
};

struct SiteTypes : public TableOfContentsEntry {
public:
	static const uint32 ID = 			201;
	std::map<uint64,uint64> typeCountMap;
	byte	unused_1[12];
	SiteTypes():TableOfContentsEntry(ID) {memset(unused_1, 0, sizeof(unused_1));}
	virtual ~SiteTypes() {}
	virtual void writeTo(std::ostream* out) const throw(std::ios::failure);
	virtual void readFrom(std::istream* in) throw(std::ios::failure);
};

}

// Stream operators for table of contents entries.
std::ostream& operator<<(std::ostream& out, const lk::TableOfContentsEntry& lattice);
std::ostream& operator<<(std::ostream& out, const lk::TableOfContentsEntry* lattice);

#endif /*LK_LATTICEFILE_H_*/
