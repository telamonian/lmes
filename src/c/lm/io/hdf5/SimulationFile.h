/*
 * University of Illinois Open Source License
 * Copyright 2010 Luthey-Schulten Group,
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

#ifndef LM_IO_HDF5_SIMULATIONFILE_H_
#define LM_IO_HDF5_SIMULATIONFILE_H_

#include <string>
#include <map>
#include <vector>
#include "lm/Exceptions.h"
#include "lm/Types.h"
#include "lm/io/hdf5/HDF5.h"

namespace lm {

namespace rdme{
class Lattice;
}

namespace io {

class DiffusionModel;
class ReactionModel;
class Lattice;
class ParameterValues;
class SpeciesCounts;
class SpatialModel;
class FirstPassageTimes;

namespace hdf5 {

using std::string;
using std::map;
using std::vector;
using lm::IOException;

class IOException;

class SimulationFile
{
public:
    static const uint MIN_VERSION;
    static const uint CURRENT_VERSION;
    static const uint MAX_REACTION_RATE_CONSTANTS;
    static const uint MAX_SHAPE_PARAMETERS;

public:
    static bool isValidFile(const string filename) throw(IOException,HDF5Exception);
    static bool isValidFile(const char * filename) throw(IOException,HDF5Exception);
    static void create(const string filename) throw(IOException,HDF5Exception);
    static void create(const char *  filename) throw(IOException,HDF5Exception);
    static void create(const string filename, unsigned int numberSpecies) throw(IOException,HDF5Exception);
    static void create(const char *  filename, unsigned int numberSpecies) throw(IOException,HDF5Exception);
    static void create(const char * filename, bool initializeModel, unsigned int numberSpecies=0) throw(IOException,HDF5Exception);

protected:
    static herr_t parseParameter(hid_t location_id, const char *attr_name, const H5A_info_t *ainfo, void *op_data);

public:
    SimulationFile(const string filename) throw(IOException,HDF5Exception,Exception);
    SimulationFile(const char* filename) throw(IOException,HDF5Exception,Exception);
	virtual ~SimulationFile() throw(IOException,HDF5Exception);
    virtual void close() throw(IOException,HDF5Exception);
    virtual void flush() throw(HDF5Exception);
    virtual string checkpoint() throw(IOException,HDF5Exception);

    // Methods for working with parameters.
    virtual map<string,string> getParameters();
    virtual string getParameter(string key, string defaultValue="");
    virtual void setParameter(string key, string value) throw(HDF5Exception);

    // Methods for working with the model.
    virtual void getReactionModel(lm::io::ReactionModel * reactionModel) throw(Exception,InvalidArgException,HDF5Exception);
    virtual void setReactionModel(lm::io::ReactionModel * reactionModel) throw(Exception,InvalidArgException,HDF5Exception);
    virtual void getDiffusionModel(lm::io::DiffusionModel * diffusionModel) throw(Exception,InvalidArgException,HDF5Exception);
    virtual void getDiffusionModelLattice(lm::io::DiffusionModel * diffusionModel, byte * lattice, size_t latticeMaxSize, byte * latticeSites, size_t latticeSitesMaxSize) throw(Exception,InvalidArgException,HDF5Exception);
    virtual void getDiffusionModelLattice(lm::io::DiffusionModel * diffusionModel, lm::rdme::Lattice * lattice) throw(Exception,InvalidArgException,HDF5Exception);
    virtual void setDiffusionModel(lm::io::DiffusionModel * diffusionModel) throw(Exception,InvalidArgException,HDF5Exception);
    virtual void setDiffusionModelLattice(lm::io::DiffusionModel * m, uint8_t * lattice, uint8_t * latticeSites) throw(Exception,InvalidArgException,HDF5Exception);
    virtual void setDiffusionModelLattice(lm::io::DiffusionModel * m, lm::rdme::Lattice * lattice) throw(Exception,InvalidArgException,HDF5Exception);
    virtual void setSpatialModel(lm::io::SpatialModel * model) throw(Exception,InvalidArgException,HDF5Exception);
    virtual void getSpatialModel(lm::io::SpatialModel * model) throw(Exception,InvalidArgException,HDF5Exception);


    // Methods for working with a replicate.
    virtual bool replicateExists(unsigned int replicate) throw(HDF5Exception);
    virtual void openReplicate(unsigned int replicate) throw(HDF5Exception);
    virtual void appendSpeciesCounts(unsigned int replicate, lm::io::SpeciesCounts * speciesCounts) throw(HDF5Exception);
    virtual void appendLattice(unsigned int replicate, lm::io::Lattice * lattice, byte * latticeData, size_t latticeDataSize) throw(InvalidArgException,HDF5Exception);
    virtual void appendParameterValues(unsigned int replicate, lm::io::ParameterValues * parameterValues) throw(HDF5Exception,InvalidArgException);
    virtual void setFirstPassageTimes(unsigned int replicate, lm::io::FirstPassageTimes * speciesCounts) throw(HDF5Exception,InvalidArgException);
    virtual vector<double> getLatticeTimes(unsigned int replicate) throw(HDF5Exception,InvalidArgException);
    virtual void getLattice(unsigned int replicate, unsigned int latticeIndex, lm::rdme::Lattice * lattice) throw(HDF5Exception,InvalidArgException);
    virtual void closeReplicate(unsigned int replicate) throw(HDF5Exception);
    virtual void closeAllReplicates() throw(HDF5Exception);

    //virtual void appendSpatialModelObjects(unsigned int replicate, lm::io::SpatialModel * model) throw(HDF5Exception,InvalidArgException);
    //virtual void getSpatialModelObjects(unsigned int replicate, lm::io::SpatialModel * model) throw(HDF5Exception);

	/*virtual lattice_coord_t getLatticeSize() const;
	virtual nmdist_t getLatticeSpacing() const;
	virtual uint getMaxParticlesPerSite() const;
	virtual lattice_particle_t getMaxParticleType() const;
	virtual lattice_site_t getMaxSiteType() const;
	virtual const std::map<uint64,uint64> getMaxParticleCounts() const;
	virtual const std::map<uint64,uint64> getMaxSiteCounts() const;
	
	virtual uint64 getNumberFrames() const;	
	virtual const std::vector<nstime_t> getFrameTimes() const;
	virtual void loadFrame(uint64 frameIndex, Lattice* lattice, nstime_t* time=NULL) const throw(HDF5Exception);
	
	virtual uint64 getNumberLatticeConfigurations() const;
	virtual const std::vector<nstime_t> getLatticeConfigurationTimes() const;
	virtual void loadLatticeConfiguration(uint64 latticeIndex, Lattice* lattice, nstime_t* time=NULL) const throw(HDF5Exception);*/
	
public:

    struct ReplicateHandles
    {
        hid_t group;
        hid_t speciesCountsDataset, speciesCountTimesDataset;
        ReplicateHandles():group(H5I_INVALID_HID),speciesCountsDataset(H5I_INVALID_HID),speciesCountTimesDataset(H5I_INVALID_HID) {}
    };

	
protected:
    virtual void open() throw(IOException,HDF5Exception,Exception);
    virtual void openGroups() throw(HDF5Exception);
    virtual void loadParameters() throw(HDF5Exception);
    virtual void loadModel() throw(Exception,HDF5Exception);
    virtual ReplicateHandles * openReplicateHandles(unsigned int replicate) throw(HDF5Exception);
    virtual ReplicateHandles * createReplicateHandles(string replicateString) throw(Exception,HDF5Exception);
    virtual void closeReplicateHandles(ReplicateHandles * handles) throw(HDF5Exception);
	
protected:
    string          filename;
    hid_t           file;
    unsigned int    version;

    // Main group handles.
    hid_t           parametersGroup, modelGroup, simulationsGroup;

    // The parameters.
    map<string,string> parameterMap;

    // The model.
    bool            modelLoaded;
    unsigned int    numberSpecies;

    // Handles for each replicate that is open.
    map<unsigned int,ReplicateHandles *> openReplicates;

};

}
}
}

#endif
