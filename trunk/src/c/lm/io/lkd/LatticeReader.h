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

#ifndef LK_LATTICEREADER_H_
#define LK_LATTICEREADER_H_

#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include "Types.h"
#include "LatticeFile.h"

namespace lk
{

class Lattice;

class LatticeReader
{
public:
	LatticeReader(std::string filename) throw(std::ios::failure);
	LatticeReader(const char* filename) throw(std::ios::failure);
	virtual ~LatticeReader() throw(std::ios::failure);
	virtual lattice_coord_t getLatticeSize() const;
	virtual nmdist_t getLatticeSpacing() const;
	virtual uint getMaxParticlesPerSite() const;
	virtual lattice_particle_t getMaxParticleType() const;
	virtual lattice_site_t getMaxSiteType() const;
	virtual const std::map<uint64,uint64> getMaxParticleCounts() const;
	virtual const std::map<uint64,uint64> getMaxSiteCounts() const;
	
	virtual uint64 getNumberFrames() const;	
	virtual const std::vector<nstime_t> getFrameTimes() const;
	virtual void loadFrame(uint64 frameIndex, Lattice* lattice, nstime_t* time=NULL) const throw(std::ios::failure);
	
	virtual uint64 getNumberLatticeConfigurations() const;
	virtual const std::vector<nstime_t> getLatticeConfigurationTimes() const;
	virtual void loadLatticeConfiguration(uint64 latticeIndex, Lattice* lattice, nstime_t* time=NULL) const throw(std::ios::failure);
	
	virtual void close() throw(std::ios::failure);
	
protected:
	std::ifstream* infile;
	lattice_coord_t latticeSize;
	nmdist_t latticeSpacing;
	LatticeProperties* latticeProperties;
	FramePositions* framePositions;
	LatticePositions* latticePositions;
	FrameTimes* frameTimes;
	LatticeConfigurationTimes* latticeTimes;
	ParticleTypes* particleTypes;
	SiteTypes *siteTypes;
	
	static const int BIT_BUFFER_SIZE=16;
	
protected: 
	virtual void readAlignmentPadding() const throw(std::ios::failure);
	virtual void readFileHeader(FileHeader* fileHeader) throw(std::ios::failure);
	virtual void readTableOfContents(std::list<TableOfContentsEntry*>* entries) throw(std::ios::failure);
	virtual void buildTableOfContents(std::list<TableOfContentsEntry*>* entries, uint64 firstLatticePosition, uint64 firstFramePosition) throw(std::ios::failure);
	virtual void readFrameHeader(FrameHeader* frameHeader) const throw(std::ios::failure);
	virtual void readFrameData(uint32 frameDataMethod, Lattice* lattice) const throw(std::ios::failure,InvalidArgException);
	virtual void readFrameDataM2(Lattice* lattice) const throw(std::ios::failure);
	virtual void readFrameDataM3(Lattice* lattice) const throw(std::ios::failure);
	virtual void readLatticeHeader(LatticeHeader* latticeHeader) const throw(std::ios::failure);
	virtual void readLatticeData(uint32 latticeDataMethod, Lattice* lattice) const throw(std::ios::failure,InvalidArgException);
	virtual void readLatticeDataM2(Lattice* lattice) const throw(std::ios::failure);
	
private:
	void init(const char* filename) throw(std::ios::failure);
	
	friend class LatticeReaderTest;
};

}

#endif /*LK_LATTICEREADER_H_*/
