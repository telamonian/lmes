/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
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
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, the Roberts Group, Johns Hopkins University, nor the names
 * of its contributors may be used to endorse or promote products derived from
 * this Software without specific prior written permission.
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
#ifndef LM_IO_HDF5_SIMULATIONFILE_H_
#define LM_IO_HDF5_SIMULATIONFILE_H_

#include <google/protobuf/repeated_field.h>
#include <map>
#include <string>
#include <vector>

#include "lm/protowrap/NDArray.h"
#include "lm/io/hdf5/HDF5.h"
#include "lm/Exceptions.h"
#include "lm/Types.h"
#include "robertslab/pbuf/NDArray.pb.h"

namespace lm {

namespace rdme{
class Lattice;
}

namespace input {
class BoundaryConditions;
class DiffusionModel;
class OrderParameters;
class ReactionModel;
class SimulationParameters;
class SpatialModel;
class Tilings;
}

namespace io {
class FirstPassageTimes;
class FFluxOutput;
class Lattice;
class LatticeTimeSeries;
class OrderParameterFirstPassageTimes;
class ParameterValues;
class SpeciesCounts;
class SpeciesTimeSeries;
class TilingHist;

namespace hdf5 {

using std::string;
using std::map;
using std::vector;
using lm::IOException;

//class IOException;

typedef struct {
    lm::input::OrderParameters * orderParameters;
} CallbackDataOrderParameters;

typedef struct {
    lm::input::Tilings * tilings;
    string filename;
} CallbackDataTilings;

class SimulationFile
{
public:
    SimulationFile() {}
    virtual ~SimulationFile() {}
};

class Hdf5File : public SimulationFile
{
public:
    // struct that handles the set of arguments required to write out a dataset
    struct DatasetDescriptor
    {
        DatasetDescriptor(const std::string& groupPath, const std::string& datasetName, const utuple& shape, hid_t hdf5Type, void* data, hid_t rootGroup=-1);
        DatasetDescriptor(const std::string& groupPath, const std::string& datasetName, const robertslab::pbuf::NDArray& ndarrayMsg, hid_t rootGroup=-1);
        ~DatasetDescriptor();

        hid_t rootGroup;
        const std::string& groupPath;
        const std::string& datasetName;

        utuple shape;
        uint startingColumn;

        hid_t hdf5Type;
        void* data;
        bool compressed_deflate;
        bool isNDArray;
    };

    struct ReplicateHandles
    {
        hid_t group;
        hid_t speciesCountsDataset, speciesCountTimesDataset;
        ReplicateHandles():group(H5I_INVALID_HID),speciesCountsDataset(H5I_INVALID_HID),speciesCountTimesDataset(H5I_INVALID_HID) {}
    };
    typedef PairMap<string, uint64_t, ReplicateHandles *> ReplicateHandleMap;

    static const uint MIN_VERSION;
    static const uint CURRENT_VERSION;
    static const uint MAX_REACTION_RATE_CONSTANTS;
    static const uint MAX_SHAPE_PARAMETERS;

    static bool isValidFile(const string filename) throw(IOException,HDF5Exception);
    static bool isValidFile(const char * filename) throw(IOException,HDF5Exception);
    static void create(const string filename) throw(IOException,HDF5Exception);
    static void create(const char *  filename) throw(IOException,HDF5Exception);
    static void create(const string filename, unsigned int numberSpecies) throw(IOException,HDF5Exception);
    static void create(const char *  filename, unsigned int numberSpecies) throw(IOException,HDF5Exception);
    static void create(const char * filename, bool initializeModel, unsigned int numberSpecies=0) throw(IOException,HDF5Exception);

    Hdf5File(const string filename) throw(IOException,HDF5Exception,Exception);
    Hdf5File(const char* filename) throw(IOException,HDF5Exception,Exception);
	virtual ~Hdf5File();
    virtual void close() throw(IOException,HDF5Exception);
    virtual string checkpoint() throw(IOException,HDF5Exception);
    virtual void flush() throw(HDF5Exception);
    virtual hid_t initGroup(const std::vector<std::string>& groupPathVector, hid_t rootGroup=-1);
    virtual hid_t initGroup(const std::string& groupPath, hid_t rootGroup=-1);

    // Methods for working with parameters.
    virtual void getParameters(lm::input::SimulationParameters* parameters) const;
    virtual const map<string,string>& getParameters() const;
    virtual map<string,string>& getParameters();
    virtual string getParameter(string key, string defaultValue="");
    virtual void setParameter(string key, string value) throw(HDF5Exception);

    // Methods for working with the model.
    virtual bool hasDiffusionModel() const;
    virtual void getDiffusionModel(lm::input::DiffusionModel* diffusionModel) const;
    virtual void setDiffusionModel(lm::input::DiffusionModel* diffusionModel);
    virtual bool hasOrderParameters() const;
    virtual void getOrderParameters(lm::input::OrderParameters* orderParameters) const;
    virtual void setOrderParameters(lm::input::OrderParameters* orderParameters);
    virtual bool hasReactionModel() const;
    virtual void getReactionModel(lm::input::ReactionModel* reactionModel) const;
    virtual void setReactionModel(lm::input::ReactionModel* reactionModel);
    virtual void setSpatialModel(lm::input::SpatialModel* model);
    virtual void getSpatialModel(lm::input::SpatialModel* model) const;
    virtual bool hasTilings() const;
    virtual void getTilings(lm::input::Tilings* tilings) const;
    virtual void setTilings(lm::input::Tilings* tilings);
    virtual bool hasBoundaryGradient() const;
    virtual void getBoundaryGradient(lm::input::BoundaryConditions* bc) const;

    // Methods for working with a replicate.
    virtual bool replicateExists(uint64_t replicate);
    virtual void openReplicate(uint64_t replicate) throw(HDF5Exception);
    virtual void appendSpeciesCounts(uint64_t replicate, lm::io::SpeciesCounts* speciesCounts) throw(HDF5Exception);
    static int32_t* dumpSpeciesCounts(const lm::io::SpeciesTimeSeries& speciesTimeSeries);
    static double* dumpSpeciesTimes(const lm::io::SpeciesTimeSeries& speciesTimeSeries);
    virtual void appendSpeciesTimeSeries(uint64_t replicate, const lm::io::SpeciesTimeSeries& speciesCounts);
    virtual void appendSpeciesTimeSeries(uint64_t replicate, int numberEntries, int numberSpecies, const int32_t* counts, const double* times);
    virtual void appendLatticeTimeSeries(uint64_t replicate, const lm::io::LatticeTimeSeries& data);
    virtual void appendParameterValues(uint64_t replicate, lm::io::ParameterValues* parameterValues) throw(HDF5Exception,InvalidArgException);
    virtual void setFirstPassageTimes(uint64_t replicate, lm::io::FirstPassageTimes* speciesCounts) throw(HDF5Exception,InvalidArgException);
    virtual vector<double> getLatticeTimes(uint64_t replicate) throw(HDF5Exception,InvalidArgException);
    virtual void getLattice(uint64_t replicate, unsigned int latticeIndex, lm::rdme::Lattice* lattice) throw(HDF5Exception,InvalidArgException);
    virtual void closeReplicate(uint64_t replicate) throw(HDF5Exception);
    virtual void closeAllReplicates() throw(HDF5Exception);

    // Methods for working with output from forward flux simulations
    virtual void setFFluxOutput(lm::io::FFluxOutput* ffluxOutput);
    virtual void setFFluxBasinOutput(lm::io::FFluxOutput* ffluxOutput, int basinIndex, hid_t basinGroup);
    virtual void setFFluxFinalOutput(lm::io::FFluxOutput* ffluxOutput, hid_t ffluxOutputGroup);
    virtual void setFFluxTrajectoryOutput_Count(lm::io::FFluxOutput* ffluxOutput, int outIndex, hid_t lifecycleGroup);
    virtual void setFFluxTrajectoryOutput_EdgeID(lm::io::FFluxOutput* ffluxOutput, int outIndex, hid_t lifecycleGroup);
    virtual void setFFluxTrajectoryOutput_SpeciesCount(lm::io::FFluxOutput* ffluxOutput, int outIndex, hid_t lifecycleGroup);
    virtual void setFFluxTrajectoryOutput_Time(lm::io::FFluxOutput* ffluxOutput, int outIndex, hid_t lifecycleGroup);
    virtual void setFFluxTrajectoryOutput_TrajectoryID(lm::io::FFluxOutput* ffluxOutput, int outIndex, hid_t lifecycleGroup);
    template <typename T> void _setFFluxTrajectoryOutput(::google::protobuf::RepeatedField<T> data, hsize_t* dims, string dsetName, hid_t dsetType, hid_t lifecycleGroup, uint RANK);
    virtual void setTilingHist(lm::io::TilingHist* tilingHist, std::string datasetName, hid_t superGroup);

    virtual void setRecordNamePrefix(const string& newRecordNamePrefix);

    //virtual void appendSpatialModelObjects(uint64_t replicate, lm::input::SpatialModel * model) throw(HDF5Exception,InvalidArgException);
    //virtual void getSpatialModelObjects(uint64_t replicate, lm::input::SpatialModel * model) throw(HDF5Exception);

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

	// Methods for working with NDArrays
    hsize_t setDatasetFromNDArray(const std::string& groupPath, const std::string& datasetName, const robertslab::pbuf::NDArray& ndarrayRef, hid_t rootGroup = -1);
    void setDatasetFromNDArrayReplicate(uint64_t replicate, const std::string& groupRelativePath, const std::string& datasetName, const robertslab::pbuf::NDArray& ndarray, bool condensed=false);
    void setDatasetFromNDArrayReplicateCondensed(uint64_t replicate, const std::string& groupRelativePath, const std::string& datasetName, const robertslab::pbuf::NDArray& ndarray);
    template <typename Container> hsize_t setDatasetFromContainer(const std::string& groupPath, const std::string& datasetName, const Container& container, hid_t rootGroup = -1)
    {
        utuple shape(container.size());
        hid_t hdf5Type = HDF5Type<typename Container::value_type>::T();

        return setDataset(DatasetDescriptor(groupPath, datasetName, shape, hdf5Type, (void*)container.data(), rootGroup));
    }

    // low(ish)-level methods for outputing abstract multi-dimensional array (ie a pointer plus a shape) as a dataset
    hsize_t setDataset(const DatasetDescriptor& dd);
//    void setDatasets(std::vector<DatasetDescriptor>* ddVector);

protected:
    static herr_t parseParameter(hid_t location_id, const char *attr_name, const H5A_info_t *ainfo, void *op_data);
    static herr_t getOrderParametersCallback (hid_t loc_id, const char *name, const H5L_info_t *info, void *callbackDataOrderParameters);
    static herr_t getTilingsCallback (hid_t loc_id, const char *name, const H5L_info_t *info, void *callbackDataTilings);

    virtual void open() throw(IOException,HDF5Exception,Exception);
    virtual void openGroups() throw(HDF5Exception);
    virtual void loadParameters() throw(HDF5Exception);
    virtual void loadModel() throw(Exception,HDF5Exception);
    virtual Hdf5File::ReplicateHandles* openReplicateHandles(uint64_t replicate) throw(HDF5Exception);
    virtual Hdf5File::ReplicateHandles* createReplicateHandles(string replicateString) throw(Exception,HDF5Exception);
    virtual void closeReplicateHandles(ReplicateHandles * handles) throw(HDF5Exception);
	
protected:
    string          filename;
    hid_t           file;
    unsigned int    version;

    // Main group handles.
    hid_t           parametersGroup, modelGroup, simulationsGroup;

    // for HDF5 output files, recordNamePrefix is used to
    string recordNamePrefix;

    // The parameters.
    map<string,string> parameterMap;

    // The model.
    bool            modelLoaded;
    unsigned int    numberSpecies;

    // Handles for each replicate that is open.
    ReplicateHandleMap::T openReplicates;
//    map<uint64_t,ReplicateHandles *> openReplicates;

};

}
}
}

#endif
