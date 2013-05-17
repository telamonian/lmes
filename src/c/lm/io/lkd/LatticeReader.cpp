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
#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include "Types.h"
#include "ibitstream.h"
#include "Lattice.h"
#include "LatticeFile.h"
#include "LatticeReader.h"

using std::list;
using std::map;
using std::vector;

namespace lk
{

LatticeReader::LatticeReader(std::string filename) throw(std::ios::failure)
:infile(NULL),latticeSize(0,0,0),latticeSpacing(0),latticeProperties(NULL),framePositions(NULL),latticePositions(NULL),frameTimes(NULL),latticeTimes(NULL),particleTypes(NULL),siteTypes(NULL)
{
	init(filename.c_str());
}

LatticeReader::LatticeReader(const char* filename) throw(std::ios::failure)
:infile(NULL),latticeSize(0,0,0),latticeSpacing(0),latticeProperties(NULL),framePositions(NULL),latticePositions(NULL),frameTimes(NULL),latticeTimes(NULL),particleTypes(NULL),siteTypes(NULL)
{
	init(filename);
}

void LatticeReader::init(const char* filename)
throw(std::ios::failure)
{
	//Make sure the structure sizes are correct on this platform.
	if (CHAR_BIT != 8 || sizeof(FileHeader) != FileHeader_Size || sizeof(TableOfContents) != TableOfContents_Size || sizeof(FrameHeader) != FrameHeader_Size || sizeof(FrameDataM2) != FrameDataM2_Size || sizeof(FrameDataM3) != FrameDataM3_Size || sizeof(LatticeHeader) != LatticeHeader_Size || sizeof(LatticeDataM2) != LatticeDataM2_Size)
		throw Exception("File structure sizes are not valid on this architecture.");
		
	// Open the file for writing.
	infile = new std::ifstream(filename, std::ios::in|std::ios::binary);
	if (infile == NULL || infile->fail())
		throw std::ios::failure(std::string("Error opening file for reading: ")+filename);
	
	// Use exception iostream handling.
	infile->exceptions(std::ios::eofbit|std::ios::failbit|std::ios::badbit);
	
	// Read the file header.
	FileHeader fileHeader;
	readFileHeader(&fileHeader);
	
	// Store any needed info from the file header.
	latticeSize.x = fileHeader.xSize;
	latticeSize.y = fileHeader.ySize;
	latticeSize.z = fileHeader.zSize;
	latticeSpacing = fileHeader.siteToSiteDistance;
	
	// See if the file has a table of contents.
	list<TableOfContentsEntry*> entries;
	if (fileHeader.tableOfContentsPosition != 0)
	{
		// Load the file table of contents.
		infile->seekg(fileHeader.tableOfContentsPosition);
		readTableOfContents(&entries);
	}
	else
	{
		// The file table of contents was missing, try and rebuild it by parsing the file.
		buildTableOfContents(&entries, fileHeader.firstLatticePosition, fileHeader.firstFramePosition);
	}
	
	// Got through each entry and save it if we understand it.	
	for (list<TableOfContentsEntry*>::iterator it=entries.begin(); it != entries.end(); it++)
	{
		TableOfContentsEntry* entry = *it;
		if (entry->id == LatticeProperties::ID)
			latticeProperties = static_cast<LatticeProperties*>(entry);
		else if (entry->id == FramePositions::ID)
			framePositions = static_cast<FramePositions*>(entry);
		else if (entry->id == LatticePositions::ID)
			latticePositions = static_cast<LatticePositions*>(entry);
		else if (entry->id == FrameTimes::ID)
			frameTimes = static_cast<FrameTimes*>(entry);
		else if (entry->id == LatticeConfigurationTimes::ID)
			latticeTimes = static_cast<LatticeConfigurationTimes*>(entry);
		else if (entry->id == ParticleTypes::ID)
			particleTypes = static_cast<ParticleTypes*>(entry);
		else if (entry->id == SiteTypes::ID)
			siteTypes = static_cast<SiteTypes*>(entry);
		else
			delete entry;
	}
	
	// Make sure we read every required toc entry.
	if (latticeProperties == NULL || framePositions == NULL || latticePositions == NULL || particleTypes == NULL || siteTypes == NULL)
		throw std::ios::failure(std::string("Invalid file format: missing file table of contents entries"));
}

LatticeReader::~LatticeReader()
throw(std::ios::failure)
{
	//Close the file, if it is still open.
	close();
}

lattice_coord_t LatticeReader::getLatticeSize() const
{
	return latticeSize;
}

nmdist_t LatticeReader::getLatticeSpacing() const
{
	return latticeSpacing;
}

uint LatticeReader::getMaxParticlesPerSite() const
{
	return latticeProperties->maxParticlesPerSite;
}

lattice_particle_t LatticeReader::getMaxParticleType() const
{
	return latticeProperties->maxParticleType;
}

lattice_site_t LatticeReader::getMaxSiteType() const
{
	return latticeProperties->maxSiteType;
}

const map<uint64,uint64> LatticeReader::getMaxParticleCounts() const
{
	return particleTypes->typeCountMap;
}

const map<uint64,uint64> LatticeReader::getMaxSiteCounts() const
{
	return siteTypes->typeCountMap;
}

uint64 LatticeReader::getNumberFrames() const
{
	if (framePositions == NULL) return 0;
	return framePositions->positions.size();	
}

const vector<nstime_t> LatticeReader::getFrameTimes() const
{
	if (frameTimes == NULL) return vector<nstime_t>();
	return frameTimes->times;
}

uint64 LatticeReader::getNumberLatticeConfigurations() const
{
	if (latticePositions == NULL) return 0;
	return latticePositions->positions.size();
}

const vector<nstime_t> LatticeReader::getLatticeConfigurationTimes() const
{
	if (latticeTimes == NULL) return vector<nstime_t>();
	return latticeTimes->times;
}

void LatticeReader::close()
throw(std::ios::failure)
{
	// Free any table of contents entries.
	if (latticeProperties != NULL)
	{
		delete latticeProperties;
		latticeProperties = NULL;
	}
	if (framePositions != NULL)
	{
		delete framePositions;
		framePositions = NULL;
	}
	if (latticePositions != NULL)
	{
		delete latticePositions;
		latticePositions = NULL;
	}
	if (particleTypes != NULL)
	{
		delete particleTypes;
		particleTypes = NULL;
	}
	if (siteTypes != NULL)
	{
		delete siteTypes;
		siteTypes = NULL;
	}
	
	// See if the file is still open.
	if (infile != NULL)
	{		
		//Close the file and delete the object.
		infile->close();
		delete infile;
		infile = NULL;
	}
}

void LatticeReader::readAlignmentPadding() const throw(std::ios::failure)
{
	uint paddingCount = 16U-(infile->tellg())%16U;
	if (paddingCount < 16U) infile->seekg(paddingCount, std::ios_base::cur);
}

void LatticeReader::readFileHeader(FileHeader* fileHeader) throw(std::ios::failure)
{
	// Read and validate the file header.
	infile->read((char*)fileHeader, sizeof(*fileHeader));
	if (strncmp(FileHeader_Magic, fileHeader->fileMagic, 4) != 0)
		throw std::ios::failure(std::string("Invalid file format: bad file magic"));
	if (fileHeader->byteOrderMagic != 0x04030201U)
		throw std::ios::failure(std::string("Invalid file format: incompatible byte order"));
	if (fileHeader->version != 1U)
		throw std::ios::failure(std::string("Invalid file format: incompatible version"));
}

void LatticeReader::readTableOfContents(std::list<TableOfContentsEntry*>* entries) throw(std::ios::failure)
{
	//Read the toc header.
	TableOfContents toc;
	infile->read((char*)&toc, sizeof(toc));
	if (strncmp(TableOfContents_Magic, toc.magic, 4) != 0)
		throw std::ios::failure(std::string("Invalid file format: bad toc magic"));
	
	// Read the entries.
	for (uint64 i=0; i < toc.numEntries; i++)
	{
		entries->push_back(TableOfContentsEntry::constructFrom(infile));
		readAlignmentPadding();
	}
}

void LatticeReader::buildTableOfContents(std::list<TableOfContentsEntry*>* entries, uint64 firstLatticePosition, uint64 firstFramePosition) throw(std::ios::failure)
{
	/*
	TableOfContentsEntry* entry = *it;
	if (entry->id == LatticeProperties::ID)
		latticeProperties = static_cast<LatticeProperties*>(entry);
	else if (entry->id == ParticleTypes::ID)
		particleTypes = static_cast<ParticleTypes*>(entry);
	else if (entry->id == SiteTypes::ID)
		siteTypes = static_cast<SiteTypes*>(entry);
	*/

	// Print a warning message so we know the file was not complete.
	std::cout << "WARNING: file was incomplete, reconstructing table of contents." << std::endl;

	// Walk through all of the lattice configurations.
	LatticePositions* latticePositions = new LatticePositions();
	LatticeConfigurationTimes* latticeTimes = new LatticeConfigurationTimes();
	LatticeHeader latticeHeader;
	latticeHeader.nextLatticePosition = firstLatticePosition;
	do
	{
		// Save the position of the lattice.
		latticePositions->positions.push_back(latticeHeader.nextLatticePosition);

		// Move to the next lattice configuration.
		infile->seekg(latticeHeader.nextLatticePosition);

		// Read the header.
		readLatticeHeader(&latticeHeader);

		// Save any needed info from the header.
		latticeTimes->times.push_back(latticeHeader.time);
	}
	while (latticeHeader.nextLatticePosition != 0);
	entries->push_back(latticePositions);
	entries->push_back(latticeTimes);
	std::cout << "Recovered " << latticePositions->positions.size() << " lattice configurations" << std::endl;

	// Walk through all of the frame configurations.
	FramePositions* framePositions = new FramePositions();
	FrameTimes* frameTimes = new FrameTimes();
	FrameHeader frameHeader;
	uint64 framePosition = firstFramePosition;
	do
	{
		// Move to the frame configuration.
		infile->seekg(framePosition);

		// Read the header.
		readFrameHeader(&frameHeader);

		// Make sure the TOC position was written, so that we know the frame write completed.
		if (frameHeader.tableOfContentsPosition == 0)
			break;

		// Save the position of the frame.
		framePositions->positions.push_back(framePosition);

		// Save any needed info from the header.
		frameTimes->times.push_back(frameHeader.time);

		// Get the position of the next frame.
		framePosition = frameHeader.nextFramePosition;
	}
	while (framePosition != 0);
	entries->push_back(framePositions);
	entries->push_back(frameTimes);
	std::cout << "Recovered " << framePositions->positions.size() << " frame configurations" << std::endl;

	LatticeProperties* latticeProperties = new LatticeProperties();
	latticeProperties->maxParticlesPerSite = 6;
	latticeProperties->maxParticleType = 7;
	latticeProperties->maxSiteType = 7;
	entries->push_back(latticeProperties);
	ParticleTypes* particleTypes = new ParticleTypes();
	entries->push_back(particleTypes);
	SiteTypes* siteTypes = new SiteTypes();
	entries->push_back(siteTypes);
}

void LatticeReader::loadFrame(uint64 frameIndex, Lattice* lattice, nstime_t* time) const throw(std::ios::failure)
{
	// Make sure the arguments are valid.
	if (framePositions == NULL || frameIndex >= framePositions->positions.size())
		throw std::ios::failure(std::string("Error loading frame: invalid frame index."));
	if (lattice == NULL || lattice->getXSize() != latticeSize.x || lattice->getYSize() != latticeSize.y || lattice->getZSize() != latticeSize.z || lattice->getSpacing() != latticeSpacing)
		throw std::ios::failure(std::string("Error loading frame: incompatible lattice."));
	if (lattice->getMaxParticlesPerSite() < latticeProperties->maxParticlesPerSite || lattice->getMaxParticle() < latticeProperties->maxParticleType)
		throw std::ios::failure(std::string("Error loading frame: insufficient lattice capability."));
	
	// Read the frame header.
	FrameHeader frameHeader;
	infile->seekg(framePositions->positions[frameIndex]);
	readFrameHeader(&frameHeader);
	
	// Return the time.
	if (time != NULL) *time = frameHeader.time;
	
	// Read the frame data.
	readFrameData(frameHeader.method, lattice);
}

void LatticeReader::readFrameHeader(FrameHeader* frameHeader) const throw(std::ios::failure)
{
	// Read and validate the frame header.
	infile->read((char*)frameHeader, sizeof(*frameHeader));
	if (strncmp(FrameHeader_Magic, frameHeader->magic, 4) != 0)
		throw std::ios::failure(std::string("Invalid file format: bad magic"));
}

void LatticeReader::readFrameData(uint32 frameDataMethod, Lattice* lattice) const throw(std::ios::failure,InvalidArgException)
{
	if (frameDataMethod == 2)
		readFrameDataM2(lattice);
	else if (frameDataMethod == 3)
		readFrameDataM3(lattice);
	else
		throw InvalidArgException("The frame data method is not supported.");
}

void LatticeReader::readFrameDataM2(Lattice* lattice) const throw(std::ios::failure)
{
	// Read the frame data header.
	FrameDataM2 dataHeader;
	infile->read((char*)&dataHeader, sizeof(dataHeader));
	if (strncmp(FrameDataM2_Magic, dataHeader.magic, 4) != 0)
		throw std::ios::failure(std::string("Invalid file format: bad magic"));
	
	// Remove any particles from the lattice.
	lattice->removeAllParticles();
	
	// Read the frame data.
	ibitstream inbits(infile, BIT_BUFFER_SIZE);
	for (lattice_size_t z=0; z<lattice->getZSize(); z++)
	{
		for (lattice_size_t y=0; y<lattice->getYSize(); y++)
		{
			for (lattice_size_t x=0; x<lattice->getXSize(); x++)
			{
				// Read the particle count.
				uint particleCount;
				inbits.read_bits(particleCount, dataHeader.bitsPerParticleCount);
				for (uint i=0; i<particleCount; i++)
				{
					// Read the particle.
					lattice_particle_t particle;
					inbits.read_bits(particle, dataHeader.bitsPerParticleType);
					
					// Add the particle to the lattice.
					lattice->addParticle(x,y,z,particle);
				}
			}
		}
	}
	
	// Make sure we skip over any alignment padding.
	readAlignmentPadding();
}

void LatticeReader::readFrameDataM3(Lattice* lattice) const throw(std::ios::failure)
{
	// Read the frame data header.
	FrameDataM3 dataHeader;
	infile->read((char*)&dataHeader, sizeof(dataHeader));
	if (strncmp(FrameDataM3_Magic, dataHeader.magic, 4) != 0)
		throw std::ios::failure(std::string("Invalid file format: bad magic"));
	
	// Remove any particles from the lattice.
	lattice->removeAllParticles();
	
	// Get some lattice properties.
	lattice_size_t xSize = lattice->getXSize();
	lattice_size_t ySize = lattice->getYSize();
	lattice_size_t xySize = xSize*ySize;
	
	// Read each of the particles.
	ibitstream inbits(infile, BIT_BUFFER_SIZE);
	for (uint64 n=0; n<dataHeader.numberSites; n++)
	{
		lattice_size_t siteIndex;
		inbits.read_bits(siteIndex, dataHeader.bitsPerSiteIndex);
		uint particleCount;
		inbits.read_bits(particleCount, dataHeader.bitsPerParticleCount);
		for (uint i=0; i<particleCount; i++)
		{
			// Read the particle.
			lattice_particle_t particle;
			inbits.read_bits(particle, dataHeader.bitsPerParticleType);
			
			// Figure out the x, y, and z coordinates and add the particle to the lattice.
			lattice_size_t x = siteIndex%xSize;
			lattice_size_t y = (siteIndex/xSize)%ySize;
			lattice_size_t z = siteIndex/xySize;
			lattice->addParticle(x,y,z,particle);
		}
	}
	
	// Make sure we skip over any alignment padding.
	readAlignmentPadding();
}

void LatticeReader::loadLatticeConfiguration(uint64 latticeIndex, Lattice* lattice, nstime_t* time) const throw(std::ios::failure)
{
	// Make sure the arguments are valid.
	if (latticePositions == NULL || latticeIndex >= latticePositions->positions.size())
		throw std::ios::failure(std::string("Error loading lattice: invalid lattice index."));
	if (lattice == NULL || lattice->getXSize() != latticeSize.x || lattice->getYSize() != latticeSize.y || lattice->getZSize() != latticeSize.z || lattice->getSpacing() != latticeSpacing)
		throw std::ios::failure(std::string("Error loading lattice: incompatible lattice."));
	if (lattice->getMaxSite() < latticeProperties->maxSiteType)
		throw std::ios::failure(std::string("Error loading lattice: insufficient lattice capability."));
	
	// Read the lattice header.
	LatticeHeader latticeHeader;
	infile->seekg(latticePositions->positions[latticeIndex]);
	readLatticeHeader(&latticeHeader);
	
	// Return the time.
	if (time != NULL) *time = latticeHeader.time;
	
	// Read the lattice data.
	readLatticeData(latticeHeader.method, lattice);
}

void LatticeReader::readLatticeHeader(LatticeHeader* latticeHeader) const throw(std::ios::failure)
{
	// Read and validate the lattice header.
	infile->read((char*)latticeHeader, sizeof(*latticeHeader));
	if (strncmp(LatticeHeader_Magic, latticeHeader->magic, 4) != 0)
		throw std::ios::failure(std::string("Invalid file format: bad magic"));
}

void LatticeReader::readLatticeData(uint32 latticeDataMethod, Lattice* lattice) const throw(std::ios::failure,InvalidArgException)
{
	if (latticeDataMethod == 2)
		readLatticeDataM2(lattice);
	else
		throw InvalidArgException("The lattice data method is not supported.");
}

void LatticeReader::readLatticeDataM2(Lattice* lattice) const throw(std::ios::failure)
{
	// Read the lattice data header.
	LatticeDataM2 dataHeader;
	infile->read((char*)&dataHeader, sizeof(dataHeader));
	if (strncmp(LatticeDataM2_Magic, dataHeader.magic, 4) != 0)
		throw std::ios::failure(std::string("Invalid file format: bad magic"));
	
	// Read the lattice data.
	ibitstream inbits(infile, BIT_BUFFER_SIZE);
	for (lattice_size_t z=0; z<lattice->getZSize(); z++)
	{
		for (lattice_size_t y=0; y<lattice->getYSize(); y++)
		{
			for (lattice_size_t x=0; x<lattice->getXSize(); x++)
			{
				// Read the site type.
				lattice_site_t site;
				inbits.read_bits(site, dataHeader.bitsPerSiteType);
					
				// Set the site type.
				lattice->setSite(x,y,z,site);
			}
		}
	}
	
	// Make sure we skip over any alignment padding.
	readAlignmentPadding();
}

}
