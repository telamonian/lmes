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

#ifndef LK_LATTICEWRITER_H_
#define LK_LATTICEWRITER_H_

#include <fstream>
#include <list>
#include "Types.h"
#include "LatticeFile.h"

namespace lk
{

class Lattice;

class LatticeWriter
{
public:
	LatticeWriter(std::string filename, Lattice* lattice) throw(std::ios::failure);
	LatticeWriter(const char* filename, Lattice* lattice) throw(std::ios::failure);
	virtual ~LatticeWriter() throw(std::ios::failure);
	virtual void setFrameDataMethod(uint frameDataMethod);
	virtual void setMaxParticleType(lattice_particle_t maxParticleType);
	virtual void setWritePerFrameCounts(bool);
	virtual void appendFrame(nstime_t time) throw(std::ios::failure);
	virtual void appendLatticeConfiguration(nstime_t time) throw(std::ios::failure);
	virtual void addTableOfContents() throw(std::ios::failure);
	virtual void close() throw(std::ios::failure);
	
protected:
	Lattice* lattice;
	std::ofstream* outfile;
	uint frameDataMethod;
	lattice_particle_t maxParticleType;
	FramePositions framePositions;
	FrameTimes frameTimes;
	LatticePositions latticePositions;
	LatticeConfigurationTimes latticeTimes;
	LatticeProperties latticeProperties;
	ParticleTypes maxParticleCounts;
	SiteTypes maxSiteCounts;
	bool writePerFrameCounts;
	
	static const int BIT_BUFFER_SIZE=1024;
	
protected: 
	virtual void writeAlignmentPadding() throw(std::ios::failure);
	virtual void writeFilePosition(uint64 positionAtWhichToWrite, uint64 positionToWrite) throw(std::ios::failure);
	virtual void writeCurrentFilePosition(uint64 positionAtWhichToWrite) throw(std::ios::failure);
	virtual void writeFileHeader() throw(std::ios::failure);
	virtual void writeTableOfContents(std::list<TableOfContentsEntry*> entries) throw(std::ios::failure);
	virtual void writeFrameHeader(nstime_t time) throw(std::ios::failure);
	virtual void writeFrameData(ParticleTypes* particleTypes) throw(std::ios::failure,InvalidArgException);
	virtual void writeFrameDataM2(ParticleTypes* particleTypes) throw(std::ios::failure);
	virtual void writeFrameDataM3(ParticleTypes* particleTypes) throw(std::ios::failure);
	virtual void writeLatticeHeader(nstime_t time) throw(std::ios::failure);
	virtual void writeLatticeData(SiteTypes* siteTypes) throw(std::ios::failure);
	virtual void writeLatticeDataM2(SiteTypes* siteTypes) throw(std::ios::failure);
	
private:
	void init(const char* filename, Lattice* lattice) throw(std::ios::failure);
	
	friend class LatticeWriterTest;
};

}

#endif /*LK_LATTICEWRITER_H_*/
