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
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/io/hdf5/HDF5.h"
#include "lm/Exceptions.h"
#include "lm/Types.h"
#include "robertslab/pbuf/NDArray.pb.h"

namespace lm {

namespace rdme{
class Lattice;
}

namespace io {

class BoundaryConditions;
class DiffusionModel;
class FirstPassageTimes;
class FFluxOutput;
class Lattice;
class LatticeTimeSeries;
class OrderParameterFirstPassageTimes;
class OrderParameters;
class ReactionModel;
class ParameterValues;
class SimulationParameters;
class SpeciesCounts;
class SpatialModel;
class TilingHist;
class Tilings;

namespace hdf5 {

using std::string;
using std::map;
using std::vector;
using lm::IOException;

//class IOException;

typedef struct {
    lm::io::OrderParameters * orderParameters;
} CallbackDataOrderParameters;

typedef struct {
    lm::io::Tilings * tilings;
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
//    static herr_t getFFluxParametersInterfaceCallback (hid_t loc_id, const char *name, const H5L_info_t *info, void *operator_data);
//    static herr_t getFFluxParametersOrderParameterCallback (hid_t loc_id, const char *name, const H5L_info_t *info, void *operator_data);
    static herr_t getOrderParametersCallback (hid_t loc_id, const char *name, const H5L_info_t *info, void *callbackDataOrderParameters);
    static herr_t getTilingsCallback (hid_t loc_id, const char *name, const H5L_info_t *info, void *callbackDataTilings);

public:
    Hdf5File(const string filename) throw(IOException,HDF5Exception,Exception);
    Hdf5File(const char* filename) throw(IOException,HDF5Exception,Exception);
	virtual ~Hdf5File();
    virtual void close() throw(IOException,HDF5Exception);
    virtual string checkpoint() throw(IOException,HDF5Exception);
    virtual void flush() throw(HDF5Exception);
    virtual hid_t initGroup(std::vector<std::string>& groupPathVector, hid_t rootGroup=-1);
    virtual hid_t initGroup(std::string& groupPath, hid_t rootGroup=-1);

    // Methods for working with parameters.
    virtual void getParameters(lm::io::SimulationParameters* parameters) const;
    virtual const map<string,string>& getParameters() const;
    virtual map<string,string>& getParameters();
    virtual string getParameter(string key, string defaultValue="");
    virtual void setParameter(string key, string value) throw(HDF5Exception);

    // Methods for working with the model.
    virtual bool hasDiffusionModel() const;
    virtual void getDiffusionModel(lm::io::DiffusionModel* diffusionModel) const;
    virtual void setDiffusionModel(lm::io::DiffusionModel* diffusionModel);
    virtual bool hasOrderParameters() const;
    virtual void getOrderParameters(lm::io::OrderParameters* orderParameters) const;
    virtual void setOrderParameters(lm::io::OrderParameters* orderParameters);
    virtual bool hasReactionModel() const;
    virtual void getReactionModel(lm::io::ReactionModel* reactionModel) const;
    virtual void setReactionModel(lm::io::ReactionModel* reactionModel);
    virtual void setSpatialModel(lm::io::SpatialModel* model);
    virtual void getSpatialModel(lm::io::SpatialModel* model) const;
    virtual bool hasTilings() const;
    virtual void getTilings(lm::io::Tilings* tilings) const;
    virtual void setTilings(lm::io::Tilings* tilings);
    virtual bool hasBoundaryGradient() const;
    virtual void getBoundaryGradient(lm::io::BoundaryConditions* bc) const;

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

    //virtual void appendSpatialModelObjects(uint64_t replicate, lm::io::SpatialModel * model) throw(HDF5Exception,InvalidArgException);
    //virtual void getSpatialModelObjects(uint64_t replicate, lm::io::SpatialModel * model) throw(HDF5Exception);

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
//    template <typename T> void setNDArray(std::string& groupPath, std::string& datasetName, robertslab::pbuf::NDArray* ndarray)
//    {
//        // declare the HDF5 boilerplate variable
//        hid_t group;
//
//        // Open the group the NDArray dataset is going to be stored in
//        if ((group = H5Gopen2(file, groupPath.c_str(), H5P_DEFAULT)) < 0)
//        {
//            HDF5_EXCEPTION_CALL(group, H5Gcreate2(file, groupPath.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
//        }
//
//        setNDArray<T>(group, datasetName, ndarray);
//    }

    template <typename T> void setNDArray(std::string& groupPath, std::string& datasetName, robertslab::pbuf::NDArray* ndarray, hid_t rootGroup=-1)
    {
        // initialize the group we'll be storing the NDArray dataset in
        hid_t group = initGroup(groupPath, rootGroup);

        // extract the data for the dataset from the NDArray
        lm::protowrap::NDArray<T> ndarrayWrap(ndarray);
        T* data = ndarrayWrap.get_data();

        // declare the HDF5 boilerplate variables
        uint RANK(ndarrayWrap.rank());
        hid_t dataspace, dataset, filespace, memspace, prop;
        hsize_t chunkdims[RANK], dims[RANK], dimsr[RANK], dimstotal[RANK], maxdims[RANK], offset[RANK];

//    // If the NDArray's dataset already exists, delete it
//    if (H5Lexists(group, groupName.c_str(), H5P_DEFAULT))
//    {
//        HDF5_EXCEPTION_CHECK(H5Ldelete(group, groupName.c_str(), H5P_DEFAULT));
//    }

        // write or extend the NDArray dataset
        dims[0] = RANK > 0 ? ndarrayWrap.shape(0) : 0;
        chunkdims[0] = 1000;
        maxdims[0] = H5S_UNLIMITED;
        for (int i=1; i<RANK; i++)
        {
            dims[i] = ndarrayWrap.shape(i);
            chunkdims[i] = dims[i];
            maxdims[i] = dims[i];
        }

        // if the dataset exists, extend it
        if ((dataset = H5Dopen2(group, datasetName.c_str(), H5P_DEFAULT))>=0)
        {
            HDF5_EXCEPTION_CALL(prop, H5Dget_create_plist(dataset));

            HDF5_EXCEPTION_CALL(filespace, H5Dget_space(dataset));
            HDF5_EXCEPTION_CHECK(H5Sget_simple_extent_dims(filespace, dimsr, NULL));
            /* Extend the dataset */
            dimstotal[0] = dimsr[0] + dims[0];
            if (RANK==2) {dimstotal[1] = dimsr[1];}
            HDF5_EXCEPTION_CHECK(H5Dset_extent(dataset, dimstotal));
            // reopen the now-extended dataset's filespace
            HDF5_EXCEPTION_CALL(filespace, H5Dget_space(dataset));
            /* Select a hyperslab in extended portion of dataset  */
            offset[0] = dimsr[0];
            if (RANK==2) {offset[1] = 0;}
            HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offset, NULL, dims, NULL));
            /* Define memory space */
            HDF5_EXCEPTION_CALL(memspace, H5Screate_simple(RANK, dims, NULL));
            HDF5_EXCEPTION_CHECK(H5Dwrite(dataset, HDF5Type<T>::T(), memspace, filespace, H5P_DEFAULT, data));

            HDF5_EXCEPTION_CHECK(H5Dclose(dataset));
            HDF5_EXCEPTION_CHECK(H5Sclose(memspace));
            HDF5_EXCEPTION_CHECK(H5Sclose(filespace));
        }
            // otherwise, create the dataset
        else
        {
            /* Create the dataField space with unlimited dimensions. */
            HDF5_EXCEPTION_CALL(dataspace, H5Screate_simple(RANK, dims, maxdims));
            /* Modify dataset creation properties, i.e. enable chunking  */
            HDF5_EXCEPTION_CALL(prop, H5Pcreate(H5P_DATASET_CREATE));
            HDF5_EXCEPTION_CHECK(H5Pset_chunk(prop, RANK, chunkdims));
            /* Create a new dataset within the file using chunk creation properties.  */
            dataset = H5Dcreate2(group, datasetName.c_str(), HDF5Type<T>::T(), dataspace, H5P_DEFAULT, prop, H5P_DEFAULT);
            /* Write dataField to dataset */
            HDF5_EXCEPTION_CHECK(H5Dwrite(dataset, HDF5Type<T>::T(), H5S_ALL, H5S_ALL, H5P_DEFAULT, data));

            HDF5_EXCEPTION_CHECK(H5Dclose(dataset));
            HDF5_EXCEPTION_CHECK(H5Pclose(prop));
            HDF5_EXCEPTION_CHECK(H5Sclose(dataspace));
        }

        // clean up, if required
        if (ndarrayWrap.compressed_deflate()) delete[] data;
    }
    template <typename T> void setNDArrayReplicate(uint64_t replicate, std::string& groupRelativePath, std::string datasetName, robertslab::pbuf::NDArray* ndarray)
    {
        ReplicateHandles * replicateHandles = openReplicateHandles(replicate);

        // Open the group relative to the replicate group
//        hid_t group = initGroup(groupRelativePath, replicateHandles->group);
//        if ((group=H5Gopen2(replicateHandles->group, groupRelativePath.c_str(), H5P_DEFAULT)) < 0)
//        {
//            HDF5_EXCEPTION_CALL(group,H5Gcreate2(replicateHandles->group, groupRelativePath.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
//        }

        setNDArray<T>(groupRelativePath, datasetName, ndarray, replicateHandles->group);
    }

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
    virtual ReplicateHandles * openReplicateHandles(uint64_t replicate) throw(HDF5Exception);
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
    map<uint64_t,ReplicateHandles *> openReplicates;

};

}
}
}

#endif
