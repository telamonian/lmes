/*
 * University of Illinois Open Source License
 * Copyright 2008-2011 Luthey-Schulten Group,
 * Copyright 2012-2014 Roberts Group,
 * All rights reserved.
 *
 * Developed by: Luthey-Schulten Group
 *                  University of Illinois at Urbana-Champaign
 *                  http://www.scs.uiuc.edu/~schulten
 *
 * Developed by: Roberts Group
 *                  Johns Hopkins University
 *                  http://biophysics.jhu.edu/roberts/
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
#include <cstdio>
#include <cstring>
#include <google/protobuf/repeated_field.h>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <zlib.h>
#include <lm/array/NDArray.h>

#include "lm/EnumHelper.h"
#include "lm/Exceptions.h"
#include "lm/Math.h"
#include "lm/Print.h"
#include "lm/Tune.h"
#include "lm/Types.h"
#include "lm/input/DiffusionModel.pb.h"
#include "lm/input/ReactionModel.pb.h"
#include "lm/input/SimulationParameters.pb.h"
#include "lm/input/SpatialModel.pb.h"
#include "lm/input/Tilings.pb.h"
#include "lm/io/FirstPassageTimes.pb.h"
#include "lm/io/FFluxOutput.pb.h"
#include "lm/io/LatticeTimeSeries.pb.h"
#include "lm/input/OrderParameters.pb.h"
#include "lm/io/ParameterValues.pb.h"
#include "lm/io/SpeciesCounts.pb.h"
#include "lm/io/SpeciesTimeSeries.pb.h"
#include "lm/io/TilingHist.pb.h"
#include "lm/io/hdf5/HDF5.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/rdme/Lattice.h"
#include "lm/types/ArrayOrdering.pb.h"
#include "lm/types/Lattice.pb.h"

using std::list;
using std::map;
using std::stringstream;
using std::string;
using std::vector;
using lm::IOException;

namespace lm {
namespace io {
namespace hdf5 {

Hdf5File::DatasetDescriptor::DatasetDescriptor(const std::string& groupPath, const std::string& datasetName, const utuple& shape, hid_t hdf5Type, void* data, hid_t rootGroup)
:rootGroup(rootGroup),groupPath(groupPath),datasetName(datasetName),shape(shape),startingColumn(0),hdf5Type(hdf5Type),data(static_cast<byte*>(data)),compressed_deflate(false),isNDArray(false)
{
}

Hdf5File::DatasetDescriptor::DatasetDescriptor(const std::string& groupPath, const std::string& datasetName, const robertslab::pbuf::NDArray& ndarrayMsg, hid_t rootGroup)
:rootGroup(rootGroup),groupPath(groupPath),datasetName(datasetName),shape(ndarrayMsg.shape()),startingColumn(0),hdf5Type(-1),data(NULL),compressed_deflate(false),isNDArray(true)
{
    lm::protowrap::NDArray<void> ndarrayWrap(ndarrayMsg);

    hdf5Type = ndarrayWrap.hdf5_type();
    data = static_cast<byte*>(ndarrayWrap.get_data());
    compressed_deflate = ndarrayWrap.compressed_deflate();
}

Hdf5File::DatasetDescriptor::~DatasetDescriptor() {if (isNDArray and compressed_deflate) {if (data!=NULL) {delete[] data; data=NULL;}}}

const uint Hdf5File::MIN_VERSION                   = 2;
const uint Hdf5File::CURRENT_VERSION               = 4;
const uint Hdf5File::MAX_REACTION_RATE_CONSTANTS   = 10;
const uint Hdf5File::MAX_SHAPE_PARAMETERS          = 10;

Hdf5File::Hdf5File(const string filename) throw(IOException,HDF5Exception,Exception)
:filename(filename),file(H5I_INVALID_HID),version(0),parametersGroup(H5I_INVALID_HID),modelGroup(H5I_INVALID_HID),
 simulationsGroup(H5I_INVALID_HID),recordNamePrefix(""),modelLoaded(false),numberSpecies(0)
{
    open();
}

Hdf5File::Hdf5File(const char* filename) throw(IOException,HDF5Exception,Exception)
:filename(filename),file(H5I_INVALID_HID),version(0),parametersGroup(H5I_INVALID_HID),modelGroup(H5I_INVALID_HID),
 simulationsGroup(H5I_INVALID_HID),recordNamePrefix(""),modelLoaded(false),numberSpecies(0)
{
    open();
}

Hdf5File::~Hdf5File()
{
    //Close the file, if it is still open.
    close();
}

void Hdf5File::open() throw(IOException,HDF5Exception,Exception)
{
    // Make sure gzip is supported.
    unsigned int filter_info;
    if (!H5Zfilter_avail(H5Z_FILTER_DEFLATE)) throw lm::Exception("The HDF5 library does not support gzip compression.");
    HDF5_EXCEPTION_CHECK(H5Zget_filter_info (H5Z_FILTER_DEFLATE, &filter_info));
    if (!(filter_info & H5Z_FILTER_CONFIG_ENCODE_ENABLED) || !(filter_info & H5Z_FILTER_CONFIG_DECODE_ENABLED)) throw lm::Exception("The HDF5 library does not support gzip filtering for both encoding and decoding.");

    // Turn of error printing.
    HDF5_EXCEPTION_CHECK(H5Eset_auto2(H5E_DEFAULT, NULL, NULL));

    // Make sure the file exists.
    struct stat fileStats;
    if (stat(filename.c_str(), &fileStats) != 0) throw lm::IOException("the specified file did not exist", filename.c_str());

    // Make sure it is a regular file.
    if (!S_ISREG(fileStats.st_mode)) throw lm::IOException("the specified file was not a regular file", filename.c_str());

    // Make sure the file has the correct magic and hdf headers.
    if (!isValidFile(filename)) throw lm::IOException("the specified file was not of the correct format", filename.c_str());

    // Open the file.
    HDF5_EXCEPTION_CALL(file,H5Fopen(filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT));

    // Get the size of the user block.
    hid_t creationProperties;
    hsize_t userblockSize;
    HDF5_EXCEPTION_CALL(creationProperties,H5Fget_create_plist(file));
    HDF5_EXCEPTION_CHECK(H5Pget_userblock(creationProperties, &userblockSize));
    if (userblockSize < 4) throw lm::IOException("the specified file did not have the correct user block size", userblockSize);

    // Make sure the version is supported.
    HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/", "formatVersion", &version));
    if (version < MIN_VERSION || version > CURRENT_VERSION) throw lm::IOException("the specified file format version is not supported", version);

    // Open the groups.
    openGroups();

    // Loaded the parameters.
    loadParameters();
}

hid_t Hdf5File::initGroup(const vector<string>& groupPathVector, hid_t rootGroup)
{
    hid_t currentGroup, nextGroup;
    currentGroup = rootGroup>=0 ? rootGroup : file;
    for (vector<string>::const_iterator it = groupPathVector.begin(); it!=groupPathVector.end(); it++)
    {
        if ((nextGroup = H5Gopen2(currentGroup, it->c_str(), H5P_DEFAULT)) < 0)
        {
            HDF5_EXCEPTION_CALL(nextGroup, H5Gcreate2(currentGroup, it->c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
        }
        currentGroup = nextGroup;
    }
    return currentGroup;
}

hid_t Hdf5File::initGroup(const string& groupPath, hid_t rootGroup)
{
    vector<string> groupPathVector;
    std::stringstream ss(groupPath);
    std::string item;
    while (std::getline(ss, item, '/'))
    {
        if (item.size() > 0)
        {
            groupPathVector.push_back(item);
        }
    }

    return initGroup(groupPathVector, rootGroup);
}

void Hdf5File::openGroups() throw(HDF5Exception)
{
    HDF5_EXCEPTION_CALL(parametersGroup,H5Gopen2(file, "/Parameters", H5P_DEFAULT));
    HDF5_EXCEPTION_CALL(modelGroup,H5Gopen2(file, "/Model", H5P_DEFAULT));
    HDF5_EXCEPTION_CALL(simulationsGroup,H5Gopen2(file, "/Simulations", H5P_DEFAULT));
}

void Hdf5File::flush() throw(HDF5Exception)
{
    if (file != H5I_INVALID_HID)
    {
        lm::Print::printf(Print::DEBUG, "Flushing file %s.", filename.c_str());
        HDF5_EXCEPTION_CHECK(H5Fflush(file, H5F_SCOPE_GLOBAL));
    }
}

string Hdf5File::checkpoint() throw(IOException,HDF5Exception)
{
    // Close the file.
    close();

    // Copy the file to a checkpoint file.
    string checkpointFilename(filename+".chk");
    FILE * out=NULL, * in=NULL;
    in=fopen(filename.c_str(), "rb");
    if (in == NULL) throw lm::IOException("the checkpoint input file could not be opened", filename.c_str());
    out=fopen(checkpointFilename.c_str(), "wb");
    if (out == NULL) throw lm::IOException("the checkpoint output file could not be opened", checkpointFilename.c_str());
    size_t bufferSize = 1024*1024;
    char * buffer = new char[bufferSize];
    while (!feof(in))
    {
        size_t bytesRead = fread(buffer, sizeof(char), bufferSize, in);
        if (bytesRead != bufferSize && ferror(in)) throw lm::IOException("the checkpoint input file could not be read", filename.c_str());
        size_t bytesWritten = fwrite(buffer, sizeof(char), bytesRead, out);
        if (bytesWritten != bytesRead) throw lm::IOException("the checkpoint input file could not be read", checkpointFilename.c_str());
    }
    if (fclose(in) != 0)  throw lm::IOException("the checkpoint input file could not be closed", filename.c_str());
    if (fclose(out) != 0)  throw lm::IOException("the checkpoint output file could not be closed", checkpointFilename.c_str());
    delete[] buffer;

    // Open the file again.
    open();

    return checkpointFilename;
}

void Hdf5File::close() throw(IOException,HDF5Exception)
{
    // Close any open replicate groups.
    closeAllReplicates();

    // Close any open groups.
    if (parametersGroup != H5I_INVALID_HID)
    {
        HDF5_EXCEPTION_CHECK(H5Gclose(parametersGroup));
        parametersGroup = H5I_INVALID_HID;
    }
    if (modelGroup != H5I_INVALID_HID)
    {
        HDF5_EXCEPTION_CHECK(H5Gclose(modelGroup));
        modelGroup = H5I_INVALID_HID;
    }
    if (simulationsGroup != H5I_INVALID_HID)
    {
        HDF5_EXCEPTION_CHECK(H5Gclose(simulationsGroup));
        simulationsGroup = H5I_INVALID_HID;
    }

    // Close the file.
    if (file != H5I_INVALID_HID)
    {
        lm::Print::printf(Print::DEBUG, "Closing file %s, %d open objects remaining.", filename.c_str(), H5Fget_obj_count(file, H5F_OBJ_ALL)-1);
        HDF5_EXCEPTION_CHECK(H5Fclose(file));
        file = H5I_INVALID_HID;
    }
}

bool Hdf5File::isValidFile(const char * filename) throw(IOException,HDF5Exception)
{
    return isValidFile(string(filename));
}

bool Hdf5File::isValidFile(const string filename) throw(IOException,HDF5Exception)
{
    // Make sure the file has the right magic.
    FILE * fp = NULL;
    fp=fopen(filename.c_str(), "r");
    if (fp == NULL) throw lm::IOException("the specified file could not be opened", filename.c_str());
    char magic[5];
    magic[4] = '\0';
    for (int i=0; i<4; i++)
    {
        magic[i] = fgetc(fp);
        if (magic[i] == EOF)
        {
            magic[i] = '\0';
            break;
        }
    }
    if (fclose(fp) != 0)  throw lm::IOException("the specified file could not be closed", filename.c_str());
    if (strncmp(magic, "LMH5", 4) != 0) return false;

    // Make sure it is an hdf5 file.
    int isHdf5 = false;
    HDF5_EXCEPTION_CALL(isHdf5,H5Fis_hdf5(filename.c_str()));
    if (!isHdf5) return false;

    return true;

}

void Hdf5File::loadParameters() throw(HDF5Exception)
{
    hsize_t n=0;
    HDF5_EXCEPTION_CHECK(H5Aiterate2(parametersGroup, H5_INDEX_CRT_ORDER, H5_ITER_NATIVE, &n, &Hdf5File::parseParameter, this));
}

herr_t Hdf5File::parseParameter(hid_t location_id, const char *attr_name, const H5A_info_t *ainfo, void *op_data)
{
    Hdf5File * file = reinterpret_cast<Hdf5File *>(op_data);

    // Open the attribute.
    hid_t attr, type;
    H5T_class_t typeClass;
    hsize_t size;
    HDF5_EXCEPTION_CALL(attr,H5Aopen(location_id, attr_name, H5P_DEFAULT));
    HDF5_EXCEPTION_CALL(type,H5Aget_type(attr));
    typeClass=H5Tget_class(type);
    HDF5_EXCEPTION_CALL(size,H5Aget_storage_size(attr));
    if (file->version == 2 && typeClass==H5T_FLOAT && size == sizeof(double))
    {
        double value;
        char buffer[33];
        HDF5_EXCEPTION_CHECK(H5Aread(attr, H5T_NATIVE_DOUBLE, &value));
        snprintf(buffer, sizeof(buffer), "%g", value);
        file->parameterMap[attr_name] = buffer;
    }
    else if (file->version >= 3 && typeClass==H5T_STRING)
    {
        // Get the dataspace.
        hid_t space;
        HDF5_EXCEPTION_CALL(space,H5Aget_space(attr));

        // hdf5 requires different handling for fixed and variable length string attributes
        htri_t is_variable_len = H5Tis_variable_str(type);

        hid_t memtype;
        char * value;
        if (is_variable_len)
        {
            // Create the memory datatype.
//            HDF5_EXCEPTION_CALL(memtype, H5Tcopy(H5T_C_S1));
            HDF5_EXCEPTION_CALL(memtype, H5Tget_native_type(type, H5T_DIR_DEFAULT));

            // set the size to variable
//            HDF5_EXCEPTION_CHECK(H5Tset_size(memtype, H5T_VARIABLE));

            // Read the data. For variable length strings, pass the output char array as a char**. Apparently H5Aread will also take care of allocation
            HDF5_EXCEPTION_CHECK(H5Aread(attr, memtype, &value));

            // Add the parameter to the map.
            file->parameterMap[attr_name] = value;

            // Reclaim the memory.
            #ifdef OLD_HDFREE
                free(value);
            #else
                H5free_memory(value);
            #endif
        }
        else
        {
            // Create the memory datatype.
            HDF5_EXCEPTION_CALL(memtype, H5Tcopy(H5T_C_S1));

            // set the fixed size
            HDF5_EXCEPTION_CHECK(H5Tset_size(memtype, size));

            // allocate space for the fixed string
            value = new char[size];

            // Read the data.
            HDF5_EXCEPTION_CHECK(H5Aread(attr, memtype, value));

            // Add the parameter to the map.
            file->parameterMap[attr_name] = value;

            // Reclaim the memory.
            delete [] value;
        }

        HDF5_EXCEPTION_CHECK(H5Sclose(space));
        HDF5_EXCEPTION_CHECK(H5Tclose(memtype));

    }
    else
    {
        // could not parse the parameter. Warn the user
        if (file->version < 3)
        {
            THROW_EXCEPTION(lm::InputException, "Unable to parse user defined value for parameter: %s\n", attr_name);
        }
        else
        {
            THROW_EXCEPTION(lm::InputException, "Unable to parse user defined value for parameter: %s\n"
                "Try converting all parameter values to strings before setting them.\n", attr_name);
        }
    }
    HDF5_EXCEPTION_CHECK(H5Tclose(type));
    HDF5_EXCEPTION_CHECK(H5Aclose(attr));
    return 0;
}

void Hdf5File::getParameters(lm::input::SimulationParameters* parameters) const
{
    parameters->Clear();
    for (map<string,string>::const_iterator it=parameterMap.begin(); it != parameterMap.end(); it++)
    {
        parameters->add_key(it->first);
        parameters->add_value(it->second);
    }
}

const map<string,string>& Hdf5File::getParameters() const
{
    return parameterMap;
}

map<string,string>& Hdf5File::getParameters()
{
    return parameterMap;
}

string Hdf5File::getParameter(string key, string defaultValue)
{
    // If we didn't find the key, return the default value.
    map<string,string>::iterator it = parameterMap.find(key);
    if (it == parameterMap.end()) return defaultValue;

    return it->second;
}

void Hdf5File::setParameter(string key, string value) throw(HDF5Exception)
{
    // Set the parameter in the map.
    parameterMap[key] = value;

    // Save the parameter in the file.
    if (version <= 2)
    {
        double dvalue = atof(value.c_str());
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_double(file, "/Parameters", key.c_str(), &dvalue, 1));
    }
    else
    {
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_string(file, "/Parameters", key.c_str(), value.c_str()));
    }
}

void Hdf5File::loadModel() throw(Exception,HDF5Exception)
{
    if (!modelLoaded)
    {
        if (version <= 3)
        {
            HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Model", "numberSpecies", &numberSpecies));
            modelLoaded = true;
        }
        else
        {
            if (H5Lexists(file, "/Model/Reaction", H5P_DEFAULT) > 0)
            {
                HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Model/Reaction", "numberSpecies", &numberSpecies));
                modelLoaded = true;
            }
            else
            {
                throw Exception("No model has been defined.");
            }
        }
    }
}

bool Hdf5File::hasDiffusionModel() const
{
    return (H5Lexists(file, "/Model/Diffusion", H5P_DEFAULT) != 0);
}

void Hdf5File::getDiffusionModel(lm::input::DiffusionModel* diffusionModel) const
{
    // Make sure the model is not null and then clear it.
    if (diffusionModel == NULL) throw InvalidArgException("diffusionModel", "cannot be null");
    diffusionModel->Clear();

    if (H5Lexists(file, "/Model/Diffusion", H5P_DEFAULT))
    {
        // Read the diffusion model attributes.
        uint numberSpecies, numberReactions, numberSiteTypes, latticeXSize, latticeYSize, latticeZSize, particlesPerSite;
        double latticeSpacing;
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Model/Diffusion", "numberSpecies", &numberSpecies));
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Model/Diffusion", "numberReactions", &numberReactions));
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Model/Diffusion", "numberSiteTypes", &numberSiteTypes));
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_double(file, "/Model/Diffusion", "latticeSpacing", &latticeSpacing));
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Model/Diffusion", "latticeXSize", &latticeXSize));
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Model/Diffusion", "latticeYSize", &latticeYSize));
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Model/Diffusion", "latticeZSize", &latticeZSize));
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Model/Diffusion", "particlesPerSite", &particlesPerSite));

        // Fill in the model.
        diffusionModel->set_number_species(numberSpecies);
        diffusionModel->set_number_reactions(numberReactions);
        diffusionModel->set_number_site_types(numberSiteTypes);
        diffusionModel->set_lattice_spacing(latticeSpacing);
        lm::types::Lattice* lattice=diffusionModel->mutable_initial_lattice();
        lattice->set_lattice_x_size(latticeXSize);
        lattice->set_lattice_y_size(latticeYSize);
        lattice->set_lattice_z_size(latticeZSize);
        lattice->set_particles_per_site(particlesPerSite);

        int ndims;
        hsize_t dims[4];
        H5T_class_t type;
        size_t size;

        // Read the diffusion matrix.
        H5LTget_dataset_info(file, "/Model/Diffusion/DiffusionMatrix", dims, &type, &size);
        if (dims[0] != numberSiteTypes || dims[1] != numberSiteTypes || dims[2] != numberSpecies || size != sizeof(double)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Diffusion/DiffusionMatrix");
        double * D = new double[numberSiteTypes*numberSiteTypes*numberSpecies];
        H5LTread_dataset_double(file, "/Model/Diffusion/DiffusionMatrix", D);
        for (uint i=0; i<numberSiteTypes*numberSiteTypes*numberSpecies; i++) diffusionModel->add_diffusion_matrix(D[i]);
        delete [] D;

        // If we have reactions, read the reaction location matrix.
        if (numberReactions > 0)
        {
            H5LTget_dataset_info(file, "/Model/Diffusion/ReactionLocationMatrix", dims, &type, &size);
            if (dims[0] != numberReactions || dims[1] != numberSiteTypes || size != sizeof(uint)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Diffusion/ReactionLocationMatrix");
            uint * RL = new uint[numberReactions*numberSiteTypes];
            H5LTread_dataset(file, "/Model/Diffusion/ReactionLocationMatrix", H5T_STD_U32LE, RL);
            for (uint i=0; i<numberReactions*numberSiteTypes; i++) diffusionModel->add_reaction_location_matrix(RL[i]);
            delete [] RL;
        }

        // Read the initial lattice.
        HDF5_EXCEPTION_CHECK(H5LTget_dataset_ndims(file, "/Model/Diffusion/Lattice", &ndims));
        if (ndims != 4) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Diffusion/Lattice");
        HDF5_EXCEPTION_CHECK(H5LTget_dataset_info(file, "/Model/Diffusion/Lattice",dims, &type, &size));
        if (lattice->lattice_x_size() != (int)dims[0] || lattice->lattice_y_size() != (int)dims[1] || lattice->lattice_z_size() != (int)dims[2] || lattice->particles_per_site() != (int)dims[3] || size != sizeof(uint8_t)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Diffusion/Lattice");
        string* particles=new string();
        particles->resize(dims[0]*dims[1]*dims[2]*dims[3]);
        HDF5_EXCEPTION_CHECK(H5LTread_dataset(file, "/Model/Diffusion/Lattice", H5T_NATIVE_UINT8, &((*particles)[0])));
        lattice->set_allocated_particles(particles);
        lattice->set_particles_ordering(lm::types::ROW_MAJOR);

        // Read the initial lattice sites.
        HDF5_EXCEPTION_CHECK(H5LTget_dataset_ndims(file, "/Model/Diffusion/LatticeSites", &ndims));
        if (ndims != 3) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Diffusion/LatticeSites");
        HDF5_EXCEPTION_CHECK(H5LTget_dataset_info(file, "/Model/Diffusion/LatticeSites",dims, &type, &size));
        if (lattice->lattice_x_size() != (int)dims[0] || lattice->lattice_y_size() != (int)dims[1] || lattice->lattice_z_size() != (int)dims[2] || size != sizeof(uint8_t)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Diffusion/LatticeSites");
        string* sites=new string();
        sites->resize(dims[0]*dims[1]*dims[2]);
        HDF5_EXCEPTION_CHECK(H5LTread_dataset(file, "/Model/Diffusion/LatticeSites", H5T_NATIVE_UINT8, &((*sites)[0])));
        lattice->set_allocated_sites(sites);
        lattice->set_sites_ordering(lm::types::ROW_MAJOR);
    }
}

void Hdf5File::setDiffusionModel(lm::input::DiffusionModel * diffusionModel)
{
    // Validate that the model is consistent.
    if (diffusionModel == NULL) throw InvalidArgException("diffusionModel", "cannot be NULL");
    if (diffusionModel->number_species() == 0) throw InvalidArgException("diffusionModel.number_species", "cannot be zero");
    if (diffusionModel->number_site_types() == 0) throw InvalidArgException("diffusionModel.number_site_types", "cannot be zero");
    if (diffusionModel->diffusion_matrix_size() != (int)(diffusionModel->number_site_types()*diffusionModel->number_site_types()*diffusionModel->number_species())) throw InvalidArgException("diffusion.diffusion_matrix", "inconsistent size");
    if (diffusionModel->reaction_location_matrix_size() != (int)(diffusionModel->number_reactions()*diffusionModel->number_site_types())) throw InvalidArgException("diffusion.reaction_location_matrix", "inconsistent size");
    if (diffusionModel->lattice_spacing() <= 0.0) throw InvalidArgException("diffusionModel.lattice_spacing", "must be greater than zero");
    if (diffusionModel->initial_lattice().lattice_x_size() == 0) throw InvalidArgException("diffusionModel.lattice_x_size", "cannot be zero");
    if (diffusionModel->initial_lattice().lattice_y_size() == 0) throw InvalidArgException("diffusionModel.lattice_y_size", "cannot be zero");
    if (diffusionModel->initial_lattice().lattice_z_size() == 0) throw InvalidArgException("diffusionModel.lattice_z_size", "cannot be zero");
    if (diffusionModel->initial_lattice().particles_per_site() == 0) throw InvalidArgException("diffusionModel.particles_per_site", "cannot be zero");

    // If a diffusion model already exists, delete it.
    if (H5Lexists(file, "/Model/Diffusion", H5P_DEFAULT))
    {
        HDF5_EXCEPTION_CHECK(H5Ldelete(file, "/Model/Diffusion", H5P_DEFAULT));
    }

    // Create the group for the reaction model.
    hid_t group;
    HDF5_EXCEPTION_CALL(group,H5Gcreate2(file, "/Model/Diffusion", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    HDF5_EXCEPTION_CHECK(H5Gclose(group));

    // Write the attributes.
    numberSpecies = diffusionModel->number_species();
    uint numberReactions = diffusionModel->number_reactions();
    uint numberSiteTypes = diffusionModel->number_site_types();
    double latticeSpacing = diffusionModel->lattice_spacing();
    uint latticeXSize = diffusionModel->initial_lattice().lattice_x_size();
    uint latticeYSize = diffusionModel->initial_lattice().lattice_y_size();
    uint latticeZSize = diffusionModel->initial_lattice().lattice_z_size();
    uint particlesPerSite = diffusionModel->initial_lattice().particles_per_site();
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(file, "/Model/Diffusion", "numberSpecies", &numberSpecies, 1));
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(file, "/Model/Diffusion", "numberReactions", &numberReactions, 1));
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(file, "/Model/Diffusion", "numberSiteTypes", &numberSiteTypes, 1));
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_double(file, "/Model/Diffusion", "latticeSpacing", &latticeSpacing, 1));
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(file, "/Model/Diffusion", "latticeXSize", &latticeXSize, 1));
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(file, "/Model/Diffusion", "latticeYSize", &latticeYSize, 1));
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(file, "/Model/Diffusion", "latticeZSize", &latticeZSize, 1));
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(file, "/Model/Diffusion", "particlesPerSite", &particlesPerSite, 1));

    // Write the diffusion matrix.
    {
        const unsigned int RANK=3;
        hsize_t dims[RANK];
        dims[0] = numberSiteTypes;
        dims[1] = numberSiteTypes;
        dims[2] = numberSpecies;
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(file, "/Model/Diffusion/DiffusionMatrix", RANK, dims, H5T_IEEE_F64LE, diffusionModel->diffusion_matrix().data()));
    }

    // Write the reaction location matrix.
    if (numberReactions > 0)
    {
        const unsigned int RANK=2;
        hsize_t dims[RANK];
        dims[0] = numberReactions;
        dims[1] = numberSiteTypes;
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(file, "/Model/Diffusion/ReactionLocationMatrix", RANK, dims, H5T_STD_U32LE, diffusionModel->reaction_location_matrix().data()));
    }
}

void Hdf5File::setFFluxOutput(lm::io::FFluxOutput* ffluxOutput)
{
    // Validate the forward flux output
    //if (tilings==NULL) throw InvalidArgException("tilings", "cannot be NULL");
    // TODO_LOW: add checks

    hid_t tilingsGroup, tilingGroup, ffluxOutputGroup;

    // get handle to Tilings group
    HDF5_EXCEPTION_CALL(tilingsGroup, H5Gopen(file, "/Tilings", H5P_DEFAULT));

    // declare a stringstream for the Tiling group name (i.e. its ID number)
    std::stringstream tilingSS;

    // clear the stringstream
    tilingSS.str(std::string());
    tilingSS.clear();

    // write the Tiling group name to the stringstream
    tilingSS.fill('0');
    tilingSS.width(7);
    tilingSS << ffluxOutput->tiling_id();

    // get handle to Tiling group
    HDF5_EXCEPTION_CALL(tilingGroup, H5Gopen(tilingsGroup, tilingSS.str().c_str(), H5P_DEFAULT));

    // If the FFluxOutput group already exists, get the handle to it. Otherwise, create it
    if ((ffluxOutputGroup = H5Gopen2(tilingGroup, "FFluxOutput", H5P_DEFAULT))>=0) {}
    else {HDF5_EXCEPTION_CALL(ffluxOutputGroup, H5Gcreate2(tilingGroup, "FFluxOutput", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));}


//    // If the FFluxOutput group already exists, delete it
//    if (H5Lexists(tilingGroup, "FFluxOutput", H5P_DEFAULT))
//    {
//        HDF5_EXCEPTION_CHECK(H5Ldelete(tilingGroup, "FFluxOutput", H5P_DEFAULT));
//    }
//    HDF5_EXCEPTION_CALL(ffluxOutputGroup, H5Gcreate2(tilingGroup, "FFluxOutput", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));

    // write the attributes for the FFluxOutput
    hid_t attr, scalarSpace;
    // steps to create attribute using H5Acreate: H5Screate,H5Acreate,H5Awrite,H5Aclose,H5Sclose
    // create the scalar data space
    HDF5_EXCEPTION_CALL(scalarSpace, H5Screate(H5S_SCALAR));

    int32_t number_species = ffluxOutput->number_species();
    // if the attr exists, get a handle to it, otherwise create it and still get the handle
    if ((attr = H5Aopen(ffluxOutputGroup, "NumberSpecies", H5P_DEFAULT))>=0) {}
    else {HDF5_EXCEPTION_CALL(attr, H5Acreate(ffluxOutputGroup, "NumberSpecies", H5T_STD_I32LE, scalarSpace, H5P_DEFAULT, H5P_DEFAULT));}
    HDF5_EXCEPTION_CHECK(H5Awrite(attr, H5T_STD_I32LE, &number_species));
    HDF5_EXCEPTION_CHECK(H5Aclose(attr));

    uint64_t number_tiles = ffluxOutput->number_tiles();
    if ((attr = H5Aopen(ffluxOutputGroup, "NumberTiles", H5P_DEFAULT))>=0) {}
    else {HDF5_EXCEPTION_CALL(attr, H5Acreate(ffluxOutputGroup, "NumberTiles", H5T_STD_U64LE, scalarSpace, H5P_DEFAULT, H5P_DEFAULT));}
    HDF5_EXCEPTION_CHECK(H5Awrite(attr, H5T_STD_U64LE, &number_tiles));
    HDF5_EXCEPTION_CHECK(H5Aclose(attr));

    uint32_t tiling_id = ffluxOutput->tiling_id();
    if ((attr = H5Aopen(ffluxOutputGroup, "TilingID", H5P_DEFAULT))>0) {}
    else {HDF5_EXCEPTION_CALL(attr, H5Acreate(ffluxOutputGroup, "TilingID", H5T_STD_U32LE, scalarSpace, H5P_DEFAULT, H5P_DEFAULT));}
    HDF5_EXCEPTION_CHECK(H5Awrite(attr, H5T_STD_U32LE, &tiling_id));
    HDF5_EXCEPTION_CHECK(H5Aclose(attr));

    // free the scalar data space
    HDF5_EXCEPTION_CHECK(H5Sclose(scalarSpace));

    // write the datasets for the FFluxOutput
    hid_t directionGroup, lifecycleGroup;
    int outIndex;
    vector<string> directionStrings; directionStrings.push_back("FORWARD"); directionStrings.push_back("BACKWARD");
    vector<string> lifecycleStrings; lifecycleStrings.push_back("INITIAL"); lifecycleStrings.push_back("RUNNING"); lifecycleStrings.push_back("FINAL");

    if (ffluxOutput->has_final_output())
    {
        setFFluxFinalOutput(ffluxOutput, ffluxOutputGroup);
    }

    for (int i=0;i<ffluxOutput->basin_outputs_size();i++)
    {
        lm::io::FFluxOutput::BasinOutput* basinOut = ffluxOutput->mutable_basin_outputs(i);
        // If the group corresponding to the basin direction already exists, get the handle to it. Otherwise, create it
        if ((directionGroup = H5Gopen2(ffluxOutputGroup, directionStrings[basinOut->direction()].c_str(), H5P_DEFAULT))>=0) {}
        else {HDF5_EXCEPTION_CALL(directionGroup, H5Gcreate2(ffluxOutputGroup, directionStrings[basinOut->direction()].c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));}

        setFFluxBasinOutput(ffluxOutput, basinOut->direction(), directionGroup);

        HDF5_EXCEPTION_CHECK(H5Gclose(directionGroup));
    }

    for (int i=0;i<ffluxOutput->trajectory_outputs_size();i++)
    {
        // some versions of HDF5 complain if you try to write out empty datasets, so skip those
        if (ffluxOutput->trajectory_outputs(i).count_size()==0)
        {
            continue;
        }
        lm::io::FFluxOutput::TrajectoryOutput* trajOut = ffluxOutput->mutable_trajectory_outputs(i);
        outIndex = (trajOut->direction())*3 + trajOut->lifecycle();
        // If the group corresponding to the basin direction already exists, get the handle to it. Otherwise, create it
        if ((directionGroup = H5Gopen2(ffluxOutputGroup, directionStrings[trajOut->direction()].c_str(), H5P_DEFAULT))>=0) {}
        else {HDF5_EXCEPTION_CALL(directionGroup, H5Gcreate2(ffluxOutputGroup, directionStrings[trajOut->direction()].c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));}
        // If the group corresponding to the trajectory direction and lifecycle (INITIAL or FINAL) already exists, get the handle to it. Otherwise, create it
        if ((lifecycleGroup = H5Gopen2(directionGroup, lifecycleStrings[trajOut->lifecycle()].c_str(), H5P_DEFAULT))>=0) {}
        else {HDF5_EXCEPTION_CALL(lifecycleGroup, H5Gcreate2(directionGroup, lifecycleStrings[trajOut->lifecycle()].c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));}

        setFFluxTrajectoryOutput_Count(ffluxOutput, i, lifecycleGroup);
        setFFluxTrajectoryOutput_EdgeID(ffluxOutput, i, lifecycleGroup);
        setFFluxTrajectoryOutput_SpeciesCount(ffluxOutput, i, lifecycleGroup);
        setFFluxTrajectoryOutput_Time(ffluxOutput, i, lifecycleGroup);
        setFFluxTrajectoryOutput_TrajectoryID(ffluxOutput, i, lifecycleGroup);

        HDF5_EXCEPTION_CHECK(H5Gclose(lifecycleGroup));
        HDF5_EXCEPTION_CHECK(H5Gclose(directionGroup));
    }

    HDF5_EXCEPTION_CHECK(H5Gclose(ffluxOutputGroup));
    HDF5_EXCEPTION_CHECK(H5Gclose(tilingGroup));
    HDF5_EXCEPTION_CHECK(H5Gclose(tilingsGroup));
}

void Hdf5File::setFFluxBasinOutput(lm::io::FFluxOutput* ffluxOutput, int basinIndex, hid_t basinGroup)
{
    // get a pointer to the germane BasinOutput buf
    lm::io::FFluxOutput::BasinOutput* basinOut = ffluxOutput->mutable_basin_outputs(basinIndex);

    // write the attributes for this particular BasinOutput
    hid_t attr, scalarSpace;
    HDF5_EXCEPTION_CALL(scalarSpace, H5Screate(H5S_SCALAR));
    if (basinOut->has_flux_out_of_tile_zero())
    {
        double flux_out_of_tile_zero = basinOut->flux_out_of_tile_zero();
        HDF5_EXCEPTION_CALL(attr, H5Acreate(basinGroup, "FluxOutOfTileZero", H5T_IEEE_F64LE, scalarSpace, H5P_DEFAULT, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5Awrite(attr, H5T_IEEE_F64LE, &flux_out_of_tile_zero));
        HDF5_EXCEPTION_CHECK(H5Aclose(attr));
    }
    if (basinOut->has_switching_rate_constant())
    {
        double switching_rate_constant = basinOut->switching_rate_constant();
        HDF5_EXCEPTION_CALL(attr, H5Acreate(basinGroup, "SwitchingRateConstant", H5T_IEEE_F64LE, scalarSpace, H5P_DEFAULT, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5Awrite(attr, H5T_IEEE_F64LE, &switching_rate_constant));
        HDF5_EXCEPTION_CHECK(H5Aclose(attr));
    }
    if (basinOut->has_this_basin_last_visited_probability())
    {
        double this_basin_last_visited_probability = basinOut->this_basin_last_visited_probability();
        HDF5_EXCEPTION_CALL(attr, H5Acreate(basinGroup, "ThisBasinLastVisitedProbability", H5T_IEEE_F64LE, scalarSpace, H5P_DEFAULT, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5Awrite(attr, H5T_IEEE_F64LE, &this_basin_last_visited_probability));
        HDF5_EXCEPTION_CHECK(H5Aclose(attr));
    }
    if (basinOut->has_probability_i_weight())
    {
        double probability_i_weight = basinOut->probability_i_weight();
        HDF5_EXCEPTION_CALL(attr, H5Acreate(basinGroup, "ProbabilityIWeight", H5T_IEEE_F64LE, scalarSpace, H5P_DEFAULT, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5Awrite(attr, H5T_IEEE_F64LE, &probability_i_weight));
        HDF5_EXCEPTION_CHECK(H5Aclose(attr));
    }
    HDF5_EXCEPTION_CHECK(H5Sclose(scalarSpace));

    // write the datasets for this particular BasinOutput
    hid_t probabilityIToIPlusOneGroup, probabilityOneToIPlusOneGroup, normalizedProbabilityIGroup;
    hsize_t dims[1];
    uint number_tiles, tiling_id;

    if (basinOut->probability_i_to_i_plus_one().tile_vals_size() > 0)
    {
        dims[0] = basinOut->probability_i_to_i_plus_one().tile_vals_size();
        HDF5_EXCEPTION_CALL(probabilityIToIPlusOneGroup, H5Gcreate2(basinGroup, "ProbabilityIToIPlusOne", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(probabilityIToIPlusOneGroup, "TileIndices", 1, dims, H5T_STD_U32LE, basinOut->probability_i_to_i_plus_one().tile_indices().data()));
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(probabilityIToIPlusOneGroup, "TileVals", 1, dims, H5T_IEEE_F64LE, basinOut->probability_i_to_i_plus_one().tile_vals().data()));
        number_tiles = basinOut->probability_i_to_i_plus_one().number_tiles();
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(basinGroup, "ProbabilityIToIPlusOne", "NumberTiles", &number_tiles, 1));
        tiling_id = basinOut->probability_i_to_i_plus_one().tiling_id();
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(basinGroup, "ProbabilityIToIPlusOne", "TilingID", &tiling_id, 1));
        HDF5_EXCEPTION_CHECK(H5Gclose(probabilityIToIPlusOneGroup));
    }
    if (basinOut->probability_one_to_i_plus_one().tile_vals_size() > 0)
    {
        dims[0] = basinOut->probability_one_to_i_plus_one().tile_vals_size();
        HDF5_EXCEPTION_CALL(probabilityOneToIPlusOneGroup, H5Gcreate2(basinGroup, "ProbabilityOneToIPlusOne", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(probabilityOneToIPlusOneGroup, "TileIndices", 1, dims, H5T_STD_U32LE, basinOut->probability_one_to_i_plus_one().tile_indices().data()));
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(probabilityOneToIPlusOneGroup, "TileVals", 1, dims, H5T_IEEE_F64LE, basinOut->probability_one_to_i_plus_one().tile_vals().data()));
        number_tiles = basinOut->probability_one_to_i_plus_one().number_tiles();
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(basinGroup, "ProbabilityOneToIPlusOne", "NumberTiles", &number_tiles, 1));
        tiling_id = basinOut->probability_one_to_i_plus_one().tiling_id();
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(basinGroup, "ProbabilityOneToIPlusOne", "TilingID", &tiling_id, 1));
        HDF5_EXCEPTION_CHECK(H5Gclose(probabilityOneToIPlusOneGroup));
    }
    if (basinOut->normalized_probability_i().tile_vals_size() > 0)
    {
        dims[0] = basinOut->normalized_probability_i().tile_vals_size();
        HDF5_EXCEPTION_CALL(normalizedProbabilityIGroup, H5Gcreate2(basinGroup, "ProbabilityI", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(normalizedProbabilityIGroup, "TileIndices", 1, dims, H5T_STD_U32LE, basinOut->normalized_probability_i().tile_indices().data()));
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(normalizedProbabilityIGroup, "TileVals", 1, dims, H5T_IEEE_F64LE, basinOut->normalized_probability_i().tile_vals().data()));
        number_tiles = basinOut->normalized_probability_i().number_tiles();
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(basinGroup, "ProbabilityI", "NumberTiles", &number_tiles, 1));
        tiling_id = basinOut->normalized_probability_i().tiling_id();
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(basinGroup, "ProbabilityI", "TilingID", &tiling_id, 1));
        HDF5_EXCEPTION_CHECK(H5Gclose(normalizedProbabilityIGroup));
    }
    setTilingHist(basinOut->mutable_runs_per_phase(), "RunsPerPhase", basinGroup);
    setTilingHist(basinOut->mutable_time_per_phase(), "TimePerPhase", basinGroup);
}

void Hdf5File::setFFluxFinalOutput(lm::io::FFluxOutput* ffluxOutput, hid_t ffluxOutputGroup)
{
    // get a pointer to the FinalOutput buf
    lm::io::FFluxOutput::FinalOutput* finalOut = ffluxOutput->mutable_final_output();

    // write the attributes for the FinalOutput
    hid_t attr, scalarSpace;
    HDF5_EXCEPTION_CALL(scalarSpace, H5Screate(H5S_SCALAR));
    if (finalOut->has_probability_i_weight())
    {
        double probability_i_weight = finalOut->probability_i_weight();
        HDF5_EXCEPTION_CALL(attr, H5Acreate(ffluxOutputGroup, "ProbabilityIWeight", H5T_IEEE_F64LE, scalarSpace, H5P_DEFAULT, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5Awrite(attr, H5T_IEEE_F64LE, &probability_i_weight));
        HDF5_EXCEPTION_CHECK(H5Aclose(attr));
    }
    if (finalOut->switching_rate_constants_size() > 0)
    {
        std::stringstream srcLabel;
        for (int i=0;i<finalOut->switching_rate_constants_size();i++)
        {
            srcLabel.str(std::string());
            srcLabel.clear();
            srcLabel << "SwitchingRateConstant_FromBasin";
            srcLabel << i;
            double switching_rate_constant = finalOut->switching_rate_constants(i);
            HDF5_EXCEPTION_CALL(attr, H5Acreate(ffluxOutputGroup, srcLabel.str().c_str(), H5T_IEEE_F64LE, scalarSpace, H5P_DEFAULT, H5P_DEFAULT));
            HDF5_EXCEPTION_CHECK(H5Awrite(attr, H5T_IEEE_F64LE, &switching_rate_constant));
            HDF5_EXCEPTION_CHECK(H5Aclose(attr));
        }
    }
    HDF5_EXCEPTION_CHECK(H5Sclose(scalarSpace));

    // write the datasets for the FinalOutput
    hid_t normalizedProbabilityIGroup;
    hsize_t dims[1];
    uint number_tiles, tiling_id;

    dims[0] = finalOut->normalized_probability_i().tile_vals_size();
    HDF5_EXCEPTION_CALL(normalizedProbabilityIGroup, H5Gcreate2(ffluxOutputGroup, "ProbabilityI", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    // TODO: intermediateOutputFlag doesn't exist anymore. Figure out some way to refactor/remove all of this surrounding code
    // if the intermediateOutputFlag is not set, TileIndices will be supressed, so account for that possibility
    if (finalOut->normalized_probability_i().tile_indices_size() > 0) {HDF5_EXCEPTION_CHECK(H5LTmake_dataset(normalizedProbabilityIGroup, "TileIndices", 1, dims, H5T_STD_U32LE, finalOut->normalized_probability_i().tile_indices().data()));}
    HDF5_EXCEPTION_CHECK(H5LTmake_dataset(normalizedProbabilityIGroup, "TileVals", 1, dims, H5T_IEEE_F64LE, finalOut->normalized_probability_i().tile_vals().data()));
    number_tiles = finalOut->normalized_probability_i().number_tiles();
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(ffluxOutputGroup, "ProbabilityI", "NumberTiles", &number_tiles, 1));
    tiling_id = finalOut->normalized_probability_i().tiling_id();
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(ffluxOutputGroup, "ProbabilityI", "TilingID", &tiling_id, 1));
    HDF5_EXCEPTION_CHECK(H5Gclose(normalizedProbabilityIGroup));
}

void Hdf5File::setFFluxTrajectoryOutput_Count(lm::io::FFluxOutput* ffluxOutput, int outIndex, hid_t lifecycleGroup)
{
    lm::io::FFluxOutput::TrajectoryOutput* trajOut = ffluxOutput->mutable_trajectory_outputs(outIndex);
    hsize_t dims[1];
    dims[0] = ffluxOutput->trajectory_outputs(outIndex).count_size();
    _setFFluxTrajectoryOutput(trajOut->count(), dims, "Count", H5T_IEEE_F64LE, lifecycleGroup, 1);
}

void Hdf5File::setFFluxTrajectoryOutput_EdgeID(lm::io::FFluxOutput* ffluxOutput, int outIndex, hid_t lifecycleGroup)
{
    lm::io::FFluxOutput::TrajectoryOutput* trajOut = ffluxOutput->mutable_trajectory_outputs(outIndex);
    hsize_t dims[1];
    dims[0] = ffluxOutput->trajectory_outputs(outIndex).edge_id_size();
    _setFFluxTrajectoryOutput(trajOut->edge_id(), dims, "EdgeID", H5T_STD_U64LE, lifecycleGroup, 1);
}

void Hdf5File::setFFluxTrajectoryOutput_SpeciesCount(lm::io::FFluxOutput* ffluxOutput, int outIndex, hid_t lifecycleGroup)
{
    lm::io::FFluxOutput::TrajectoryOutput* trajOut = ffluxOutput->mutable_trajectory_outputs(outIndex);
    hsize_t dims[2];
    dims[0] = trajOut->species_count_size()/ffluxOutput->number_species();
    dims[1] = ffluxOutput->number_species();
    _setFFluxTrajectoryOutput(trajOut->species_count(), dims, "SpeciesCount", H5T_STD_I32LE, lifecycleGroup, 2);
}

void Hdf5File::setFFluxTrajectoryOutput_Time(lm::io::FFluxOutput* ffluxOutput, int outIndex, hid_t lifecycleGroup)
{
    lm::io::FFluxOutput::TrajectoryOutput* trajOut = ffluxOutput->mutable_trajectory_outputs(outIndex);
    hsize_t dims[1];
    dims[0] = ffluxOutput->trajectory_outputs(outIndex).time_size();
    _setFFluxTrajectoryOutput(trajOut->time(), dims, "Time", H5T_IEEE_F64LE, lifecycleGroup, 1);
}

void Hdf5File::setFFluxTrajectoryOutput_TrajectoryID(lm::io::FFluxOutput* ffluxOutput, int outIndex, hid_t lifecycleGroup)
{
    lm::io::FFluxOutput::TrajectoryOutput* trajOut = ffluxOutput->mutable_trajectory_outputs(outIndex);
    hsize_t dims[1];
    dims[0] = ffluxOutput->trajectory_outputs(outIndex).trajectory_id_size();
    _setFFluxTrajectoryOutput(trajOut->trajectory_id(), dims, "TrajectoryID", H5T_STD_U64LE, lifecycleGroup, 1);
}

template <typename T>
void Hdf5File::_setFFluxTrajectoryOutput(::google::protobuf::RepeatedField<T> dataField, hsize_t* dims, string dsetName, hid_t dsetType, hid_t lifecycleGroup, uint RANK)
{
    // get a pointer to the TrajectoryOutput buf


    // write the attributes for this particular TrajectoryOutput

    // write the datasets for this particular TrajectoryOutput (adapted from the h5_extend.c tutorial)
    // variables related to the 1D datasets (Count, EdgeID, Time, TrajectoryID)
    hid_t dataspace, dataset, filespace, memspace, prop;
    hsize_t chunkdims[RANK], dimsr[RANK], dimstotal[RANK], maxdims[RANK], offset[RANK];

    // write or extend the 1D Count dataset
    chunkdims[0] = 100;
    maxdims[0] = H5S_UNLIMITED;
    if (RANK==2)
    {
        chunkdims[1] = dims[1];
        maxdims[1] = H5S_UNLIMITED;
    }

    // if the dataset exists, extend it
    if ((dataset = H5Dopen2(lifecycleGroup, dsetName.c_str(), H5P_DEFAULT))>=0)
    {
        HDF5_EXCEPTION_CALL(prop, H5Dget_create_plist (dataset));

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
        HDF5_EXCEPTION_CHECK(H5Dwrite(dataset, dsetType, memspace, filespace, H5P_DEFAULT, dataField.data()));

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
        dataset = H5Dcreate2(lifecycleGroup, dsetName.c_str(), dsetType, dataspace, H5P_DEFAULT, prop, H5P_DEFAULT);
        /* Write dataField to dataset */
        HDF5_EXCEPTION_CHECK(H5Dwrite(dataset, dsetType, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataField.data()));

        HDF5_EXCEPTION_CHECK(H5Dclose(dataset));
        HDF5_EXCEPTION_CHECK(H5Pclose(prop));
        HDF5_EXCEPTION_CHECK(H5Sclose(dataspace));
    }
}

void Hdf5File::setTilingHist(lm::io::TilingHist* tilingHist, std::string datasetName, hid_t superGroup)
{
    if (tilingHist->tile_vals_size() > 0)
    {
        hid_t thGroup;
        hsize_t dims[1];
        uint number_tiles, tiling_id;

        dims[0] = tilingHist->tile_vals_size();
        HDF5_EXCEPTION_CALL(thGroup, H5Gcreate2(superGroup, datasetName.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(thGroup, "TileIndices", 1, dims, H5T_STD_U32LE, tilingHist->tile_indices().data()));
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(thGroup, "TileVals", 1, dims, H5T_IEEE_F64LE, tilingHist->tile_vals().data()));
        number_tiles = tilingHist->number_tiles();
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(superGroup, datasetName.c_str(), "NumberTiles", &number_tiles, 1));
        tiling_id = tilingHist->tiling_id();
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(superGroup, datasetName.c_str(), "TilingID", &tiling_id, 1));
        HDF5_EXCEPTION_CHECK(H5Gclose(thGroup));
    }
}

bool Hdf5File::hasOrderParameters() const
{
    return (H5Lexists(file, "/OrderParameters", H5P_DEFAULT)!=0);
}

herr_t Hdf5File::getOrderParametersCallback(hid_t loc_id, const char * name, const H5L_info_t * info, void * callbackDataOrderParameters)
{
    // Declare and initialize handle for the order parameter group
    hid_t opGroup;
    HDF5_EXCEPTION_CALL(opGroup, H5Gopen(loc_id, name, H5P_DEFAULT));

    // recast the callbackData structure away from void *
    CallbackDataOrderParameters* cdOP = (CallbackDataOrderParameters*)callbackDataOrderParameters;

    // create a new order parameter in the ffluxParameters protobuf
    lm::input::OrderParameter* newOP = cdOP->orderParameters->add_order_parameters();

    // get the order parameter type and ID
    uint type, id;
    HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(loc_id, name, "Type", &type));
    HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(loc_id, name, "ID", &id));
    newOP->set_type(type);
    newOP->set_id(id);

    // read in the ids of the species that this order parameter uses, as well as their associated coefficients
    hsize_t dims[1];
    H5T_class_t hdf5Type;
    size_t size;

    H5LTget_dataset_info(opGroup, "SpeciesIDs", dims, &hdf5Type, &size);
    int * speciesBuffer = new int[dims[0]];
    H5LTread_dataset_int(opGroup, "SpeciesIDs", speciesBuffer);
    for (int i=0;i<dims[0];i++)
    {
        newOP->add_species_ids(speciesBuffer[i]);
    }

    H5LTget_dataset_info(opGroup, "SpeciesCoefficients", dims, &hdf5Type, &size);
    double* coefficientBuffer = new double[dims[0]];
    H5LTread_dataset_double(opGroup, "SpeciesCoefficients", coefficientBuffer);
    for (int i=0;i<dims[0];i++)
    {
        newOP->add_species_coefficients(coefficientBuffer[i]);
    }

    // free the buffers
    delete[] speciesBuffer;
    delete[] coefficientBuffer;

    // free the group handle
    HDF5_EXCEPTION_CHECK(H5Gclose(opGroup));

    return 0;
}

void Hdf5File::getOrderParameters(lm::input::OrderParameters* orderParameters) const
{
    // Make sure the orderParameters protobuf is not null and then clear it
    if (orderParameters == NULL) throw InvalidArgException("orderParameters", "cannot be null");
    orderParameters->Clear();

    // Declare and initialize the data structure for the callbacks in the order parameters iterator
    CallbackDataOrderParameters* opCD = new CallbackDataOrderParameters;
    opCD->orderParameters = orderParameters;

    if (H5Lexists(file, "/OrderParameters", H5P_DEFAULT))
    {
        H5Literate_by_name(file, "/OrderParameters", H5_INDEX_NAME, H5_ITER_INC, NULL, getOrderParametersCallback, (void *)opCD, H5P_DEFAULT);
    }
}

void Hdf5File::setOrderParameters(lm::input::OrderParameters * orderParameters)
{
    // Validate the set of order parameters
    if (orderParameters==NULL) throw InvalidArgException("orderParameters", "cannot be NULL");
    // TODO_LOW: add more checks

    // If a set of order parameters exist, delete it
    if (H5Lexists(file, "/OrderParameters", H5P_DEFAULT))
    {
        HDF5_EXCEPTION_CHECK(H5Ldelete(file, "/OrderParameters", H5P_DEFAULT));
    }

    hid_t opsGroup;
    HDF5_EXCEPTION_CALL(opsGroup, H5Gcreate2(file, "/OrderParameters", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));

    // If there are any order parameters, write out the relevant tables
    uint id, type;
    hid_t opGroup;
    hsize_t opDims[1];
    std::stringstream opSS; // declare a stringstream for the order parameter's group name
    for (int i=0;i<orderParameters->order_parameters_size();i++)
    {
        // get the order parameter's attribute data from the corresponding protobuf
        id = orderParameters->order_parameters(i).id();
        type = orderParameters->order_parameters(i).type();

        // clear the stringstream with the order parameter's group name
        opSS.str(std::string());
        opSS.clear();

        // write the order parameter's group name to the stringstream
        opSS.fill('0');
        opSS.width(7);
        opSS << id;

        // write the order parameter's attributes
        HDF5_EXCEPTION_CALL(opGroup, H5Gcreate2(opsGroup, opSS.str().c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(opsGroup, opSS.str().c_str(), "ID", &id, 1));
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(opsGroup, opSS.str().c_str(), "Type", &type, 1));

        // write the order parameter's datasets
        opDims[0] = orderParameters->order_parameters(i).species_ids().size();
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(opGroup, "SpeciesIDs", 1, opDims, H5T_STD_U32LE, orderParameters->order_parameters(i).species_ids().data()));
        opDims[0] = orderParameters->order_parameters(i).species_coefficients().size();
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(opGroup, "SpeciesCoefficients", 1, opDims, H5T_IEEE_F64LE, orderParameters->order_parameters(i).species_coefficients().data()));
        HDF5_EXCEPTION_CHECK(H5Gclose(opGroup));
    }
    HDF5_EXCEPTION_CHECK(H5Gclose(opsGroup));
}

bool Hdf5File::hasReactionModel() const
{
    return (H5Lexists(file, "/Model/Reaction", H5P_DEFAULT) != 0);
}

void Hdf5File::getReactionModel(lm::input::ReactionModel * reactionModel) const
{
    // Make sure the model is not null and then clear it.
    if (reactionModel == NULL) throw InvalidArgException("reactionModel", "cannot be null");
    reactionModel->Clear();

    if (H5Lexists(file, "/Model/Reaction", H5P_DEFAULT))
    {
        // Read at least the numbers of species.
        unsigned int constNumberSpecies;
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Model/Reaction", "numberSpecies", &constNumberSpecies));
        reactionModel->set_number_species(constNumberSpecies);
        reactionModel->set_number_reactions(0);

        // If we have the number of reactions, we must have a full model so read it.
        if (H5Aexists_by_name(file, "/Model/Reaction", "numberReactions", H5P_DEFAULT) > 0)
        {
            hsize_t dims[2];
            H5T_class_t type;
            size_t size;

            // Read the initial species counts.
            H5LTget_dataset_info(file, "/Model/Reaction/InitialSpeciesCounts", dims, &type, &size);
            if (dims[0] != constNumberSpecies || size != sizeof(uint)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Reaction/InitialSpeciesCounts");

            // Read the number of reactions.
            uint numberReactions;
            HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Model/Reaction", "numberReactions", &numberReactions));
            reactionModel->set_number_reactions(numberReactions);

            // Read the reaction tables.
            if (numberReactions > 0)
            {
                // Make sure all of the data sets are the correct size.
                H5LTget_dataset_info(file, "/Model/Reaction/ReactionTypes", dims, &type, &size);
                if (dims[0] != numberReactions || size != sizeof(uint)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Reaction/ReactionTypes");
                H5LTget_dataset_info(file, "/Model/Reaction/ReactionRateConstants", dims, &type, &size);
                if (dims[0] != numberReactions || dims[1] != MAX_REACTION_RATE_CONSTANTS || size != sizeof(double)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Reaction/ReactionRateConstants");
                H5LTget_dataset_info(file, "/Model/Reaction/StoichiometricMatrix", dims, &type, &size);
                if (dims[0] != constNumberSpecies || dims[1] != numberReactions || size != sizeof(int)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Reaction/StoichiometricMatrix");
                H5LTget_dataset_info(file, "/Model/Reaction/DependencyMatrix", dims, &type, &size);
                if (dims[0] != constNumberSpecies || dims[1] != numberReactions || size != sizeof(uint)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Reaction/DependencyMatrix");

                // If we have rate noise terms, make sure they are the correct size.
                const uint NUMBER_NOISE_COLS = 2;
                bool hasNoiseTable = false;
                if (H5Lexists(file, "/Model/Reaction/ReactionRateNoise", H5P_DEFAULT))
                {
                    hasNoiseTable = true;
                    H5LTget_dataset_info(file, "/Model/Reaction/ReactionRateNoise", dims, &type, &size);
                    if (dims[0] != numberReactions || dims[1] != NUMBER_NOISE_COLS || size != sizeof(double)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Reaction/ReactionRateNoise");
                }

                // If we have an initial species counts for a reversed system, make sure it is the correct size
                bool hasInitialSpeciesCountsBackward = false;
                if (H5Lexists(file, "/Model/Reaction/InitialSpeciesCountsBackward", H5P_DEFAULT))
                {
                    hasInitialSpeciesCountsBackward = true;
                    H5LTget_dataset_info(file, "/Model/Reaction/InitialSpeciesCountsBackward", dims, &type, &size);
                    if (dims[0] != constNumberSpecies || size != sizeof(uint)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Reaction/InitialSpeciesCountsBackward");
                }

                // Allocate some buffers for reading the data.
                int * intBuffer = new int[constNumberSpecies*numberReactions];
                double * doubleBuffer = new double[numberReactions*MAX_REACTION_RATE_CONSTANTS];
                double * noiseBuffer = new double[numberReactions*NUMBER_NOISE_COLS];

                // Read the initial species counts.
                H5LTread_dataset_int(file, "/Model/Reaction/InitialSpeciesCounts", intBuffer);
                for (uint i=0; i<constNumberSpecies; i++) reactionModel->add_initial_species_count((uint)intBuffer[i]);
                if (hasInitialSpeciesCountsBackward)
                {
                    H5LTread_dataset_int(file, "/Model/Reaction/InitialSpeciesCountsBackward", intBuffer);
                    for (uint i=0; i<constNumberSpecies; i++) reactionModel->add_initial_species_count_backward((uint)intBuffer[i]);
                }

                // Read the reaction info.
                H5LTread_dataset_int(file, "/Model/Reaction/ReactionTypes", intBuffer);
                H5LTread_dataset_double(file, "/Model/Reaction/ReactionRateConstants", doubleBuffer);
                if (hasNoiseTable) H5LTread_dataset_double(file, "/Model/Reaction/ReactionRateNoise", noiseBuffer);
                for (uint i=0; i<numberReactions; i++)
                {
                    reactionModel->add_reaction();
                    reactionModel->mutable_reaction(i)->set_type((uint)intBuffer[i]);
                    for (uint j=0; j<MAX_REACTION_RATE_CONSTANTS; j++)
                    {
                        double k = doubleBuffer[i*MAX_REACTION_RATE_CONSTANTS+j];
                        if (!std::isnan(k))
                            reactionModel->mutable_reaction(i)->add_rate_constant(k);
                        else
                            break;
                    }

                    // If we have noise terms, set them.
                    if (hasNoiseTable)
                    {
                        double nvar = noiseBuffer[i*NUMBER_NOISE_COLS];
                        double ntau = noiseBuffer[i*NUMBER_NOISE_COLS+1];
                        if (nvar > 0.0 && ntau > 0.0 && !std::isnan(nvar) && !std::isnan(ntau))
                        {
                            reactionModel->mutable_reaction(i)->set_rate_has_noise(true);
                            reactionModel->mutable_reaction(i)->set_rate_noise_variance(nvar);
                            reactionModel->mutable_reaction(i)->set_rate_noise_tau(ntau);
                        }
                    }
                }

                // Read the matrices.
                H5LTread_dataset_int(file, "/Model/Reaction/StoichiometricMatrix", intBuffer);
                for (uint i=0; i<constNumberSpecies*numberReactions; i++) reactionModel->add_stoichiometric_matrix(intBuffer[i]);
                H5LTread_dataset_int(file, "/Model/Reaction/DependencyMatrix", intBuffer);
                for (uint i=0; i<constNumberSpecies*numberReactions; i++) reactionModel->add_dependency_matrix((uint)intBuffer[i]);

                // Free the buffers.
                delete [] noiseBuffer;
                delete [] doubleBuffer;
                delete [] intBuffer;
            }
        }
    }
}

void Hdf5File::setReactionModel(lm::input::ReactionModel * reactionModel)
{
    // Validate that the model is consistent.
    if (reactionModel == NULL) throw InvalidArgException("reactionModel", "cannot be NULL");
    if (reactionModel->number_species() == 0) throw InvalidArgException("reactionModel.number_species", "cannot be zero");
    if (reactionModel->initial_species_count_size() != (int)reactionModel->number_species()) throw InvalidArgException("reactionModel.initial_species_count", "inconsistent size");
    if (reactionModel->reaction_size() != (int)reactionModel->number_reactions()) throw InvalidArgException("reactionModel.reaction", "inconsistent size");
    if (reactionModel->stoichiometric_matrix_size() != (int)(reactionModel->number_species()*reactionModel->number_reactions())) throw InvalidArgException("reactionModel.stoichiometric_matrix", "inconsistent size");
    if (reactionModel->dependency_matrix_size() != (int)(reactionModel->number_species()*reactionModel->number_reactions())) throw InvalidArgException("reactionModel.dependency_matrix", "inconsistent size");

    // If a reaction model already exists, delete it.
    if (H5Lexists(file, "/Model/Reaction", H5P_DEFAULT))
    {
        HDF5_EXCEPTION_CHECK(H5Ldelete(file, "/Model/Reaction", H5P_DEFAULT));
    }

    // Create the group for the reaction model.
    hid_t group;
    HDF5_EXCEPTION_CALL(group,H5Gcreate2(file, "/Model/Reaction", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    HDF5_EXCEPTION_CHECK(H5Gclose(group));

    // Write the numbers of species and reactions.
    numberSpecies = reactionModel->number_species();
    uint numberReactions = reactionModel->number_reactions();
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(file, "/Model/Reaction", "numberSpecies", &numberSpecies, 1));
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(file, "/Model/Reaction", "numberReactions", &numberReactions, 1));

    hsize_t dims[2];

    // Write the initial species counts.
    dims[0] = numberSpecies;
    HDF5_EXCEPTION_CHECK(H5LTmake_dataset(file, "/Model/Reaction/InitialSpeciesCounts", 1, dims, H5T_STD_U32LE, reactionModel->initial_species_count().data()));
    // If we have them, write out the initial species counts for the reversed system
    if (reactionModel->initial_species_count_backward_size()>0)
    {
        if (reactionModel->initial_species_count_backward_size()!=reactionModel->initial_species_count_size()) throw InvalidArgException("reactionModel.initial_species_count_backward", "inconsistent size");
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(file, "/Model/Reaction/InitialSpeciesCountsBackward", 1, dims, H5T_STD_U32LE, reactionModel->initial_species_count_backward().data()));
    }

    // If we have any reactions, write out the reaction tables.
    if (reactionModel->number_reactions())
    {
        // Write the reaction tables.
        uint * types = new uint[numberReactions];
        double * constants = new double[numberReactions*MAX_REACTION_RATE_CONSTANTS];
        bool hasNoiseTable = false;
        const uint NUMBER_NOISE_COLS = 2;
        double * noiseTerms = new double[numberReactions*NUMBER_NOISE_COLS];
        for (uint i=0; i<numberReactions*NUMBER_NOISE_COLS; i++) noiseTerms[i] = 0.0;
        for (uint i=0; i<numberReactions; i++)
        {
            types[i] = reactionModel->reaction(i).type();
            uint j=0;
            for (; j<(uint)(reactionModel->reaction(i).rate_constant_size()) && j<MAX_REACTION_RATE_CONSTANTS; j++)
                constants[i*MAX_REACTION_RATE_CONSTANTS+j] = reactionModel->reaction(i).rate_constant(j);
            for (; j<MAX_REACTION_RATE_CONSTANTS; j++)
                constants[i*MAX_REACTION_RATE_CONSTANTS+j] = NAN;

            // If we have noise terms, fill them in and mark that we need to create the noise table.
            if (reactionModel->reaction(i).rate_has_noise())
            {
                hasNoiseTable = true;
                noiseTerms[i*NUMBER_NOISE_COLS] = reactionModel->reaction(i).rate_noise_variance();
                noiseTerms[i*NUMBER_NOISE_COLS+1] = reactionModel->reaction(i).rate_noise_tau();
            }
        }
        dims[0] = numberReactions;
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(file, "/Model/Reaction/ReactionTypes", 1, dims, H5T_STD_U32LE, types));
        dims[0] = numberReactions;
        dims[1] = MAX_REACTION_RATE_CONSTANTS;
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(file, "/Model/Reaction/ReactionRateConstants", 2, dims, H5T_IEEE_F64LE, constants));

        // If necessary, create the noise table.
        if (hasNoiseTable)
        {
            dims[0] = numberReactions;
            dims[1] = NUMBER_NOISE_COLS;
            HDF5_EXCEPTION_CHECK(H5LTmake_dataset(file, "/Model/Reaction/ReactionRateNoise", 2, dims, H5T_IEEE_F64LE, noiseTerms));
        }
        delete [] noiseTerms;
        delete [] constants;
        delete [] types;

        // Write the matrices.
        dims[0] = numberSpecies;
        dims[1] = numberReactions;
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(file, "/Model/Reaction/StoichiometricMatrix", 2, dims, H5T_STD_I32LE, reactionModel->stoichiometric_matrix().data()));
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(file, "/Model/Reaction/DependencyMatrix", 2, dims, H5T_STD_U32LE, reactionModel->dependency_matrix().data()));
    }
}

void Hdf5File::getSpatialModel(lm::input::SpatialModel * spatialModel) const
{
    // Make sure the model is not null and then clear it.
    if (spatialModel == NULL) throw InvalidArgException("spatialModel", "cannot be null");
    spatialModel->Clear();

    if (H5Lexists(file, "/Model/Spatial", H5P_DEFAULT))
    {
        // Read the diffusion model attributes.
        uint numberRegions, numberObstacles;
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Model/Spatial/Regions", "numberRegions", &numberRegions));
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Model/Spatial/Obstacles", "numberObstacles", &numberObstacles));

        hsize_t dims[2];
        H5T_class_t type;
        size_t size;

        if (numberRegions > 0)
        {
            // Read the region types table.
            {
                H5LTget_dataset_info(file, "/Model/Spatial/Regions/Types", dims, &type, &size);
                if (dims[0] != numberRegions || dims[1] != 2 || size != sizeof(uint)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Spatial/Regions/Types");
                uint * types = new uint[dims[0]*dims[1]];
                H5LTread_dataset(file, "/Model/Spatial/Regions/Types", H5T_STD_U32LE, types);
                for (uint i=0; i<dims[0]; i++)
                {
                    spatialModel->add_region();
                    spatialModel->mutable_region(i)->set_shape(types[i*dims[1]]);
                    spatialModel->mutable_region(i)->set_site_type(types[i*dims[1]+1]);
                }
                delete [] types;
            }

            // Read the shape parameters table.
            {
                H5LTget_dataset_info(file, "/Model/Spatial/Regions/ShapeParameters", dims, &type, &size);
                if (dims[0] != numberRegions || dims[1] != MAX_SHAPE_PARAMETERS || size != sizeof(double)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Spatial/Regions/ShapeParameters");
                double * shapeParams = new double[dims[0]*dims[1]];
                H5LTread_dataset_double(file, "/Model/Spatial/Regions/ShapeParameters", shapeParams);
                for (uint i=0; i<dims[0]; i++)
                {
                    for (uint j=0; j<dims[1]; j++)
                    {
                        double shapeParam = shapeParams[i*dims[1]+j];
                        if (!std::isnan(shapeParam))
                            spatialModel->mutable_region(i)->add_shape_parameter(shapeParam);
                        else
                            break;
                    }
                }
                delete [] shapeParams;
            }
        }

        if (numberObstacles > 0)
        {
            // Read the obstacle types table.
            {
                H5LTget_dataset_info(file, "/Model/Spatial/Obstacles/Types", dims, &type, &size);
                if (dims[0] != numberObstacles || dims[1] != 2 || size != sizeof(uint)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Spatial/Obstacles/Types");
                uint * types = new uint[dims[0]*dims[1]];
                H5LTread_dataset(file, "/Model/Spatial/Obstacles/Types", H5T_STD_U32LE, types);
                for (uint i=0; i<dims[0]; i++)
                {
                    spatialModel->add_obstacle();
                    spatialModel->mutable_obstacle(i)->set_shape(types[i*dims[1]]);
                    spatialModel->mutable_obstacle(i)->set_site_type(types[i*dims[1]+1]);
                }
                delete [] types;
            }

            // Read the obstacle parameters table.
            {
                H5LTget_dataset_info(file, "/Model/Spatial/Obstacles/ShapeParameters", dims, &type, &size);
                if (dims[0] != numberObstacles || dims[1] != MAX_SHAPE_PARAMETERS || size != sizeof(double)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Spatial/Obstacles/ShapeParameters");
                double * shapeParams = new double[dims[0]*dims[1]];
                H5LTread_dataset_double(file, "/Model/Spatial/Obstacles/ShapeParameters", shapeParams);
                for (uint i=0; i<dims[0]; i++)
                {
                    for (uint j=0; j<dims[1]; j++)
                    {
                        double shapeParam = shapeParams[i*dims[1]+j];
                        if (!std::isnan(shapeParam))
                            spatialModel->mutable_obstacle(i)->add_shape_parameter(shapeParam);
                        else
                            break;
                    }
                }
                delete [] shapeParams;
            }
        }
    }
}

void Hdf5File::setSpatialModel(lm::input::SpatialModel * spatialModel)
{
    // Validate that the model is consistent.
    if (spatialModel == NULL) throw InvalidArgException("spatialModel", "cannot be NULL");

    // If a diffusion model already exists, delete it.
    if (H5Lexists(file, "/Model/Spatial", H5P_DEFAULT))
    {
        HDF5_EXCEPTION_CHECK(H5Ldelete(file, "/Model/Spatial", H5P_DEFAULT));
    }

    // Create the group for the spatial model.
    hid_t group;
    HDF5_EXCEPTION_CALL(group,H5Gcreate2(file, "/Model/Spatial", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    HDF5_EXCEPTION_CHECK(H5Gclose(group));
    HDF5_EXCEPTION_CALL(group,H5Gcreate2(file, "/Model/Spatial/Regions", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    HDF5_EXCEPTION_CHECK(H5Gclose(group));
    HDF5_EXCEPTION_CALL(group,H5Gcreate2(file, "/Model/Spatial/Obstacles", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    HDF5_EXCEPTION_CHECK(H5Gclose(group));

    // Write the attributes.
    uint numberRegions = spatialModel->region_size();
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(file, "/Model/Spatial/Regions", "numberRegions", &numberRegions, 1));
    uint numberObstacles = spatialModel->obstacle_size();
    HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(file, "/Model/Spatial/Obstacles", "numberObstacles", &numberObstacles, 1));

    if (numberRegions > 0)
    {
        // Write the region types table.
        {
            const unsigned int RANK=2;
            hsize_t dims[RANK];
            dims[0] = numberRegions;
            dims[1] = 2;
            uint * types = new uint[dims[0]*dims[1]];
            for (uint i=0; i<dims[0]; i++)
            {
                types[i*dims[1]] = spatialModel->region(i).shape();
                types[i*dims[1]+1] = spatialModel->region(i).site_type();
            }
            HDF5_EXCEPTION_CHECK(H5LTmake_dataset(file, "/Model/Spatial/Regions/Types", RANK, dims, H5T_STD_U32LE, types));
            delete [] types;
        }

        // Write the shape parameters table.
        {
            const unsigned int RANK=2;
            hsize_t dims[RANK];
            dims[0] = numberRegions;
            dims[1] = MAX_SHAPE_PARAMETERS;
            double * shapeParams = new double[dims[0]*dims[1]];
            for (uint i=0; i<dims[0]; i++)
            {
                for (uint j=0; j<dims[1]; j++)
                {
                    if (j < (uint)spatialModel->region(i).shape_parameter_size())
                        shapeParams[i*dims[1]+j] = spatialModel->region(i).shape_parameter(j);
                    else
                        shapeParams[i*dims[1]+j] = NAN;
                }
            }
            HDF5_EXCEPTION_CHECK(H5LTmake_dataset(file, "/Model/Spatial/Regions/ShapeParameters", RANK, dims, H5T_IEEE_F64LE, shapeParams));
            delete [] shapeParams;
        }
    }

    if (numberObstacles > 0)
    {
        // Write the obstacle types table.
        {
            const unsigned int RANK=2;
            hsize_t dims[RANK];
            dims[0] = numberObstacles;
            dims[1] = 2;
            uint * types = new uint[dims[0]*dims[1]];
            for (uint i=0; i<dims[0]; i++)
            {
                types[i*dims[1]] = spatialModel->obstacle(i).shape();
                types[i*dims[1]+1] = spatialModel->obstacle(i).site_type();
            }
            HDF5_EXCEPTION_CHECK(H5LTmake_dataset(file, "/Model/Spatial/Obstacles/Types", RANK, dims, H5T_STD_U32LE, types));
            delete [] types;
        }

        // Write the shape parameters table.
        {
            const unsigned int RANK=2;
            hsize_t dims[RANK];
            dims[0] = numberObstacles;
            dims[1] = MAX_SHAPE_PARAMETERS;
            double * shapeParams = new double[dims[0]*dims[1]];
            for (uint i=0; i<dims[0]; i++)
            {
                for (uint j=0; j<dims[1]; j++)
                {
                    if (j < (uint)spatialModel->obstacle(i).shape_parameter_size())
                        shapeParams[i*dims[1]+j] = spatialModel->obstacle(i).shape_parameter(j);
                    else
                        shapeParams[i*dims[1]+j] = NAN;
                }
            }
            HDF5_EXCEPTION_CHECK(H5LTmake_dataset(file, "/Model/Spatial/Obstacles/ShapeParameters", RANK, dims, H5T_IEEE_F64LE, shapeParams));
            delete [] shapeParams;
        }
    }
}

bool Hdf5File::hasTilings() const
{
    return (H5Lexists(file, "/Tilings", H5P_DEFAULT)!=0);
}

herr_t Hdf5File::getTilingsCallback(hid_t loc_id, const char * name, const H5L_info_t * info, void * callbackDataTilings)
{
    // Declare and initialize handle for the tiling group
    hid_t tilingGroup;
    HDF5_EXCEPTION_CALL(tilingGroup, H5Gopen(loc_id, name, H5P_DEFAULT));

    // recast the callbackData structure away from void *
    CallbackDataTilings* cdT = (CallbackDataTilings *)callbackDataTilings;

    // create a new interface in the tilings protobuf
    lm::input::Tiling* newTiling = cdT->tilings->add_tilings();

    // get the ID and Type of the tiling and the ID of the order parameter associated with this tiling
    uint id, type, opID;
    HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(loc_id, name, "ID", &id));
    HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(loc_id, name, "OrderParameterID", &opID));
    HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(loc_id, name, "Type", &type));
    newTiling->set_id(id);
    newTiling->add_order_parameter_ids(opID);
    newTiling->set_type(type);

    // read in the values of the tiling's edges
    {
        hsize_t dims[1];
        H5T_class_t hdf5Type;
        size_t size;

        HDF5_EXCEPTION_CHECK(H5LTget_dataset_info(tilingGroup, "Edges", dims, &hdf5Type, &size));
        double* edgeBuffer = new double[dims[0]];
        HDF5_EXCEPTION_CHECK(H5LTread_dataset_double(tilingGroup, "Edges", edgeBuffer));
        for (int i = 0; i < dims[0]; i++)
        {
            newTiling->add_edges(edgeBuffer[i]);
        }

        // free the buffer
        delete[] edgeBuffer;
    }

    // read in the values of the tiling's basins
    herr_t basinsExists;
    HDF5_EXCEPTION_CALL(basinsExists, H5LTfind_dataset(tilingGroup, "Basins"))
    if (basinsExists > 0)
    {
        int rank;
        hsize_t dims[2];
        H5T_class_t hdf5Type;
        size_t size;

        // sanity check the rank of the basins dataset
        HDF5_EXCEPTION_CHECK(H5LTget_dataset_ndims(tilingGroup, "Basins", &rank));
        if (rank != 2) THROW_EXCEPTION(IOException, "Rank of Basins dataset invalid.\n"
            "Please ensure that all Basins datasets are 2D in your input .lm file. rank: %d", rank);

        HDF5_EXCEPTION_CHECK(H5LTget_dataset_info(tilingGroup, "Basins", dims, &hdf5Type, &size));
        double* basinsBuffer = new double[dims[0]*dims[1]];
        HDF5_EXCEPTION_CHECK(H5LTread_dataset_double(tilingGroup, "Basins", basinsBuffer));

        for (uint i=0; i<dims[0]; i++)
        {
            lm::input::Basin* newBasin = newTiling->add_basins();
            for (uint j=0; j<dims[1]; j++)
            {
                newBasin->add_species_count(basinsBuffer[i*dims[1]+j]);
            }
        }

        // free the buffers
        delete[] basinsBuffer;
    }

    // infer whether edges is sorted ascending or descending
    TilingEnums::SortOrder sortOrder = newTiling->edges(newTiling->edges_size()-1)>=newTiling->edges(0) ? TilingEnums::ASCENDING : TilingEnums::DESCENDING;
    newTiling->add_sort_orders(sortOrder);

    // ensure that edges is actually sorted the way we guessed
    if (sortOrder==TilingEnums::ASCENDING)
    {
        for (int i=0;i<newTiling->edges_size()-1;i++)
        {
            if (newTiling->edges(i) > newTiling->edges(i+1)) throw Exception("A set of Edges in one of your Tilings is improperly sorted (guessed ASCENDING)", cdT->filename.c_str(), "/Tilings/xxxxxxx/Edges");
        }
    }
    else // (sortOrder==lm::input::Tilings::DESCENDING)
    {
        for (int i=0;i<newTiling->edges_size()-1;i++)
        {
            if (newTiling->edges(i+1) > newTiling->edges(i)) throw Exception("A set of Edges in one of your Tilings is improperly sorted (guessed DESCENDING)", cdT->filename.c_str(), "/Tilings/xxxxxxx/Edges");
        }
    }


    // free the group handle
    HDF5_EXCEPTION_CHECK(H5Gclose(tilingGroup));

    return 0;
}

void Hdf5File::getTilings(lm::input::Tilings* tilings) const
{
    // Make sure the tilings protobuf is not null and then clear it
    if (tilings == NULL) throw InvalidArgException("tilings", "cannot be null");
    tilings->Clear();

    // Declare and initialize the data structure for the callbacks in the tilings iterator
    CallbackDataTilings* cdT = new CallbackDataTilings;
    cdT->tilings = tilings;
    cdT->filename = filename;

    uint32_t currentTilingID;
    if (H5Lexists(file, "/Tilings", H5P_DEFAULT))
    {
        htri_t currentTilingIDExists;
        HDF5_EXCEPTION_CALL(currentTilingIDExists, H5Aexists_by_name(file, "/Tilings", "CurrentTilingID", H5P_DEFAULT))
        if (currentTilingIDExists > 0)
        {
            HDF5_EXCEPTION_CHECK(H5LTget_attribute_uint(file, "/Tilings", "CurrentTilingID", &currentTilingID));
            tilings->set_current_tiling_id(currentTilingID);
        }
        H5Literate_by_name(file, "/Tilings", H5_INDEX_NAME, H5_ITER_INC, NULL, getTilingsCallback, (void *)cdT, H5P_DEFAULT);
    }
}

void Hdf5File::setTilings(lm::input::Tilings * tilings)
{
    // Validate the set of tililngs
    if (tilings==NULL) throw InvalidArgException("tilings", "cannot be NULL");
    // TODO_LOW: add more checks

    // If a set of tilings exist, delete it
    if (H5Lexists(file, "/Tilings", H5P_DEFAULT))
    {
        HDF5_EXCEPTION_CHECK(H5Ldelete(file, "/Tilings", H5P_DEFAULT));
    }

    hid_t tilingsGroup;

    HDF5_EXCEPTION_CALL(tilingsGroup, H5Gcreate2(file, "/Tilings", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    if (tilings->has_current_tiling_id())
    {
        uint CurrentTilingID = tilings->current_tiling_id();
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(file, "/Tilings", "CurrentTilingID", &CurrentTilingID, 1));
    }

    // If there are any sets of tilings, write out the relevant tables
    uint id, opID, type;
    hid_t tilingGroup;
    hsize_t binDims[1];
    std::stringstream tilingSS; // declare a stringstream for the tiling's group name
    for (int i=0;i<tilings->tilings_size();i++)
    {
        // get a pointer to the right tiling buf
        lm::input::Tiling* tilingBuf = tilings->mutable_tilings(i);

        // get the tiling's attribute data from the corresponding protobuf
        id = tilingBuf->id();
        opID = tilingBuf->order_parameter_ids(0);
        type = tilingBuf->type();

        // clear the stringstream with the tiling's group name
        tilingSS.str(std::string());
        tilingSS.clear();

        // write the tiling's group name to the stringstream
        tilingSS.fill('0');
        tilingSS.width(7);
        tilingSS << id;

        // write the tiling's attributes
        HDF5_EXCEPTION_CALL(tilingGroup, H5Gcreate2(tilingsGroup, tilingSS.str().c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(tilingsGroup, tilingSS.str().c_str(), "ID", &id, 1));
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(tilingsGroup, tilingSS.str().c_str(), "OrderParameterID", &opID, 1));
        HDF5_EXCEPTION_CHECK(H5LTset_attribute_uint(tilingsGroup, tilingSS.str().c_str(), "Type", &type, 1));

        // write the tiling's datasets
        binDims[0] = tilingBuf->edges_size();
        HDF5_EXCEPTION_CHECK(H5LTmake_dataset(tilingGroup, "Edges", 1, binDims, H5T_IEEE_F64LE, tilingBuf->edges().data()));
        HDF5_EXCEPTION_CHECK(H5Gclose(tilingGroup));
    }
    HDF5_EXCEPTION_CHECK(H5Gclose(tilingsGroup));
}

bool Hdf5File::hasBoundaryGradient() const
{
    return (H5Lexists(file, "/Model/Diffusion/Gradient", H5P_DEFAULT) != 0);
}

void Hdf5File::getBoundaryGradient(lm::types::BoundaryConditions* bc) const
{
    // Make sure the model is not null and then clear it.
    if (bc == NULL) throw InvalidArgException("bc", "cannot be null");
    bc->clear_boundary_gradient_ordering();
    bc->clear_boundary_gradient();

    if (H5Lexists(file, "/Model/Diffusion/Gradient", H5P_DEFAULT))
    {
        int ndims;
        hsize_t dims[3];
        H5T_class_t type;
        size_t size;

        // Read the lattice size.
        int latticeXSize,latticeYSize,latticeZSize;
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_int(file, "/Model/Diffusion", "latticeXSize", &latticeXSize));
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_int(file, "/Model/Diffusion", "latticeYSize", &latticeYSize));
        HDF5_EXCEPTION_CHECK(H5LTget_attribute_int(file, "/Model/Diffusion", "latticeZSize", &latticeZSize));

        // Read the initial lattice sites.
        HDF5_EXCEPTION_CHECK(H5LTget_dataset_ndims(file, "/Model/Diffusion/Gradient", &ndims));
        if (ndims != 3) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Diffusion/Gradient");
        HDF5_EXCEPTION_CHECK(H5LTget_dataset_info(file, "/Model/Diffusion/Gradient",dims, &type, &size));
        if (latticeXSize+2 != (int)dims[0] || latticeYSize+2 != (int)dims[1] || latticeZSize+2 != (int)dims[2] || size != sizeof(double)) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Diffusion/Gradient");
        int dataSize = dims[0]*dims[1]*dims[2];
        double* data=new double[dataSize];
        HDF5_EXCEPTION_CHECK(H5LTread_dataset(file, "/Model/Diffusion/Gradient", H5T_NATIVE_DOUBLE, data));
        for (int i=0; i<dataSize; i++)
            bc->add_boundary_gradient(data[i]);
        bc->set_boundary_gradient_ordering(lm::types::ROW_MAJOR);
        delete[] data;
    }
}

bool Hdf5File::replicateExists(uint64_t replicate)
{
    char replicateName[8];
    snprintf(replicateName, sizeof(replicateName), "%07d", (int)replicate);
    if (H5Lexists(simulationsGroup, replicateName, H5P_DEFAULT) > 0) return true;
    return false;
}

void Hdf5File::openReplicate(uint64_t replicate) throw(HDF5Exception)
{
    openReplicateHandles(replicate);
}

void Hdf5File::appendSpeciesCounts(uint64_t replicate, lm::io::SpeciesCounts * speciesCounts) throw(HDF5Exception)
{
    appendSpeciesTimeSeries(replicate, speciesCounts->number_entries(), speciesCounts->number_species(), speciesCounts->species_count().data(), speciesCounts->time().data());
}

int32_t* Hdf5File::dumpSpeciesCounts(const lm::io::SpeciesTimeSeries& speciesTimeSeries)
{
    int numberEntries = speciesTimeSeries.counts().shape(0);
    int numberSpecies = speciesTimeSeries.counts().shape(1);

    // Extract the data, decompressing if necessary.
    int32_t* counts=NULL;
    if (speciesTimeSeries.counts().compressed_deflate())
    {
        counts = new int32_t[numberEntries*numberSpecies];
        size_t size = numberEntries*numberSpecies*sizeof(counts[0]);
        size_t uncompressedSize = size;
        const std::string& str = speciesTimeSeries.counts().data();
        ZLIB_EXCEPTION_CHECK(uncompress((unsigned char *)counts, &uncompressedSize, (unsigned char*)&(str[0]), str.size()));
        if (uncompressedSize != size)
            throw Exception("Error during data decompression, wrong number of bytes returned.");
    }
    else
    {
        const std::string& str = speciesTimeSeries.counts().data();
        if (str.size() != numberEntries*numberSpecies*sizeof(counts[0]))
            InvalidArgException("speciesTimeSeries.counts.data", "Incorrect size for data array.");
        counts = (int32_t*)&(str[0]);
    }
    return counts;
}

double* Hdf5File::dumpSpeciesTimes(const lm::io::SpeciesTimeSeries& speciesTimeSeries)
{
    // Extract the data, decompressing if necessary.
    int numberEntries = speciesTimeSeries.counts().shape(0);

    double* times=NULL;
    if (speciesTimeSeries.times().compressed_deflate())
    {
        times = new double[numberEntries];
        size_t size = numberEntries*sizeof(times[0]);
        size_t uncompressedSize = size;
        const std::string& str = speciesTimeSeries.times().data();
        ZLIB_EXCEPTION_CHECK(uncompress((unsigned char *)times, &uncompressedSize, (unsigned char*)&(str[0]), str.size()));
        if (uncompressedSize != size)
            throw Exception("Error during data decompression, wrong number of bytes returned.");
    }
    else
    {
        const std::string& str = speciesTimeSeries.times().data();
        if (str.size() != numberEntries*sizeof(times[0]))
            InvalidArgException("speciesTimeSeries.times.data", "Incorrect size for data array.");
        times = (double*)&(str[0]);
    }
    return times;
}

void Hdf5File::appendSpeciesTimeSeries(uint64_t replicate, const lm::io::SpeciesTimeSeries& speciesTimeSeries)
{
    int numberEntries = speciesTimeSeries.counts().shape(0);
    int numberSpecies = speciesTimeSeries.counts().shape(1);

    if (speciesTimeSeries.times().shape(0) != numberEntries)
        InvalidArgException("speciesTimeSeries.times.shape", "Numebr of rows in time array incocnsistent with counts array.");

    // Extract the data, decompressing if necessary.
    int32_t* counts=dumpSpeciesCounts(speciesTimeSeries);
    double* times=dumpSpeciesTimes(speciesTimeSeries);

    // Append  the data.
    appendSpeciesTimeSeries(replicate, numberEntries, numberSpecies, counts, times);

    // Free any allocated memory.
    if (speciesTimeSeries.counts().compressed_deflate())
        delete[] counts;
    if (speciesTimeSeries.times().compressed_deflate())
        delete[] times;
}

void Hdf5File::appendSpeciesTimeSeries(uint64_t replicate, int numberEntries, int numberSpecies, const int32_t* counts, const double* times)
{
    ReplicateHandles * handles = openReplicateHandles(replicate);

    // Update the species counts dataset.
    {
        // Get the current size of the dataset.
        unsigned int RANK=2;
        hsize_t dims[RANK];
        hid_t dataspace_id;
        int result;
        HDF5_EXCEPTION_CALL(dataspace_id,H5Dget_space(handles->speciesCountsDataset));
        HDF5_EXCEPTION_CALL(result,H5Sget_simple_extent_dims(dataspace_id, dims, NULL));
        HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));

        // Extend the dataset by the number of rows in the data set.
        dims[0] += numberEntries;
        HDF5_EXCEPTION_CHECK(H5Dset_extent(handles->speciesCountsDataset, dims));

        // Create the memory dataset.
        hid_t memspace_id;
        hsize_t memDims[RANK];
        memDims[0] = numberEntries;
        memDims[1] = numberSpecies;
        HDF5_EXCEPTION_CALL(memspace_id,H5Screate_simple(RANK, memDims, NULL));

        // Write the new data.
        HDF5_EXCEPTION_CALL(dataspace_id,H5Dget_space(handles->speciesCountsDataset));
        hsize_t start[RANK], count[RANK];
        start[0] = dims[0]-numberEntries;
        start[1] = 0;
        count[0] = memDims[0];
        count[1] = memDims[1];
        HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, start, NULL, count, NULL));
        HDF5_EXCEPTION_CHECK(H5Dwrite(handles->speciesCountsDataset, H5T_NATIVE_INT32, memspace_id, dataspace_id, H5P_DEFAULT, counts));

        // Cleanup some resources.
        HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));
        HDF5_EXCEPTION_CHECK(H5Sclose(memspace_id));
    }

    // Update the species count times dataset.
    {
        // Get the current size of the dataset.
        unsigned int RANK=1;
        hsize_t dims[RANK];
        hid_t dataspace_id;
        int result;
        HDF5_EXCEPTION_CALL(dataspace_id,H5Dget_space(handles->speciesCountTimesDataset));
        HDF5_EXCEPTION_CALL(result,H5Sget_simple_extent_dims(dataspace_id, dims, NULL));
        HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));

        // Extend the dataset by the number of rows in the data set.
        dims[0] += numberEntries;
        HDF5_EXCEPTION_CHECK(H5Dset_extent(handles->speciesCountTimesDataset, dims));

        // Create the memory dataset.
        hid_t memspace_id;
        hsize_t memDims[RANK];
        memDims[0] = numberEntries;
        HDF5_EXCEPTION_CALL(memspace_id,H5Screate_simple(RANK, memDims, NULL));

        // Write the new data.
        HDF5_EXCEPTION_CALL(dataspace_id,H5Dget_space(handles->speciesCountTimesDataset));
        hsize_t start[RANK], count[RANK];
        start[0] = dims[0]-numberEntries;
        count[0] = memDims[0];
        HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, start, NULL, count, NULL));
        HDF5_EXCEPTION_CHECK(H5Dwrite(handles->speciesCountTimesDataset, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, times));

        // Cleanup some resources.
        HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));
        HDF5_EXCEPTION_CHECK(H5Sclose(memspace_id));
    }
}

void Hdf5File::appendLatticeTimeSeries(uint64_t replicate, const lm::io::LatticeTimeSeries& data)
{
    ReplicateHandles * replicateHandles = openReplicateHandles(replicate);

    // Open or create the lattice group.
    hid_t latticeGroupHandle, latticeTimesDatasetHandle;
    if (H5Lexists(replicateHandles->group, "Lattice", H5P_DEFAULT) > 0)
    {
        HDF5_EXCEPTION_CALL(latticeGroupHandle,H5Gopen2(replicateHandles->group, "Lattice", H5P_DEFAULT));
        HDF5_EXCEPTION_CALL(latticeTimesDatasetHandle,H5Dopen2(replicateHandles->group, "LatticeTimes", H5P_DEFAULT));
    }
    else
    {
        HDF5_EXCEPTION_CALL(latticeGroupHandle,H5Gcreate2(replicateHandles->group, "Lattice", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));

        // Create the lattice times dataset.
        const unsigned int RANK=1;
        hsize_t dims[RANK], maxDims[RANK], chunkDims[RANK];
        dims[0] = 0;
        maxDims[0] = H5S_UNLIMITED;
        chunkDims[0] = 100;
        hid_t dataspaceHandle, propsHandle;
        HDF5_EXCEPTION_CALL(dataspaceHandle,H5Screate_simple(RANK, dims, maxDims));
        HDF5_EXCEPTION_CALL(propsHandle,H5Pcreate(H5P_DATASET_CREATE));
        HDF5_EXCEPTION_CHECK(H5Pset_chunk(propsHandle, RANK, chunkDims));
        HDF5_EXCEPTION_CALL(latticeTimesDatasetHandle,H5Dcreate2(replicateHandles->group, "LatticeTimes", H5T_IEEE_F64LE, dataspaceHandle, H5P_DEFAULT, propsHandle, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5Pclose(propsHandle));
        HDF5_EXCEPTION_CHECK(H5Sclose(dataspaceHandle));
    }

    // Append the time to the times data set.
    for (int i=0; i<data.number_entries(); i++)
    {
        uint latticeIndex;
        {
            // Get the current size of the dataset.
            unsigned int RANK=1;
            hsize_t dims[RANK];
            hid_t dataspaceHandle;
            int result;
            HDF5_EXCEPTION_CALL(dataspaceHandle,H5Dget_space(latticeTimesDatasetHandle));
            HDF5_EXCEPTION_CALL(result,H5Sget_simple_extent_dims(dataspaceHandle, dims, NULL));
            HDF5_EXCEPTION_CHECK(H5Sclose(dataspaceHandle));

            // Extend the dataset by the number of rows in the data set.
            dims[0] += 1;
            HDF5_EXCEPTION_CHECK(H5Dset_extent(latticeTimesDatasetHandle, dims));

            // Create the memory dataset.
            hid_t memspaceHandle;
            hsize_t memDims[RANK];
            memDims[0] = 1;
            HDF5_EXCEPTION_CALL(memspaceHandle,H5Screate_simple(RANK, memDims, NULL));

            // Write the new data.
            HDF5_EXCEPTION_CALL(dataspaceHandle,H5Dget_space(latticeTimesDatasetHandle));
            hsize_t start[RANK], count[RANK];
            start[0] = dims[0]-1;
            latticeIndex=start[0];
            count[0] = memDims[0];
            HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(dataspaceHandle, H5S_SELECT_SET, start, NULL, count, NULL));
            double time = data.time(i);
            HDF5_EXCEPTION_CHECK(H5Dwrite(latticeTimesDatasetHandle, H5T_NATIVE_DOUBLE, memspaceHandle, dataspaceHandle, H5P_DEFAULT, &time));

            // Cleanup some resources.
            HDF5_EXCEPTION_CHECK(H5Sclose(dataspaceHandle));
            HDF5_EXCEPTION_CHECK(H5Sclose(memspaceHandle));
        }

        // Create the lattice data set.
        {
            const lm::types::Lattice& lattice = data.lattice(i);
            if (!lattice.has_particles_per_site()) throw Exception("Invalid lattice, particles per site must be specified for HDF5 file output.");
            if (!lattice.has_particles_ordering()) throw Exception("Invalid lattice, data ordering must be specified for HDF5 file output.");
            if (lattice.particles_ordering() != lm::types::ROW_MAJOR) throw Exception("Invalid lattice, data ordering must be in ROW_MAJOR format for HDF5 file output.");
            if (!lattice.has_particles()) throw Exception("Invalid lattice, data must be specified for HDF5 file output.");

            size_t latticeParticlesSize=0;
            unsigned char* latticeParticlesData=NULL;
            if (lattice.particles_compressed_deflate())
            {
                // Create a temporary buffer.
                latticeParticlesSize = lattice.lattice_x_size()*lattice.lattice_y_size()*lattice.lattice_z_size()*lattice.particles_per_site();
                latticeParticlesData = new unsigned char [latticeParticlesSize];

                // Uncompress the particle data into the temp buffer.
                const std::string& particles = lattice.particles();
                ZLIB_EXCEPTION_CHECK(uncompress(latticeParticlesData, &latticeParticlesSize, (unsigned char*)&(particles[0]), particles.size()));
                if (latticeParticlesSize != lattice.lattice_x_size()*lattice.lattice_y_size()*lattice.lattice_z_size()*lattice.particles_per_site())
                    throw Exception("Error during particle decompression, wrong number of bytes returned",latticeParticlesSize,lattice.lattice_x_size()*lattice.lattice_y_size()*lattice.lattice_z_size()*lattice.particles_per_site());
            }
            else
            {
                const std::string& particles = lattice.particles();
                latticeParticlesSize = particles.size();
                latticeParticlesData = (unsigned char*)&(particles[0]);
            }

            if (lattice.lattice_x_size()*lattice.lattice_y_size()*lattice.lattice_z_size()*lattice.particles_per_site() != latticeParticlesSize) throw Exception("Invalid lattice, lattice size and data size must agree for HDF5 file output.");

            char latticeDatasetName[11];
            snprintf(latticeDatasetName, sizeof(latticeDatasetName), "%010d", latticeIndex);
            const unsigned int RANK=4;
            hsize_t dims[RANK], chunk[RANK];
            dims[0] = lattice.lattice_x_size();
            dims[1] = lattice.lattice_y_size();
            dims[2] = lattice.lattice_z_size();
            dims[3] = lattice.particles_per_site();
            chunk[0] = min(TUNE_LATTICE_GZIP_CHUNK_SIZE,lattice.lattice_x_size());
            chunk[1] = min(TUNE_LATTICE_GZIP_CHUNK_SIZE,lattice.lattice_y_size());
            chunk[2] = min(TUNE_LATTICE_GZIP_CHUNK_SIZE,lattice.lattice_z_size());
            chunk[3] = lattice.particles_per_site();
            hid_t dataspaceHandle, dcplHandle, datasetHandle;
            HDF5_EXCEPTION_CALL(dataspaceHandle,H5Screate_simple(RANK, dims, NULL));
            HDF5_EXCEPTION_CALL(dcplHandle,H5Pcreate(H5P_DATASET_CREATE));
            HDF5_EXCEPTION_CHECK(H5Pset_deflate (dcplHandle, TUNE_LATTICE_GZIP_COMPRESSION_LEVEL));
            HDF5_EXCEPTION_CHECK(H5Pset_chunk(dcplHandle, RANK, chunk));
            HDF5_EXCEPTION_CALL(datasetHandle,H5Dcreate2(latticeGroupHandle, latticeDatasetName, H5T_STD_U8LE, dataspaceHandle, H5P_DEFAULT, dcplHandle, H5P_DEFAULT));
            HDF5_EXCEPTION_CHECK(H5Dwrite(datasetHandle, H5T_NATIVE_UINT8, H5S_ALL, H5S_ALL, H5P_DEFAULT, latticeParticlesData));
            HDF5_EXCEPTION_CHECK(H5Dclose(datasetHandle));
            HDF5_EXCEPTION_CHECK(H5Pclose(dcplHandle));
            HDF5_EXCEPTION_CHECK(H5Sclose(dataspaceHandle));

            // If this was compressed data, free the buffer.
            if (lattice.particles_compressed_deflate())
            {
                delete[] latticeParticlesData;
            }
        }
    }

    // Cleanup some resources.
    HDF5_EXCEPTION_CHECK(H5Dclose(latticeTimesDatasetHandle));
    HDF5_EXCEPTION_CHECK(H5Gclose(latticeGroupHandle));

}

void Hdf5File::appendParameterValues(uint64_t replicate, lm::io::ParameterValues * parameterValues) throw(HDF5Exception,InvalidArgException)
{
    if (parameterValues->value_size() != parameterValues->time_size()) throw InvalidArgException("parameterValues", "inconsistent number of entries");

    ReplicateHandles * replicateHandles = openReplicateHandles(replicate);

    // Open or create the parameter values group.
    hid_t pvGroupHandle;
    if ((pvGroupHandle=H5Gopen2(replicateHandles->group, "ParameterValues", H5P_DEFAULT)) < 0)
    {
        HDF5_EXCEPTION_CALL(pvGroupHandle,H5Gcreate2(replicateHandles->group, "ParameterValues", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    }

    // Open or create the parameter values dataset.
    hid_t pvDatasetHandle;
    if ((pvDatasetHandle=H5Dopen2(pvGroupHandle, parameterValues->parameter().c_str(), H5P_DEFAULT)) < 0)
    {
        // Create the datasets.
        uint RANK=2;
        hsize_t dims[RANK], maxDims[RANK], chunkDims[RANK];
        dims[0] = 0;
        dims[1] = 2;
        maxDims[0] = H5S_UNLIMITED;
        maxDims[1] = 2;
        chunkDims[0] = 100;
        chunkDims[1] = 2;
        hid_t dataspace_id, dcpl_id;
        HDF5_EXCEPTION_CALL(dataspace_id,H5Screate_simple(RANK, dims, maxDims));
        HDF5_EXCEPTION_CALL(dcpl_id,H5Pcreate(H5P_DATASET_CREATE));
        HDF5_EXCEPTION_CHECK(H5Pset_chunk(dcpl_id, RANK, chunkDims));
        HDF5_EXCEPTION_CALL(pvDatasetHandle,H5Dcreate2(pvGroupHandle, parameterValues->parameter().c_str(), H5T_IEEE_F64LE, dataspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5Pclose(dcpl_id));
        HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));
    }

    // Get the current size of the dataset.
    unsigned int RANK=2;
    hsize_t dims[RANK];
    hid_t dataspace_id;
    int result;
    HDF5_EXCEPTION_CALL(dataspace_id,H5Dget_space(pvDatasetHandle));
    HDF5_EXCEPTION_CALL(result,H5Sget_simple_extent_dims(dataspace_id, dims, NULL));
    HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));

    // Extend the dataset by the number of rows in the data set.
    dims[0] += parameterValues->value_size();
    HDF5_EXCEPTION_CHECK(H5Dset_extent(pvDatasetHandle, dims));

    // Create the memory dataset.
    hid_t memspace_id;
    hsize_t memDims[RANK];
    memDims[0] = parameterValues->value_size();
    memDims[1] = 1;
    HDF5_EXCEPTION_CALL(memspace_id,H5Screate_simple(RANK, memDims, NULL));

    // Write the new times.
    HDF5_EXCEPTION_CALL(dataspace_id,H5Dget_space(pvDatasetHandle));
    hsize_t start[RANK], count[RANK];
    start[0] = dims[0]-parameterValues->value_size();
    count[0] = parameterValues->value_size();
    start[1] = 0;
    count[1] = 1;
    HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, start, NULL, count, NULL));
    HDF5_EXCEPTION_CHECK(H5Dwrite(pvDatasetHandle, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, parameterValues->time().data()));
    start[1] = 1;
    count[1] = 1;
    HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, start, NULL, count, NULL));
    HDF5_EXCEPTION_CHECK(H5Dwrite(pvDatasetHandle, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, parameterValues->value().data()));

    // Cleanup some resources.
    HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));
    HDF5_EXCEPTION_CHECK(H5Sclose(memspace_id));
    HDF5_EXCEPTION_CHECK(H5Dclose(pvDatasetHandle));
    HDF5_EXCEPTION_CHECK(H5Gclose(pvGroupHandle));
}

void Hdf5File::setFirstPassageTimes(uint64_t replicate, lm::io::FirstPassageTimes * firstPassageTimes) throw(HDF5Exception,InvalidArgException)
{
    // Make sure the data is consistent.
    if (firstPassageTimes->species_count_size() == 0) throw InvalidArgException("firstPassageTimes", "no entries to save");
    if (firstPassageTimes->species_count_size() != firstPassageTimes->number_entries() || firstPassageTimes->first_passage_time_size() != firstPassageTimes->number_entries()) throw InvalidArgException("firstPassageTimes", "inconsistent number of first passage time entries");

    ReplicateHandles * replicateHandles = openReplicateHandles(replicate);

    // Open the first passage time group.
    hid_t fptGroupHandle;
    if ((fptGroupHandle=H5Gopen2(replicateHandles->group, "FirstPassageTimes", H5P_DEFAULT)) < 0)
    {
        HDF5_EXCEPTION_CALL(fptGroupHandle,H5Gcreate2(replicateHandles->group, "FirstPassageTimes", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    }

    // Construct a string representation of the species name.
    std::stringstream ss;
    ss.fill('0');
    ss.width(2);
    ss << firstPassageTimes->species();
    string speciesString = ss.str();

    // Open the species group.
    hid_t speciesGroupHandle, countsDatasetHandle, timesDatasetHandle;
    if ((speciesGroupHandle=H5Gopen2(fptGroupHandle, speciesString.c_str(), H5P_DEFAULT)) >= 0)
    {
        // Open the data sets.
        HDF5_EXCEPTION_CALL(countsDatasetHandle,H5Dopen2(speciesGroupHandle, "Counts", H5P_DEFAULT));
        HDF5_EXCEPTION_CALL(timesDatasetHandle,H5Dopen2(speciesGroupHandle, "Times", H5P_DEFAULT));
    }
    else
    {
        // Create the group.
        HDF5_EXCEPTION_CALL(speciesGroupHandle,H5Gcreate2(fptGroupHandle, speciesString.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));

        // Create the datasets.
        uint RANK=1;
        hsize_t dims[RANK], maxDims[RANK], chunkDims[RANK];
        dims[0] = 0;
        maxDims[0] = H5S_UNLIMITED;
        chunkDims[0] = 100;
        hid_t dataspace_id, dcpl_id;
        HDF5_EXCEPTION_CALL(dataspace_id,H5Screate_simple(RANK, dims, maxDims));
        HDF5_EXCEPTION_CALL(dcpl_id,H5Pcreate(H5P_DATASET_CREATE));
        HDF5_EXCEPTION_CHECK(H5Pset_chunk(dcpl_id, RANK, chunkDims));
        HDF5_EXCEPTION_CALL(countsDatasetHandle,H5Dcreate2(speciesGroupHandle, "Counts", H5T_STD_U32LE, dataspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT));
        HDF5_EXCEPTION_CALL(timesDatasetHandle,H5Dcreate2(speciesGroupHandle, "Times", H5T_IEEE_F64LE, dataspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5Pclose(dcpl_id));
        HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));
    }

    // Read the lowest and highest first passage times in the file.
    uint RANK=1;
    int result;
    hid_t countsDataspace;
    hsize_t countsDims[RANK];
    HDF5_EXCEPTION_CALL(countsDataspace,H5Dget_space(countsDatasetHandle));
    HDF5_EXCEPTION_CALL(result,H5Sget_simple_extent_dims(countsDataspace, countsDims, NULL));
    uint minCount=0, maxCount=0;
    if (countsDims[0] == 1)
    {
        HDF5_EXCEPTION_CHECK(H5Dread(countsDatasetHandle, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, &minCount));
        maxCount = minCount;
    }
    else if (countsDims[0] > 1)
    {
        // Create the memory dataspace.
        hid_t memDataspaceHandle;
        hsize_t memDims[RANK];
        memDims[0] = 1;
        HDF5_EXCEPTION_CALL(memDataspaceHandle,H5Screate_simple(RANK, memDims, NULL));

        hsize_t start[RANK], count[RANK];
        count[0] = 1;
        start[0] = 0;
        HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(countsDataspace, H5S_SELECT_SET, start, NULL, count, NULL));
        HDF5_EXCEPTION_CHECK(H5Dread(countsDatasetHandle, H5T_NATIVE_UINT32, memDataspaceHandle, countsDataspace, H5P_DEFAULT, &minCount));
        start[0] = countsDims[0]-1;
        HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(countsDataspace, H5S_SELECT_SET, start, NULL, count, NULL));
        HDF5_EXCEPTION_CHECK(H5Dread(countsDatasetHandle, H5T_NATIVE_UINT32, memDataspaceHandle, countsDataspace, H5P_DEFAULT, &maxCount));
        HDF5_EXCEPTION_CHECK(H5Sclose(memDataspaceHandle));
    }
    HDF5_EXCEPTION_CHECK(H5Sclose(countsDataspace));

    // Figure out the min and the max from the new counts.
    uint32_t minNewCount=firstPassageTimes->species_count(0), maxNewCount=firstPassageTimes->species_count(0);
    for (int i=1; i<firstPassageTimes->species_count_size(); i++)
    {
        if (firstPassageTimes->species_count(i) < (int)minNewCount)
            minNewCount = firstPassageTimes->species_count(i);
        else if (firstPassageTimes->species_count(i) > (int)maxNewCount)
            maxNewCount = firstPassageTimes->species_count(i);
    }

    // If there are no existing records, just insert one large block.
    if (countsDims[0] == 0)
    {
        // Allocate the block.
        countsDims[0] = maxNewCount-minNewCount+1;
        uint32_t * counts = new uint32_t[countsDims[0]];
        double * times = new double[countsDims[0]];
        for (uint i=0; i<countsDims[0]; i++)
        {
            counts[i] = 0;
            times[i] = -1.0;
        }

        // Fill in the block.
        for (int i=0; i<firstPassageTimes->species_count_size(); i++)
        {
            uint32_t count = firstPassageTimes->species_count(i);
            counts[count-minNewCount] = count;
            times[count-minNewCount] = firstPassageTimes->first_passage_time(i);
        }

        // Extend the datasets and write the block.
        HDF5_EXCEPTION_CHECK(H5Dset_extent(countsDatasetHandle, countsDims));
        HDF5_EXCEPTION_CHECK(H5Dwrite(countsDatasetHandle, H5T_NATIVE_UINT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, counts));
        HDF5_EXCEPTION_CHECK(H5Dset_extent(timesDatasetHandle, countsDims));
        HDF5_EXCEPTION_CHECK(H5Dwrite(timesDatasetHandle, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, times));

        // Free the buffers.
        delete [] counts;
        delete [] times;
    }

    // Otherwise if there are only new records to append to the end, just add them
    else if (maxNewCount > maxCount && minNewCount > maxCount)
    {
        // Allocate the new buffer.
        hsize_t endingStart[RANK], endingCount[RANK];
        endingCount[0] = maxNewCount-maxCount;
        uint32_t * newEndingCounts = new uint32_t[endingCount[0]];
        double * newEndingTimes = new double[endingCount[0]];
        for (uint i=0; i<endingCount[0]; i++)
        {
            newEndingCounts[i] = 0;
            newEndingTimes[i] = -1.0;
        }

        // Fill in the buffer.
        for (int i=0; i<firstPassageTimes->species_count_size(); i++)
        {
            uint32_t count = firstPassageTimes->species_count(i);
            newEndingCounts[count-maxCount-1] = count;
            newEndingTimes[count-maxCount-1] = firstPassageTimes->first_passage_time(i);
        }

        // Create the memory dataspace.
        hid_t memDataspaceHandle;
        hsize_t memDims[RANK];
        memDims[0] = endingCount[0];
        HDF5_EXCEPTION_CALL(memDataspaceHandle,H5Screate_simple(RANK, memDims, NULL));

        // Append the records.
        hid_t countsDataspaceHandle, timesDataspaceHandle;
        endingStart[0] = countsDims[0];
        countsDims[0] += endingCount[0];
        HDF5_EXCEPTION_CHECK(H5Dset_extent(countsDatasetHandle, countsDims));
        HDF5_EXCEPTION_CALL(countsDataspaceHandle,H5Dget_space(countsDatasetHandle));
        HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(countsDataspaceHandle, H5S_SELECT_SET, endingStart, NULL, endingCount, NULL));
        HDF5_EXCEPTION_CHECK(H5Dwrite(countsDatasetHandle, H5T_NATIVE_UINT32, memDataspaceHandle, countsDataspaceHandle, H5P_DEFAULT, newEndingCounts));
        HDF5_EXCEPTION_CHECK(H5Sclose(countsDataspaceHandle));
        HDF5_EXCEPTION_CHECK(H5Dset_extent(timesDatasetHandle, countsDims));
        HDF5_EXCEPTION_CALL(timesDataspaceHandle,H5Dget_space(timesDatasetHandle));
        HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(timesDataspaceHandle, H5S_SELECT_SET, endingStart, NULL, endingCount, NULL));
        HDF5_EXCEPTION_CHECK(H5Dwrite(timesDatasetHandle, H5T_NATIVE_DOUBLE, memDataspaceHandle, timesDataspaceHandle, H5P_DEFAULT, newEndingTimes));
        HDF5_EXCEPTION_CHECK(H5Sclose(timesDataspaceHandle));
        HDF5_EXCEPTION_CHECK(H5Sclose(memDataspaceHandle));

        // Free the buffers.
        if (newEndingCounts != NULL) delete [] newEndingCounts;
        if (newEndingTimes != NULL) delete [] newEndingTimes;
    }

    // Otherwise, we need to insert some records before and/or after the existing block so we have to copy the old data and rebuild.
    else
    {
        // Declare the new buffer.
        uint32_t combinedMinCount = min(minCount,minNewCount);
        uint32_t combinedMaxCount = max(maxCount,maxNewCount);
        size_t newSize = combinedMaxCount-combinedMinCount+1;
        uint32_t * newCounts = new uint32_t[newSize];
        double * newTimes = new double[newSize];
        for (uint i=0; i<newSize; i++)
        {
            newCounts[i] = 0;
            newTimes[i] = -1.0;
        }

        // Copy the old data into the buffer.
        hid_t memDataspaceHandle;
        hsize_t memDims[RANK];
        memDims[0] = maxCount-minCount+1;
        HDF5_EXCEPTION_CALL(memDataspaceHandle,H5Screate_simple(RANK, memDims, NULL));
        HDF5_EXCEPTION_CHECK(H5Dread(countsDatasetHandle, H5T_NATIVE_UINT32, memDataspaceHandle, H5S_ALL, H5P_DEFAULT, &newCounts[minCount-combinedMinCount]));
        HDF5_EXCEPTION_CHECK(H5Dread(timesDatasetHandle, H5T_NATIVE_DOUBLE, memDataspaceHandle, H5S_ALL, H5P_DEFAULT, &newTimes[minCount-combinedMinCount]));
        HDF5_EXCEPTION_CHECK(H5Sclose(memDataspaceHandle));

        // Fill in the buffer with the new data.
        for (int i=0; i<firstPassageTimes->species_count_size(); i++)
        {
            uint32_t count = firstPassageTimes->species_count(i);
            if (count < minCount || count > maxCount)
            {
                newCounts[count-combinedMinCount] = count;
                newTimes[count-combinedMinCount] = firstPassageTimes->first_passage_time(i);
            }
            else
            {
                throw InvalidArgException("firstPassageTimes", "contained duplicates of existing counts", count);
            }
        }

        // Update the dataset with the combined data.
        memDims[0] = newSize;
        countsDims[0] = newSize;
        HDF5_EXCEPTION_CALL(memDataspaceHandle,H5Screate_simple(RANK, memDims, NULL));
        HDF5_EXCEPTION_CHECK(H5Dset_extent(countsDatasetHandle, countsDims));
        HDF5_EXCEPTION_CHECK(H5Dwrite(countsDatasetHandle, H5T_NATIVE_UINT32, memDataspaceHandle, H5S_ALL, H5P_DEFAULT, newCounts));
        HDF5_EXCEPTION_CHECK(H5Dset_extent(timesDatasetHandle, countsDims));
        HDF5_EXCEPTION_CHECK(H5Dwrite(timesDatasetHandle, H5T_NATIVE_DOUBLE, memDataspaceHandle, H5S_ALL, H5P_DEFAULT, newTimes));
        HDF5_EXCEPTION_CHECK(H5Sclose(memDataspaceHandle));

        // Free the buffers.
        if (newCounts != NULL) delete [] newCounts;
        if (newTimes != NULL) delete [] newTimes;
    }

    // Close any resources.
    HDF5_EXCEPTION_CHECK(H5Dclose(countsDatasetHandle));
    HDF5_EXCEPTION_CHECK(H5Dclose(timesDatasetHandle));
    HDF5_EXCEPTION_CHECK(H5Gclose(speciesGroupHandle));
    HDF5_EXCEPTION_CHECK(H5Gclose(fptGroupHandle));
}

/*void SimulationFile::appendSpatialModelObjects(unsigned int replicate, lm::input::SpatialModel * model) throw(HDF5Exception,InvalidArgException)
{
    if (model->sphere_xc_size() != model->sphere_type_size()) throw InvalidArgException("model", "inconsistent number of sphere entries");
    if (model->sphere_yc_size() != model->sphere_type_size()) throw InvalidArgException("model", "inconsistent number of sphere entries");
    if (model->sphere_zc_size() != model->sphere_type_size()) throw InvalidArgException("model", "inconsistent number of sphere entries");
    if (model->sphere_radius_size() != model->sphere_type_size()) throw InvalidArgException("model", "inconsistent number of sphere entries");

    ReplicateHandles * replicateHandles = openReplicateHandles(replicate);

    // Open or create the group.
    hid_t modelHandle, spatialHandle;
    if ((modelHandle=H5Gopen2(replicateHandles->group, "Model", H5P_DEFAULT)) < 0)
    {
        HDF5_EXCEPTION_CALL(modelHandle,H5Gcreate2(replicateHandles->group, "Model", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    }
    if ((spatialHandle=H5Gopen2(modelHandle, "Spatial", H5P_DEFAULT)) < 0)
    {
        HDF5_EXCEPTION_CALL(spatialHandle,H5Gcreate2(modelHandle, "Spatial", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
    }

    // Open or create the datasets.
    hid_t spheresDatasetHandle;
    if ((spheresDatasetHandle=H5Dopen2(spatialHandle, "Spheres", H5P_DEFAULT)) < 0)
    {
        // Create the datasets.
        uint RANK=2;
        hsize_t dims[RANK], maxDims[RANK], chunkDims[RANK];
        dims[0] = 0;
        dims[1] = 5;
        maxDims[0] = H5S_UNLIMITED;
        maxDims[1] = 5;
        chunkDims[0] = 100;
        chunkDims[1] = 5;
        hid_t dataspace_id, dcpl_id;
        HDF5_EXCEPTION_CALL(dataspace_id,H5Screate_simple(RANK, dims, maxDims));
        HDF5_EXCEPTION_CALL(dcpl_id,H5Pcreate(H5P_DATASET_CREATE));
        HDF5_EXCEPTION_CHECK(H5Pset_chunk(dcpl_id, RANK, chunkDims));
        HDF5_EXCEPTION_CALL(spheresDatasetHandle,H5Dcreate2(spatialHandle, "Spheres", H5T_IEEE_F64LE, dataspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5Pclose(dcpl_id));
        HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));
    }

    // Get the current size of the dataset.
    unsigned int RANK=2;
    hsize_t dims[RANK];
    hid_t dataspace_id;
    int result;
    HDF5_EXCEPTION_CALL(dataspace_id,H5Dget_space(spheresDatasetHandle));
    HDF5_EXCEPTION_CALL(result,H5Sget_simple_extent_dims(dataspace_id, dims, NULL));
    HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));

    // Extend the dataset by the number of rows in the data set.
    dims[0] += model->sphere_type_size();
    HDF5_EXCEPTION_CHECK(H5Dset_extent(spheresDatasetHandle, dims));

    // Create the memory dataset.
    hid_t memspace_id;
    hsize_t memDims[RANK];
    memDims[0] = model->sphere_type_size();
    memDims[1] = 1;
    HDF5_EXCEPTION_CALL(memspace_id,H5Screate_simple(RANK, memDims, NULL));

    // Write the values.
    HDF5_EXCEPTION_CALL(dataspace_id,H5Dget_space(spheresDatasetHandle));
    hsize_t start[RANK], count[RANK];
    start[0] = dims[0]-model->sphere_type_size();
    count[0] = model->sphere_type_size();
    start[1] = 0;
    count[1] = 1;
    HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, start, NULL, count, NULL));
    HDF5_EXCEPTION_CHECK(H5Dwrite(spheresDatasetHandle, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, model->sphere_xc().data()));
    start[1] = 1;
    HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, start, NULL, count, NULL));
    HDF5_EXCEPTION_CHECK(H5Dwrite(spheresDatasetHandle, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, model->sphere_yc().data()));
    start[1] = 2;
    HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, start, NULL, count, NULL));
    HDF5_EXCEPTION_CHECK(H5Dwrite(spheresDatasetHandle, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, model->sphere_zc().data()));
    start[1] = 3;
    HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, start, NULL, count, NULL));
    HDF5_EXCEPTION_CHECK(H5Dwrite(spheresDatasetHandle, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, model->sphere_radius().data()));
    start[1] = 4;
    HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, start, NULL, count, NULL));
    HDF5_EXCEPTION_CHECK(H5Dwrite(spheresDatasetHandle, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, model->sphere_type().data()));

    // Cleanup some resources.
    HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));
    HDF5_EXCEPTION_CHECK(H5Sclose(memspace_id));
    HDF5_EXCEPTION_CHECK(H5Dclose(spheresDatasetHandle));
    HDF5_EXCEPTION_CHECK(H5Gclose(modelHandle));
    HDF5_EXCEPTION_CHECK(H5Gclose(spatialHandle));
}

void SimulationFile::getSpatialModelObjects(unsigned int replicate, lm::input::SpatialModel * model) throw(HDF5Exception)
{
    ReplicateHandles * replicateHandles = openReplicateHandles(replicate);

    // Open the group.
    hid_t modelHandle, spatialHandle;
    HDF5_EXCEPTION_CALL(modelHandle,H5Gopen2(replicateHandles->group, "Model", H5P_DEFAULT));
    HDF5_EXCEPTION_CALL(spatialHandle,H5Gopen2(modelHandle, "Spatial", H5P_DEFAULT));

    // Open the datasets.
    hid_t spheresDatasetHandle;
    HDF5_EXCEPTION_CALL(spheresDatasetHandle,H5Dopen2(spatialHandle, "Spheres", H5P_DEFAULT));

    // Get the current size of the dataset.
    const uint RANK=2;
    const uint NUM_COLS=5;
    hsize_t dims[RANK];
    hid_t dataspace_id;
    int result;
    HDF5_EXCEPTION_CALL(dataspace_id,H5Dget_space(spheresDatasetHandle));
    HDF5_EXCEPTION_CALL(result,H5Sget_simple_extent_dims(dataspace_id, dims, NULL));

    // Create the memory dataset.
    hid_t memspace_id;
    hsize_t memDims[RANK];
    memDims[0] = TUNE_SPATIAL_MODEL_OJBECT_READ_BUFFER_SIZE;
    memDims[1] = NUM_COLS;
    HDF5_EXCEPTION_CALL(memspace_id,H5Screate_simple(RANK, memDims, NULL));
    double * memData = new double[TUNE_SPATIAL_MODEL_OJBECT_READ_BUFFER_SIZE*NUM_COLS];

    // Loop over the data and read it in one section at a time.
    hsize_t start[RANK], count[RANK];
    start[0] = 0;
    start[1] = 0;
    count[0] = TUNE_SPATIAL_MODEL_OJBECT_READ_BUFFER_SIZE;
    count[1] = NUM_COLS;
    while (start[0] < dims[0])
    {
        // Figure out how many rows to read.
        if (start[0]+count[0] > dims[0])
        {
            // This is the last read, so modify its size.
            count[0] = dims[0]-start[0];

            // Modify the memory dataspace as well.
            HDF5_EXCEPTION_CHECK(H5Sclose(memspace_id));
            memDims[0] = count[0];
            HDF5_EXCEPTION_CALL(memspace_id,H5Screate_simple(RANK, memDims, NULL));
        }

        // Read the rows.
        HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, start, NULL, count, NULL));
        HDF5_EXCEPTION_CHECK(H5Dread(spheresDatasetHandle, H5T_NATIVE_DOUBLE, memspace_id, dataspace_id, H5P_DEFAULT, (void *)memData));

        // Add them to the model.
        for (uint index=0; index<count[0]*NUM_COLS; )
        {
            model->add_sphere_xc(memData[index++]);
            model->add_sphere_yc(memData[index++]);
            model->add_sphere_zc(memData[index++]);
            model->add_sphere_radius(memData[index++]);
            model->add_sphere_type(memData[index++]);
        }

        // Move to the next section.
        start[0] += count[0];
    }

    // Cleanup some resources.
    if (memData != NULL) delete []  memData; memData = NULL;
    HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));
    HDF5_EXCEPTION_CHECK(H5Sclose(memspace_id));
    HDF5_EXCEPTION_CHECK(H5Dclose(spheresDatasetHandle));
    HDF5_EXCEPTION_CHECK(H5Gclose(modelHandle));
    HDF5_EXCEPTION_CHECK(H5Gclose(spatialHandle));
}*/


vector<double> Hdf5File::getLatticeTimes(uint64_t replicate) throw(HDF5Exception,InvalidArgException)
{
    ReplicateHandles * replicateHandles = openReplicateHandles(replicate);

    // Read the initial species counts.
    int RANK=1, rank;
    hsize_t dims[RANK];
    H5T_class_t type;
    size_t size;
    HDF5_EXCEPTION_CHECK(H5LTget_dataset_ndims(replicateHandles->group, "LatticeTimes", &rank));
    if (rank != RANK) throw Exception("Invalid dataset rank", filename.c_str(), "LatticeTimes");
    HDF5_EXCEPTION_CHECK(H5LTget_dataset_info(replicateHandles->group, "LatticeTimes", dims, &type, &size));
    if (size != sizeof(double)) throw Exception("Invalid dataset type", filename.c_str(), "LatticeTimes");
    double * timesBuffer = new double[dims[0]];
    HDF5_EXCEPTION_CHECK(H5LTread_dataset_double(replicateHandles->group, "LatticeTimes", timesBuffer));
    vector<double> times;
    for (uint i=0; i<dims[0]; i++)
        times.push_back(timesBuffer[i]);
    delete [] timesBuffer;
    return times;
}

void Hdf5File::getLattice(uint64_t replicate, unsigned int latticeIndex, lm::rdme::Lattice * lattice) throw(HDF5Exception,InvalidArgException)
{
    ReplicateHandles * replicateHandles = openReplicateHandles(replicate);

    // Read the lattice data.
    char latticeDatasetName[19];
    snprintf(latticeDatasetName, sizeof(latticeDatasetName), "Lattice/%010d", latticeIndex);
    int RANK=4, rank;
    hsize_t dims[RANK];
    H5T_class_t type;
    size_t size;
    HDF5_EXCEPTION_CHECK(H5LTget_dataset_ndims(replicateHandles->group, latticeDatasetName, &rank));
    if (rank != RANK) throw Exception("Invalid dataset rank", filename.c_str(), latticeDatasetName);
    HDF5_EXCEPTION_CHECK(H5LTget_dataset_info(replicateHandles->group, latticeDatasetName, dims, &type, &size));
    if (size != sizeof(uint8_t)) throw Exception("Invalid dataset type", filename.c_str(), latticeDatasetName);
    if (lattice->getSize().x != dims[0] || lattice->getSize().y != dims[1] || lattice->getSize().z != dims[2] || lattice->getMaxOccupancy() != dims[3]) throw Exception("Invalid lattice dimensions", filename.c_str());
    size_t particlesBufferSize = dims[0]*dims[1]*dims[2]*dims[3];
    uint8_t * particlesBuffer = new uint8_t[particlesBufferSize];
    HDF5_EXCEPTION_CHECK(H5LTread_dataset(replicateHandles->group, latticeDatasetName, H5T_STD_U8LE, particlesBuffer));

    // Set the lattice object using the data.
    //TODO lattice->setFromRowMajorByteData(particlesBuffer, particlesBufferSize);

    // Free the intermediate lattice data.
    delete [] particlesBuffer;
}

void Hdf5File::closeReplicate(uint64_t replicate) throw(HDF5Exception)
{
    ReplicateHandleMap::Key replicateKey(recordNamePrefix, replicate);
    ReplicateHandleMap::iterator it = openReplicates.find(replicateKey);
    if (it != openReplicates.end())
    {
        ReplicateHandles * handles = it->second;
        closeReplicateHandles(handles);
        delete handles;
        openReplicates.erase(it);
    }
}

void Hdf5File::closeAllReplicates() throw(HDF5Exception)
{
    for (ReplicateHandleMap::iterator it=openReplicates.begin(); it != openReplicates.end(); it++)
    {
        ReplicateHandles * handles = it->second;
        closeReplicateHandles(handles);
        delete handles;
    }
    openReplicates.clear();
}


Hdf5File::ReplicateHandles* Hdf5File::openReplicateHandles(uint64_t replicate) throw(HDF5Exception)
{
    // See if the replicate is already open.
//    map<uint64_t,ReplicateHandles *>::iterator it = openReplicates.find(replicate);
    ReplicateHandleMap::Key replicateKey(recordNamePrefix, replicate);
    ReplicateHandleMap::iterator it = openReplicates.find(replicateKey);
    if (it == openReplicates.end())
    {
        ReplicateHandles * handles;

        // Construct a string representation of the replicate name.
        std::stringstream ss;
        ss.fill('0');
        ss.width(7);
        ss << replicate;
        string replicateString = ss.str();

        // Turn off exception handling again to work around odd behavior in hdf5 1.8.4.
        HDF5_EXCEPTION_CHECK(H5Eset_auto2(H5E_DEFAULT, NULL, NULL));

        // Open the replicate group.
        hid_t groupHandle;
        if ((groupHandle=H5Gopen2(simulationsGroup, replicateString.c_str(), H5P_DEFAULT)) >= 0)
        {
            // Open the rest of the handles.
            handles = new ReplicateHandles();
            handles->group = groupHandle;
            HDF5_EXCEPTION_CALL(handles->speciesCountsDataset,H5Dopen2(handles->group, "SpeciesCounts", H5P_DEFAULT));
            HDF5_EXCEPTION_CALL(handles->speciesCountTimesDataset,H5Dopen2(handles->group, "SpeciesCountTimes", H5P_DEFAULT));
        }
        else
        {
            // If we couldn't open it, try to create a new one.
            handles = createReplicateHandles(replicateString);
        }

        // Add it to the map.
        openReplicates[replicateKey] = handles;
        return handles;
    }
    else
    {
        // Use the already open replicate.
        return it->second;
    }

}

Hdf5File::ReplicateHandles* Hdf5File::createReplicateHandles(string replicateString) throw(Exception,HDF5Exception)
{
    // Make sure the model is loaded, since need the number of species.
    loadModel();

    ReplicateHandles * handles = new ReplicateHandles();

    // Create the group.
    HDF5_EXCEPTION_CALL(handles->group,H5Gcreate2(simulationsGroup, replicateString.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));

    // Create the species counts dataset
    {
        unsigned int RANK=2;
        hsize_t dims[RANK], maxDims[RANK], chunkDims[RANK];
        dims[0] = 0;
        dims[1] = numberSpecies;
        maxDims[0] = H5S_UNLIMITED;
        maxDims[1] = numberSpecies;
        chunkDims[0] = 100;
        chunkDims[1] = numberSpecies;
        hid_t dataspace_id, dcpl_id;
        HDF5_EXCEPTION_CALL(dataspace_id,H5Screate_simple(RANK, dims, maxDims));
        HDF5_EXCEPTION_CALL(dcpl_id,H5Pcreate(H5P_DATASET_CREATE));
        HDF5_EXCEPTION_CHECK(H5Pset_chunk(dcpl_id, RANK, chunkDims));
        HDF5_EXCEPTION_CALL(handles->speciesCountsDataset,H5Dcreate2(handles->group, "SpeciesCounts", H5T_STD_I32LE, dataspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5Pclose(dcpl_id));
        HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));
    }

    // Create the species count times dataset
    {
        unsigned int RANK=1;
        hsize_t dims[RANK], maxDims[RANK], chunkDims[RANK];
        dims[0] = 0;
        maxDims[0] = H5S_UNLIMITED;
        chunkDims[0] = 100;
        hid_t dataspace_id, dcpl_id;
        HDF5_EXCEPTION_CALL(dataspace_id,H5Screate_simple(RANK, dims, maxDims));
        HDF5_EXCEPTION_CALL(dcpl_id,H5Pcreate(H5P_DATASET_CREATE));
        HDF5_EXCEPTION_CHECK(H5Pset_chunk(dcpl_id, RANK, chunkDims));
        HDF5_EXCEPTION_CALL(handles->speciesCountTimesDataset,H5Dcreate2(handles->group, "SpeciesCountTimes", H5T_IEEE_F64LE, dataspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT));
        HDF5_EXCEPTION_CHECK(H5Pclose(dcpl_id));
        HDF5_EXCEPTION_CHECK(H5Sclose(dataspace_id));
    }

    return handles;
}

void Hdf5File::closeReplicateHandles(ReplicateHandles * handles) throw(HDF5Exception)
{
    HDF5_EXCEPTION_CHECK(H5Gclose(handles->group));
    HDF5_EXCEPTION_CHECK(H5Dclose(handles->speciesCountsDataset));
    HDF5_EXCEPTION_CHECK(H5Dclose(handles->speciesCountTimesDataset));
    handles->group = H5I_INVALID_HID;
    handles->speciesCountsDataset = H5I_INVALID_HID;
    handles->speciesCountTimesDataset = H5I_INVALID_HID;
}

void Hdf5File::setRecordNamePrefix(const string& newRecordNamePrefix)
{
    if (newRecordNamePrefix!=recordNamePrefix)
    {
        recordNamePrefix.assign(newRecordNamePrefix);

        simulationsGroup = initGroup(pathJoin(recordNamePrefix, "Simulations"));
    }
}

hsize_t Hdf5File::setDatasetFromNDArray(const std::string& groupPath, const std::string& datasetName, const robertslab::pbuf::NDArray& ndarrayRef, hid_t rootGroup)
{
    // a descriptor that we'll pass to the lower level output function
    DatasetDescriptor datasetDescriptor(groupPath, datasetName, ndarrayRef, rootGroup);

    // now that we have the data and the shape, call the generalized dataset writing function
    hsize_t rows = setDataset(datasetDescriptor);

    // return the number of rows written out
    return rows;
}

void Hdf5File::setDatasetFromNDArrayReplicate(uint64_t replicate, const std::string& groupRelativePath, const std::string& datasetName, const robertslab::pbuf::NDArray& ndarray, bool condensed)
{
    if (condensed)
    {
        setDatasetFromNDArrayReplicateCondensed(replicate, groupRelativePath, datasetName, ndarray);
    }
    else
    {
        ReplicateHandles * replicateHandles = openReplicateHandles(replicate);
        setDatasetFromNDArray(groupRelativePath, datasetName, ndarray, replicateHandles->group);
    }
}

// condensed version of the generalized NDArray hdf5 output. Condensed in the sense that it shoves all of the data into as few separate groups and datasets as possible
void Hdf5File::setDatasetFromNDArrayReplicateCondensed(uint64_t replicate, const std::string& groupRelativePath, const std::string& datasetName, const robertslab::pbuf::NDArray& ndarray)
{
    // write out the dataset directly to prefix/Simulations/groupRelativePath and get the number of rows written
    hsize_t rows = setDatasetFromNDArray(groupRelativePath, datasetName, ndarray, simulationsGroup);

    // create a 1D array containing one repition of the trajectoryID for each row in ndarray
    std::vector<uint64_t> trajectoryIDs(rows, replicate);

    // write out the trajectoryID dataset we just created
    std::string trajectoryIDDatasetName = datasetName + "_-_TrajectoryIDs";
    setDatasetFromContainer(groupRelativePath, trajectoryIDDatasetName, trajectoryIDs, simulationsGroup);
}

hsize_t Hdf5File::setDataset(const DatasetDescriptor& dd)
{
    // initialize the group we'll be storing the NDArray dataset in
    hid_t group = initGroup(dd.groupPath, dd.rootGroup);

    // declare the HDF5 boilerplate variables
    uint RANK(dd.shape.len);
    hid_t dataspace, dataset, filespace, memspace, prop;
    hsize_t chunkdims[RANK], dims[RANK], dimsr[RANK], dimstotal[RANK], maxdims[RANK], offset[RANK];

    // We'd like to have the option to delete NDArray's dataset if it already exists, but HDF5 apparently can't really delete anything so leave it commented for now.
    /* if (H5Lexists(group, groupName.c_str(), H5P_DEFAULT))
    {
        HDF5_EXCEPTION_CHECK(H5Ldelete(group, groupName.c_str(), H5P_DEFAULT));
    } */

    // write or extend the NDArray dataset
    dims[0] = RANK > 0 ? dd.shape[0] : 0;
    chunkdims[0] = 10;
    maxdims[0] = H5S_UNLIMITED;
    for (int i=1; i<RANK; i++)
    {
        dims[i] = dd.shape[i];
        chunkdims[i] = dims[i];
        maxdims[i] = dims[i];
    }

    // if the dataset exists, extend it
    if ((dataset = H5Dopen2(group, dd.datasetName.c_str(), H5P_DEFAULT))>=0)
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
        HDF5_EXCEPTION_CHECK(H5Dwrite(dataset, dd.hdf5Type, memspace, filespace, H5P_DEFAULT, dd.data));

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
        dataset = H5Dcreate2(group, dd.datasetName.c_str(), dd.hdf5Type, dataspace, H5P_DEFAULT, prop, H5P_DEFAULT);
        /* Write dataField to dataset */
        HDF5_EXCEPTION_CHECK(H5Dwrite(dataset, dd.hdf5Type, H5S_ALL, H5S_ALL, H5P_DEFAULT, dd.data));

        HDF5_EXCEPTION_CHECK(H5Dclose(dataset));
        HDF5_EXCEPTION_CHECK(H5Pclose(prop));
        HDF5_EXCEPTION_CHECK(H5Sclose(dataspace));
    }

    return dims[0];
}

}
}
}

/* Generic code to read a dataset with a hyperslab.
 * int ndims;
hsize_t dims[4];
hsize_t memdims[1];
hsize_t start[4], count[4];
hid_t dataset, dataspace, memspace;
HDF5_EXCEPTION_CALL(dataset,H5Dopen2(file, "/Model/Diffusion/Lattice", H5P_DEFAULT));
HDF5_EXCEPTION_CALL(dataspace,H5Dget_space(dataset));
ndims=H5Sget_simple_extent_ndims(dataspace);
if (ndims != 4) throw Exception("Invalid dataset dimensions", filename.c_str(), "/Model/Diffusion/Lattice");
HDF5_EXCEPTION_CALL(ndims,H5Sget_simple_extent_dims(dataspace, dims, NULL));
if (dims[0]*dims[1]*dims[2]*dims[3] != latticeSize) throw InvalidArgException("lattice", "incorrect lattice size");
start[0] = 0;
start[1] = 0;
start[2] = 0;
start[3] = 0;
count[0] = dims[0];
count[1] = dims[1];
count[2] = dims[2];
count[3] = dims[3];
HDF5_EXCEPTION_CHECK(H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, start, NULL, count, NULL));
memdims[0] = dims[0]*dims[1]*dims[2]*dims[3];
HDF5_EXCEPTION_CALL(memspace,H5Screate_simple(1, memdims, NULL));
HDF5_EXCEPTION_CHECK(H5Dread(dataset, H5T_NATIVE_UINT8, memspace, dataspace, H5P_DEFAULT, (void *)lattice));
HDF5_EXCEPTION_CHECK(H5Sclose(memspace));
HDF5_EXCEPTION_CHECK(H5Sclose(dataspace));
HDF5_EXCEPTION_CHECK(H5Dclose(dataset));
*/
