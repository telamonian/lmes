%module lk
%include "typemaps.i" 
%{ 
#define SWIG_FILE_WITH_INIT
#include "Types.h" 
#include "Exceptions.h" 
#include "Lattice.h" 
#include "CUDALattice.h"
#include "LKRunner.h"
#include "CUDALKRunner.h"
#include "LatticeReader.h"
#include "LatticeWriter.h"
#include "builder/LatticeBuilder.h"
#include "builder/CellBuilder.h"
#include "builder/SpiroplasmaBuilder.h"
#include "ParticleTracker.h"  
%}

%inline %{
typedef unsigned int 		uint;
typedef unsigned char 		byte;
typedef char 				int8;
typedef unsigned char 		uint8;
typedef short 				int16;
typedef unsigned short 		uint16;
typedef int				 	int32;
typedef unsigned int	 	uint32;
typedef long long 			int64;
typedef unsigned long long 	uint64;
typedef long				intv;
typedef unsigned long	 	uintv;
typedef uint32 				lattice_size_t;
typedef uint32				lattice_site_t;
typedef uint32				lattice_particle_t;
typedef uint64 				nstime_t;
typedef uint32				nmdist_t;
%}

%typemap(out) lattice_coord_t {
	$result = PyTuple_New(3);
	PyTuple_SetItem($result, 0, PyInt_FromLong($1.x));
	PyTuple_SetItem($result, 1, PyInt_FromLong($1.y));
	PyTuple_SetItem($result, 2, PyInt_FromLong($1.z));
}

%typemap(out) std::vector<nstime_t> {
	$result = PyList_New($1.size());
	int index=0;
	for (std::vector<nstime_t>::const_iterator it=$1.begin(); it != $1.end(); it++, index++)
	{
		nstime_t value = *it;	
		PyList_SetItem($result, index, (value>LONG_MAX)?PyLong_FromUnsignedLongLong(value):PyInt_FromLong(value));
	}
}

/*
 * Type for returning particles from CUDALattice::findAllParticles.
 */
%typemap(out) std::map<lattice_particle_t,std::vector<lattice_coord_t> > {

	$result = PyDict_New();
	for (std::map<lattice_particle_t,std::vector<lattice_coord_t> >::iterator it=$1.begin(); it != $1.end(); it++)
	{
		lattice_particle_t particle = (*it).first;
		std::vector<lattice_coord_t> coordList = (*it).second;
	
		PyObject* resultList = PyList_New(coordList.size());
		int index=0;
		for (std::vector<lattice_coord_t>::const_iterator it2=coordList.begin(); it2 != coordList.end(); it2++, index++)
		{
			lattice_coord_t value = *it2;	
			PyObject* resultTuple = PyTuple_New(3);
			PyTuple_SetItem(resultTuple, 0, PyInt_FromLong(value.x));
			PyTuple_SetItem(resultTuple, 1, PyInt_FromLong(value.y));
			PyTuple_SetItem(resultTuple, 2, PyInt_FromLong(value.z));
			PyList_SetItem(resultList, index, resultTuple);
		}
		
		PyDict_SetItem($result, PyInt_FromLong(particle), resultList);
	}
}

%typemap(in) std::list<lattice_particle_t> {
	if (PyList_Check($input))
	{
		for (int i=0; i<PyList_Size($input); i++)
		{
			PyObject *o = PyList_GetItem($input, i);
			if (PyInt_Check(o))
			{
				$1.push_back((lattice_particle_t)PyInt_AsLong(o));
			}
			else
			{
				PyErr_SetString(PyExc_TypeError, "must be a list of particle types");
				return NULL;
			}
		}
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "must be a list");
		return NULL;
	}	
}
%typemap(typecheck) std::list<lattice_particle_t> {
   $1 = PyList_Check($input) ? 1 : 0;
}

%typemap(out) std::map<lattice_particle_t,uintv> {
	$result = PyDict_New();
	int index=0;
	for (std::map<lattice_particle_t,uintv>::const_iterator it=$1.begin(); it != $1.end(); it++, index++)
	{
		PyDict_SetItem($result, PyInt_FromLong((*it).first), PyInt_FromLong((*it).second));
	}
}

/*
 * Type for passing boundary concentration in to CUDALKRunner::setBoundaryConditions.
 */
%typemap(in) std::map<lattice_particle_t,double> boundaryConcentrations {
	// test 2
	if (PyDict_Check($input))
	{
		PyObject* keys = PyDict_Keys($input);
		PyObject* values = PyDict_Values($input);
		for (int i=0; i<PyDict_Size($input); i++)
		{
			PyObject *k = PyList_GetItem(keys, i);
			PyObject *v = PyList_GetItem(values, i);
			if (PyInt_Check(k) && PyFloat_Check(v))
			{
				$1[(lattice_particle_t)PyInt_AsLong(k)] = PyFloat_AsDouble(v);
			}
			else
			{
				PyErr_SetString(PyExc_TypeError, "must be a map of particle types to concentrations");
				return NULL;
			}
		}
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "must be a map");
		return NULL;
	}	
}
%typemap(typecheck) std::map<lattice_particle_t,double> boundaryConcentrations {
   $1 = PyDict_Check($input) ? 1 : 0;
}


namespace lk
{
class Lattice
{
public:
	virtual unsigned int getXSize() const;
	virtual unsigned int getYSize() const;
	virtual unsigned int getZSize() const;
	virtual unsigned int getNumberSites() const;
	virtual unsigned int getSpacing() const;
	virtual unsigned int getSite(unsigned int x, unsigned int y, unsigned int z);
	virtual void setSite(unsigned int x, unsigned int y, unsigned int z, unsigned int site);
	virtual int getMaxParticlesPerSite() const =0;
	virtual int getOccupancy(unsigned int x, unsigned int y, unsigned int z) const =0;
	virtual int getParticle(unsigned int x, unsigned int y, unsigned int z, int particleIndex) const =0;
	virtual void addParticle(unsigned int x, unsigned int y, unsigned int z, unsigned int particle) =0;
	virtual void removeAllParticles(unsigned int x, unsigned int y, unsigned int z) =0;
	virtual bool findParticle(unsigned int particle, unsigned int* OUTPUT, unsigned int* OUTPUT, unsigned int* OUTPUT, unsigned int* OUTPUT);
	virtual bool findNextParticle(unsigned int particle, unsigned int* INOUT, unsigned int* INOUT, unsigned int* INOUT, unsigned int* INOUT);
	virtual bool findNearbyParticle(unsigned int particle, unsigned int x, unsigned int y, unsigned int z, unsigned int* OUTPUT, unsigned int* OUTPUT, unsigned int* OUTPUT, unsigned int* OUTPUT);
	virtual std::map<lattice_particle_t,uintv> getParticleCounts();
};

class CUDALattice : public Lattice
{
public:
	CUDALattice(lattice_size_t xSize, lattice_size_t ySize, lattice_size_t zSize, nmdist_t latticeSpacing=1, uint maxParticlesPerSite=4);
	virtual ~CUDALattice();
	virtual int getMaxParticlesPerSite() const;
	virtual int getOccupancy(unsigned int x, unsigned int y, unsigned int z) const;
	virtual unsigned int getParticle(unsigned int x, unsigned int y, unsigned int z, int particleIndex) const;
	virtual void addParticle(unsigned int x, unsigned int y, unsigned int z, unsigned int particle);
	virtual void removeAllParticles(unsigned int x, unsigned int y, unsigned int z);
	virtual const std::map<lattice_particle_t,std::vector<lattice_coord_t> > findAllParticles(lattice_particle_t particleType, lattice_particle_t particleMask=LATTICE_PARTICLE_MAX);
};

class LatticeBuilder
{
public:
        LatticeBuilder(Lattice &_lattice, int randomSeed=1);
        LatticeBuilder(Lattice &_lattice, int cellX, int cellY, int cellZ, int randomSeed=1);
        virtual ~LatticeBuilder();
        void BuildMembrane(int membrane, int type, bool toTree);
        //void BuildMembraneFromFile(const char* filename, int type, int toTree);
        float BuildMembraneFromFile(const char* filename, int type, int toTree, const char *ribofilename, float sphere_radius, int type2, int toTree2, int inside_type, int membrane_type, int outside_type);
        void addSpheres(float percent_volume, float sphere_radius, int type, bool pbc, bool toTree, int nm_between_shell_and_membrane, int nm_between_shell_and_endcaps);
        void addSpheres(int num, float sphere_radius, int type, bool pbc, bool toTree, int nm_between_shell_and_membrane, int nm_between_shell_and_endcaps);
        void addSpheres(const char *filename, float sphere_radius, int type, bool pbc, bool toTree);
        void addSpheresInsideType(float percent_volume, float sphere_radius, int type, bool pbc, bool toTree, int nm_between_shell_and_membrane, int nm_between_shell_and_endcaps, int inside_type);
        void addRibosomes(int num, int type, bool pbc, bool toTree);
        void addRibosomes(float percent_volume, int type, bool pbc, bool toTree);
        void AddDNA(int num, int type, int dna_radius, int persistence_length, int angle, int toTree, int nm_between_shell_and_membrane, int nm_between_shell_and_endcaps);
        void fillWithParticleConcentration(float concentration, unsigned int particle, unsigned int siteType);
        void fillWithParticleNumber(unsigned int numberParticles, unsigned int particle, unsigned int siteType);
        int AddPoint(float x, float y, float z, int type, int toTree);
        void buildProbabilisticLattice(uint inside_type, uint membrane_type, uint outside_type, uint obstacle_type, float cutoff);
        void LoadCryoData(const char *filename);
        void TestFunction();
};

class CellBuilder : public LatticeBuilder
{
public:
        CellBuilder(Lattice &_lattice, int cellX, int cellY, int cellZ, int randomSeed=1);
        virtual ~CellBuilder();
        void BuildMembrane(int membrane, int type, int insideType, bool toTree);
        //void BuildMembraneFromFile(const char* filename, int type, int toTree);
        float BuildMembraneFromFile(const char* filename, int type, int toTree, const char *ribofilename, float sphere_radius, int type2, int toTree2, int inside_type, int membrane_type, int outside_type);
        void BuildMembraneSites(int membrane, int type1, int type2, int type3, int toTree);
        void addSpheres(float percent_volume, float sphere_radius, int type, bool pbc, bool toTree, int nm_between_shell_and_membrane, int nm_between_shell_and_endcaps);
        void addSpheres(int num, float sphere_radius, int type, bool pbc, bool toTree, int nm_between_shell_and_membrane, int nm_between_shell_and_endcaps);
        void addSpheres(const char *filename, float sphere_radius, int type, bool pbc, bool toTree);
        void addRibosomes(int num, int type, bool pbc, bool toTree);
        void addRibosomes(float percent_volume, int type, bool pbc, bool toTree);
        void AddDNA(int num, int type, int dna_radius, int persistence_length, int angle, int toTree, int nm_between_shell_and_membrane, int nm_between_shell_and_endcaps);
        void TestFunction();
        void AddInnerShell(int shell_type, int space_between_shell_and_membrane, int space_between_shell_and_endcaps, int toTree);
        void buildProbabilisticLattice(uint inside_type, uint membrane_type, uint outside_type, uint obstacle_type, float cutoff);
};


class SpiroplasmaBuilder
{
public:
        SpiroplasmaBuilder(Lattice &_lattice, int cellX, int cellY, int cellZ, float curve_radius, float curve_twist, float start_angle);
        virtual ~SpiroplasmaBuilder();
        void BuildMembrane(int membrane, int type, bool toTree);
        void addSpheres(float percent_volume, float sphere_radius, int type, bool pbc, bool toTree, int nm_between_shell_and_membrane, int nm_between_shell_and_endcaps);
        void addSpheres(int num, float sphere_radius, int type, bool pbc, bool toTree, int nm_between_shell_and_membrane, int nm_between_shell_and_endcaps);
        void addRibosomes(int num, int type, bool pbc, bool toTree);
        void addRibosomes(float percent_volume, int type, bool pbc, bool toTree);
        void AddDNA(int num, int type, int dna_radius, int persistence_length, int angle, int toTree, int nm_between_shell_and_membrane, int nm_between_shell_and_endcaps);
        void LoadCryoData(const char *filename);
        void BuildMembraneCryo(int membrane, int type, bool toTree);
        void TestFunction();
        void buildProbabilisticLattice(uint inside_type, uint membrane_type, uint outside_type, uint obstacle_type, float cutoff);
};
 

class LatticeReader
{
public:
	LatticeReader(const char* filename);
	virtual ~LatticeReader();
	virtual lattice_coord_t getLatticeSize() const;
	virtual nmdist_t getLatticeSpacing();
	virtual uint getMaxParticlesPerSite() const;
	virtual lattice_particle_t getMaxParticleType() const;
	virtual lattice_site_t getMaxSiteType() const;
	virtual uint64 getNumberFrames() const;	
	virtual const std::vector<nstime_t> getFrameTimes() const;
	virtual void loadFrame(uint64 frameIndex, Lattice* lattice, nstime_t* OUTPUT) const;
	virtual uint64 getNumberLatticeConfigurations() const;
	virtual const std::vector<nstime_t> getLatticeConfigurationTimes() const;
	virtual void loadLatticeConfiguration(uint64 latticeIndex, Lattice* lattice, nstime_t* OUTPUT) const;
	virtual void close();
};

class LatticeWriter
{
public:
	LatticeWriter(const char* filename, Lattice* lattice);
	virtual ~LatticeWriter();
	virtual void setFrameDataMethod(int frameDataMethod);
	virtual void setMaxParticleType(unsigned int maxParticleType);
	virtual void appendFrame(nstime_t time);
	virtual void appendLatticeConfiguration(nstime_t time);
	virtual void addTableOfContents();
	virtual void close();
};

class LKRunner
{
public:
	enum boundary_t {PERIODIC, ABSORBING, REFLECTING, CONSTANT_CONCENTRATION};
	virtual void run(nstime_t time)=0;
	virtual nstime_t getTime()=0;
	virtual boundary_t getBoundaryConditions() const;
	virtual void setBoundaryConditions(boundary_t boundaryConditions);
	virtual void setBoundaryConditions(boundary_t boundaryConditions, std::map<lattice_particle_t,double> boundaryConcentrations);
	virtual void setDiffusionCoefficient(lattice_site_t siteType, double D, lattice_particle_t particleType, lattice_particle_t particleMask=LATTICE_PARTICLE_MAX)=0;
	virtual void setSiteTransitionRate(lattice_site_t sourceSiteType, lattice_site_t destSiteType, double R, lattice_particle_t particleType, lattice_particle_t particleMask=LATTICE_PARTICLE_MAX)=0;
};

class CUDALKRunner : public LKRunner
{
public:
	CUDALKRunner(LatticeReader* reader, nstime_t timestepLength=1, int randomSeed=0);
	CUDALKRunner(CUDALattice* lattice, nstime_t initialTime=0, nstime_t timestepLength=1, int randomSeed=0);
	virtual ~CUDALKRunner();
	virtual void run(nstime_t time);
	virtual nstime_t getTime();
	virtual CUDALattice* getLattice();
	virtual void setDiffusionCoefficient(lattice_site_t siteType, double D, lattice_particle_t particleType, lattice_particle_t particleMask=LATTICE_PARTICLE_MAX);
	virtual void setSiteTransitionRate(lattice_site_t sourceSiteType, lattice_site_t destSiteType, double R, lattice_particle_t particleType, lattice_particle_t particleMask=LATTICE_PARTICLE_MAX);
	virtual void setFirstOrderReactionRate(lattice_site_t sourceSiteType, double K, lattice_particle_t reactant, std::list<lattice_particle_t> products, lattice_particle_t reactantMask=LATTICE_PARTICLE_MAX);
	virtual void setSecondOrderReactionRate(lattice_site_t sourceSiteType, double K, lattice_particle_t reactant1, lattice_particle_t reactant2, std::list<lattice_particle_t> products, lattice_particle_t reactant1Mask=LATTICE_PARTICLE_MAX, lattice_particle_t reactant2Mask=LATTICE_PARTICLE_MAX);	
};

class ParticleTracker
{
public:
	ParticleTracker(CUDALattice* lattice, lattice_particle_t particleType, lattice_particle_t particleMask=LATTICE_PARTICLE_MAX);
	virtual ~ParticleTracker();
	virtual void update();
	virtual uint getNumberParticleTypes();
	virtual lattice_particle_t getParticleType(uint index);
	virtual uint getNumberParticlePositions(lattice_particle_t particleType);
	virtual lattice_coord_t getParticlePosition(lattice_particle_t particleType, uint index);
};

}
