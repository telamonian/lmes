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

#include <string>
#include <fstream>
#include <list>
#include <map>
#include "Types.h"
#include "LKMath.h"
#include "obitstream.h"
#include "Lattice.h"
#include "LatticeFile.h"
#include "LatticeWriter.h"

using std::list;
using std::map;

namespace lk
{

LatticeWriter::LatticeWriter(std::string filename, Lattice* lattice)
throw(std::ios::failure)
:lattice(lattice),outfile(NULL),frameDataMethod(2),maxParticleType(0),writePerFrameCounts(false)
{
	init(filename.c_str(), lattice);
}

LatticeWriter::LatticeWriter(const char* filename, Lattice* lattice)
throw(std::ios::failure)
:lattice(lattice),outfile(NULL),frameDataMethod(2),maxParticleType(0),writePerFrameCounts(false)
{
	init(filename, lattice);
}

void LatticeWriter::init(const char* filename, Lattice* lattice)
throw(std::ios::failure)
{
	//Make sure the structure sizes are correct on this platform.
	if (CHAR_BIT != 8 || sizeof(FileHeader) != FileHeader_Size || sizeof(TableOfContents) != TableOfContents_Size || sizeof(FrameHeader) != FrameHeader_Size || sizeof(FrameDataM2) != FrameDataM2_Size || sizeof(FrameDataM3) != FrameDataM3_Size || sizeof(LatticeHeader) != LatticeHeader_Size || sizeof(LatticeDataM2) != LatticeDataM2_Size)
		throw Exception("File structure sizes are not valid on this architecture.");
	
	// Update any info we are tracking about the lattice.
	latticeProperties.maxParticlesPerSite = lattice->getMaxParticlesPerSite();
	latticeProperties.maxParticleType = lattice->getMaxParticle();
	latticeProperties.maxSiteType = lattice->getMaxSite();
	
	// Open the file for writing.
	outfile = new std::ofstream(filename, std::ios::out|std::ios::binary|std::ios::trunc);
	if (outfile == NULL || outfile->fail())
		throw std::ios::failure(std::string("Error opening file for writing: ")+filename);
	
	// Use exception iostream handling.
	outfile->exceptions(std::ios::failbit|std::ios::badbit);
	
	// Write the file header.
	writeFileHeader();
}

LatticeWriter::~LatticeWriter()
throw(std::ios::failure)
{
	//Close the file, if it is still open.
	close();
}

void LatticeWriter::setFrameDataMethod(uint frameDataMethod)
{
	if (frameDataMethod != 2 && frameDataMethod != 3)
		throw InvalidArgException("The specified frame data method is not supported.");
	this->frameDataMethod = frameDataMethod;
}

void LatticeWriter::setMaxParticleType(lattice_particle_t maxParticleType)
{
	latticeProperties.maxParticleType = maxParticleType;
}

void LatticeWriter::setWritePerFrameCounts(bool writePerFrameCounts)
{
	this->writePerFrameCounts = writePerFrameCounts;
}

void LatticeWriter::addTableOfContents()
throw(std::ios::failure)
{
	// See if the file is still open.
	if (outfile != NULL)
	{
		// Write the table of contents.
		uint64 tocPosition = outfile->tellp();
		list<TableOfContentsEntry*> tocEntries;
		tocEntries.push_back(&framePositions);
		tocEntries.push_back(&latticePositions);
		tocEntries.push_back(&latticeProperties);
		tocEntries.push_back(&maxParticleCounts);
		tocEntries.push_back(&maxSiteCounts);
		tocEntries.push_back(&frameTimes);
		tocEntries.push_back(&latticeTimes);
		writeTableOfContents(tocEntries);
		
		// Write the location of the table of contents.
		FileHeader fileHeader;
		writeFilePosition((uint64)((size_t)&fileHeader.tableOfContentsPosition-(size_t)&fileHeader), tocPosition);
	}
}

void LatticeWriter::close()
throw(std::ios::failure)
{
	// See if the file is still open.
	if (outfile != NULL)
	{
		// Add a table of contents, if necessary.
		addTableOfContents();

		// Close the file and delete the object.
		outfile->close();
		delete outfile;
		outfile = NULL;
	}
}

void LatticeWriter::writeAlignmentPadding()
throw(std::ios::failure)
{
	byte paddingData[16];
	memset(&paddingData,0,sizeof(paddingData));
	uint paddingCount = 16U-(outfile->tellp())%16U;
	if (paddingCount < 16U) outfile->write((char*)paddingData, (size_t)paddingCount);
}

void LatticeWriter::writeFilePosition(uint64 positionAtWhichToWrite, uint64 positionToWrite)
throw(std::ios::failure)
{
	uint64 currentPosition = outfile->tellp();
	outfile->seekp(positionAtWhichToWrite);
	outfile->write((char*)&positionToWrite, sizeof(positionToWrite));
	outfile->seekp(currentPosition);	
}

void LatticeWriter::writeCurrentFilePosition(uint64 positionAtWhichToWrite)
throw(std::ios::failure)
{
	// Move to the place to write the current frame position and write it.
	writeFilePosition(positionAtWhichToWrite, outfile->tellp());
}

void LatticeWriter::writeFileHeader()
throw(std::ios::failure)
{
	FileHeader fileHeader;
	memset(&fileHeader, 0, sizeof(fileHeader));
	
	//Fill in the file header.
	strncpy(fileHeader.fileMagic, FileHeader_Magic, sizeof(fileHeader.fileMagic));
	fileHeader.byteOrderMagic = 0x04030201U;
	fileHeader.version = 1;
	fileHeader.xSize = lattice->getXSize();
	fileHeader.ySize = lattice->getYSize();
	fileHeader.zSize = lattice->getZSize();
	fileHeader.siteToSiteDistance = lattice->getSpacing();
	fileHeader.tableOfContentsPosition = 0;
	fileHeader.firstFramePosition = 0;
	fileHeader.firstLatticePosition = 0;
	
	// Write the file header.
	outfile->write((char*)&fileHeader, sizeof(fileHeader));	
}

void LatticeWriter::writeTableOfContents(list<TableOfContentsEntry*> entries)
throw(std::ios::failure)
{
	TableOfContents toc;
	memset(&toc, 0, sizeof(toc));
	
	//Write the toc header.
	strncpy(toc.magic, TableOfContents_Magic, sizeof(toc.magic));
	toc.numEntries = entries.size();
	outfile->write((char*)&toc, sizeof(toc));
	
	// Write each entry with alignment padding.
	for (list<TableOfContentsEntry*>::iterator it=entries.begin(); it != entries.end(); it++)
	{
		*outfile << *it;
		writeAlignmentPadding();
	}
}

void LatticeWriter::writeFrameHeader(nstime_t time)
throw(std::ios::failure)
{
	FrameHeader frameHeader;
	memset(&frameHeader, 0, sizeof(frameHeader));
	
	// Write the frame header.
	strncpy(frameHeader.magic, FrameHeader_Magic, sizeof(frameHeader.magic));
	frameHeader.time = time;
	frameHeader.method = frameDataMethod;
	frameHeader.nextFramePosition = 0;
	frameHeader.tableOfContentsPosition = 0;
	outfile->write((char*)&frameHeader, sizeof(frameHeader));	
}

void LatticeWriter::writeFrameData(ParticleTypes* particleTypes)
throw(std::ios::failure,InvalidArgException)
{
	if (frameDataMethod == 2)
		writeFrameDataM2(particleTypes);
	else if (frameDataMethod == 3)
		writeFrameDataM3(particleTypes);
	else
		throw InvalidArgException("The selected frame data method is not supported.");
}

void LatticeWriter::writeFrameDataM2(ParticleTypes* particleTypes)
throw(std::ios::failure)
{
	FrameDataM2 dataHeader;
	memset(&dataHeader, 0, sizeof(dataHeader));
	
	//Fill in the data header.
	strncpy(dataHeader.magic, FrameDataM2_Magic, sizeof(dataHeader.magic));
	dataHeader.bitsPerParticleCount = log2(latticeProperties.maxParticlesPerSite)+1;
	dataHeader.bitsPerParticleType = log2(latticeProperties.maxParticleType)+1;
	
	// Write the data header.
	outfile->write((char*)&dataHeader, sizeof(dataHeader));
	
	// Write the frame data.
	obitstream outbits(outfile, BIT_BUFFER_SIZE);
	for (lattice_size_t z=0; z<lattice->getZSize(); z++)
	{
		for (lattice_size_t y=0; y<lattice->getYSize(); y++)
		{
			for (lattice_size_t x=0; x<lattice->getXSize(); x++)
			{
				uint particleCount = lattice->getOccupancy(x,y,z);
				outbits.write_bits(particleCount, dataHeader.bitsPerParticleCount);
				for (uint i=0; i<particleCount; i++)
				{
					// Get the particle.
					lattice_particle_t particle = lattice->getParticle(x,y,z,i);
					
					// Write out the particle bits.
					outbits.write_bits(particle, dataHeader.bitsPerParticleType);
					
					// Increment the count for this particle type.
					if (particleTypes->typeCountMap.count((uint64)particle) == 0)
						particleTypes->typeCountMap[(uint64)particle] = 1;
					else
						particleTypes->typeCountMap[(uint64)particle]++;
				}
			}
		}
	}
	outbits.flush();
	
	// Make sure the file is aligned after the data section.
	writeAlignmentPadding();
}

void LatticeWriter::writeFrameDataM3(ParticleTypes* particleTypes)
throw(std::ios::failure)
{
	FrameDataM3 dataHeader;
	memset(&dataHeader, 0, sizeof(dataHeader));
	
	//Fill in the data header.
	strncpy(dataHeader.magic, FrameDataM3_Magic, sizeof(dataHeader.magic));
	dataHeader.numberSites = 0;
	dataHeader.bitsPerSiteIndex = log2((lattice->getXSize()*lattice->getYSize()*lattice->getZSize())-1)+1;
	dataHeader.bitsPerParticleCount = log2(latticeProperties.maxParticlesPerSite)+1;
	dataHeader.bitsPerParticleType = log2(latticeProperties.maxParticleType)+1;
	
	// Write the data header.
	uint64 dataHeaderPosition = outfile->tellp();
	outfile->write((char*)&dataHeader, sizeof(dataHeader));
	
	// Write the frame data.
	obitstream outbits(outfile, BIT_BUFFER_SIZE);
	for (lattice_size_t z=0, index=0; z<lattice->getZSize(); z++)
	{
		for (lattice_size_t y=0; y<lattice->getYSize(); y++)
		{
			for (lattice_size_t x=0; x<lattice->getXSize(); x++, index++)
			{
				uint particleCount = lattice->getOccupancy(x,y,z);
				if (particleCount > 0)
				{
					dataHeader.numberSites++;
					outbits.write_bits(index, dataHeader.bitsPerSiteIndex);
					outbits.write_bits(particleCount, dataHeader.bitsPerParticleCount);
					for (uint i=0; i<particleCount; i++)
					{
						// Get the particle.
						lattice_particle_t particle = lattice->getParticle(x,y,z,i);
						
						// Write out the particle bits.
						outbits.write_bits(particle, dataHeader.bitsPerParticleType);
						
						// Increment the count for this particle type.
						if (particleTypes->typeCountMap.count((uint64)particle) == 0)
							particleTypes->typeCountMap[(uint64)particle] = 1;
						else
							particleTypes->typeCountMap[(uint64)particle]++;
					}
				}
			}
		}
	}
	outbits.flush();
	
	// Fill in the number of sites we wrote.
	uint64 currentPosition = outfile->tellp();
	outfile->seekp(dataHeaderPosition+(uint64)((size_t)&dataHeader.numberSites-(size_t)&dataHeader));
	outfile->write((char*)&dataHeader.numberSites, sizeof(dataHeader.numberSites));
	outfile->seekp(currentPosition);	
	
	// Make sure the file is aligned after the data section.
	writeAlignmentPadding();
}

void LatticeWriter::appendFrame(nstime_t time)
throw(std::ios::failure)
{
	// Update the previous frame or file header with the frame position.
	if (framePositions.positions.size() == 0)
	{
		FileHeader fileHeader;
		uint64 updatePosition = (uint64)((size_t)&fileHeader.firstFramePosition-(size_t)&fileHeader);
		writeCurrentFilePosition(updatePosition);
	}
	else
	{
		FrameHeader frameHeader;
		uint64 updatePosition = framePositions.positions.back()+(uint64)((size_t)&frameHeader.nextFramePosition-(size_t)&frameHeader);
		writeCurrentFilePosition(updatePosition);
	}
	
	// Get the current position, which is the start of the current frame.
	uint64 currentFramePosition = outfile->tellp();
	
	// Add the current frame to the list of frame positions.
	framePositions.positions.push_back(currentFramePosition);
	
	// Add the current frame's time to the list of frame times.
	frameTimes.times.push_back(time);
	
	// Write the frame header.
	writeFrameHeader(time);
	
	// Write the frame data.
	ParticleTypes particleCounts;
	writeFrameData(&particleCounts);
	
	// Update our running max counts for the particle types.
	for (map<uint64,uint64>::const_iterator it=particleCounts.typeCountMap.begin(); it != particleCounts.typeCountMap.end(); it++)
	{
		uint64 type = it->first;
		uint64 count = it->second;
		if (maxParticleCounts.typeCountMap.count(type) == 0 || count > maxParticleCounts.typeCountMap[type])
			maxParticleCounts.typeCountMap[type] = count;
	}	
	
	// Write the location of the frame table of contents into the frame header.
	FrameHeader frameHeader;
	uint64 updatePosition = currentFramePosition+(uint64)((size_t)&frameHeader.tableOfContentsPosition-(size_t)&frameHeader);
	writeCurrentFilePosition(updatePosition);
	
	// Write the frame table of contents.
	list<TableOfContentsEntry*> tocEntries;
	if (writePerFrameCounts) tocEntries.push_back(&particleCounts);
	writeTableOfContents(tocEntries);
}

void LatticeWriter::writeLatticeHeader(nstime_t time)
throw(std::ios::failure)
{
	LatticeHeader latticeHeader;
	memset(&latticeHeader, 0, sizeof(latticeHeader));
	
	// Write the frame header.
	strncpy(latticeHeader.magic, LatticeHeader_Magic, sizeof(latticeHeader.magic));
	latticeHeader.time = time;
	latticeHeader.method = 2;
	latticeHeader.nextLatticePosition = 0;
	latticeHeader.tableOfContentsPosition = 0;
	outfile->write((char*)&latticeHeader, sizeof(latticeHeader));	
}

void LatticeWriter::writeLatticeData(SiteTypes* siteTypes)
throw(std::ios::failure)
{
		writeLatticeDataM2(siteTypes);
}

void LatticeWriter::writeLatticeDataM2(SiteTypes* siteTypes)
throw(std::ios::failure)
{
	LatticeDataM2 dataHeader;
	memset(&dataHeader, 0, sizeof(dataHeader));
	
	//Fill in the data header.
	strncpy(dataHeader.magic, LatticeDataM2_Magic, sizeof(dataHeader.magic));
	dataHeader.bitsPerSiteType = log2(latticeProperties.maxSiteType)+1;
	
	// Write the data header.
	outfile->write((char*)&dataHeader, sizeof(dataHeader));
	
	// Write the frame data.
	obitstream outbits(outfile, BIT_BUFFER_SIZE);
	for (lattice_size_t z=0; z<lattice->getZSize(); z++)
	{
		for (lattice_size_t y=0; y<lattice->getYSize(); y++)
		{
			for (lattice_size_t x=0; x<lattice->getXSize(); x++)
			{
				lattice_site_t site = lattice->getSite(x,y,z);
				outbits.write_bits(site, dataHeader.bitsPerSiteType);
				
				// Increment the count for this particle type.
				if (siteTypes->typeCountMap.count((uint64)site) == 0)
					siteTypes->typeCountMap[(uint64)site] = 1;
				else
					siteTypes->typeCountMap[(uint64)site]++;
			}
		}
	}
	outbits.flush();
	
	// Make sure the file is aligned after the data section.
	writeAlignmentPadding();
}

void LatticeWriter::appendLatticeConfiguration(nstime_t time)
throw(std::ios::failure)
{
	// Update the previous lattice or file header with the lattice position.
	if (latticePositions.positions.size() == 0)
	{
		FileHeader fileHeader;
		uint64 updatePosition = (uint64)((size_t)&fileHeader.firstLatticePosition-(size_t)&fileHeader);
		writeCurrentFilePosition(updatePosition);
	}
	else
	{
		LatticeHeader latticeHeader;
		uint64 updatePosition = latticePositions.positions.back()+(uint64)((size_t)&latticeHeader.nextLatticePosition-(size_t)&latticeHeader);
		writeCurrentFilePosition(updatePosition);
	}
	
	// Get the current position, which is the start of the current lattice.
	uint64 currentLatticePosition = outfile->tellp();
	
	// Add the current lattice to the list of lattice positions.
	latticePositions.positions.push_back(currentLatticePosition);
	
	// Add the current lattice's time to the list of lattice times.
	latticeTimes.times.push_back(time);
	
	// Write the lattice header.
	writeLatticeHeader(time);
	
	// Write the lattice data.
	SiteTypes siteCounts;
	writeLatticeData(&siteCounts);
	
	// Update our running max counts for the site types.
	for (map<uint64,uint64>::const_iterator it=siteCounts.typeCountMap.begin(); it != siteCounts.typeCountMap.end(); it++)
	{
		uint64 type = it->first;
		uint64 count = it->second;
		if (maxSiteCounts.typeCountMap.count(type) == 0 || count > maxSiteCounts.typeCountMap[type])
			maxSiteCounts.typeCountMap[type] = count;
	}	
	
	// Write the location of the lattice table of contents into the lattice header.
	LatticeHeader latticeHeader;
	uint64 updatePosition = currentLatticePosition+(uint64)((size_t)&latticeHeader.tableOfContentsPosition-(size_t)&latticeHeader);
	writeCurrentFilePosition(updatePosition);
	
	// Write the lattice table of contents.
	list<TableOfContentsEntry*> tocEntries;
	tocEntries.push_back(&siteCounts);
	writeTableOfContents(tocEntries);
}

}
