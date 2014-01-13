%module lm
%include "typemaps.i" 
%{ 
#define SWIG_FILE_WITH_INIT
#include <vector>
#include "lm/Types.h" 
#include "lm/Exceptions.h"
#include "lm/builder/Capsule.h"
#include "lm/builder/CapsuleShell.h"
#include "lm/builder/Cuboid.h"
#include "lm/builder/Hemisphere.h"
#include "lm/builder/Shape.h"
#include "lm/builder/Sphere.h" 
#include "lm/builder/LatticeBuilder.h"
#include "lm/builder/Shape.h"
#include "lm/builder/Sphere.h"
#include "DiffusionModel.pb.h"
#include "SpatialModel.pb.h"
#include "lm/io/hdf5/SimulationFile.h"
#include "lm/rdme/ByteLattice.h"
#include "lm/rdme/Lattice.h"

using lm::Exception;
using lm::IOException;
using lm::InvalidArgException;
using lm::io::hdf5::HDF5Exception;
using lm::io::SpatialModel;
using lm::rdme::InvalidSiteException;
using lm::rdme::InvalidParticleException;
%}

%inline %{
typedef unsigned char       uchar;
typedef unsigned int        uint;
typedef unsigned long       ulong;
typedef long                intv_t;
typedef unsigned long       uintv_t;
typedef unsigned char       uint8_t;
typedef unsigned int        uint32_t;
typedef uint8_t             byte;
typedef double              si_dist_t;
typedef double              si_time_t;
typedef uint32_t            lattice_size_t;
typedef uint32_t            site_size_t;
typedef uint32_t            site_t;
typedef uint32_t            particle_t;
%}


/*
 * Typemap for returning a list of floats.
 */
%typemap(out) std::vector<double> {

    $result = PyList_New($1.size());
    int index=0;
    for (std::vector<double>::const_iterator it=$1.begin(); it != $1.end(); it++, index++)
    {
        PyList_SetItem($result, index, PyFloat_FromDouble(*it));
    }
}

/*
 * Typemap for returning a list of particle location structures.
 */
%typemap(out) std::vector<particle_loc_t> {

    $result = PyList_New($1.size());
    int index=0;
    for (std::vector<particle_loc_t>::const_iterator it=$1.begin(); it != $1.end(); it++, index++)
    {
        PyObject* resultTuple = PyTuple_New(5);
        PyTuple_SetItem(resultTuple, 0, PyInt_FromLong((*it).p));
        PyTuple_SetItem(resultTuple, 1, PyInt_FromLong((*it).x));
        PyTuple_SetItem(resultTuple, 2, PyInt_FromLong((*it).y));
        PyTuple_SetItem(resultTuple, 3, PyInt_FromLong((*it).z));
        PyTuple_SetItem(resultTuple, 4, PyInt_FromLong((*it).index));
        PyList_SetItem($result, index, resultTuple);
    }
}

/*
 * Type for returning a lattice coordinate.
 */
%typemap(out) lattice_coord_t {
	$result = PyTuple_New(3);
	PyTuple_SetItem($result, 0, PyInt_FromLong($1.x));
	PyTuple_SetItem($result, 1, PyInt_FromLong($1.y));
	PyTuple_SetItem($result, 2, PyInt_FromLong($1.z));
}

/*
 * Type for returning particles from CUDALattice::findAllParticles.
 */
%typemap(out) std::map<particle_t,std::vector<lattice_coord_t> > {

	$result = PyDict_New();
	for (std::map<particle_t,std::vector<lattice_coord_t> >::iterator it=$1.begin(); it != $1.end(); it++)
	{
		particle_t particle = (*it).first;
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

/*
 * Type for passing in a list of particle types.
 */
%typemap(in) std::list<particle_t> {
	if (PyList_Check($input))
	{
		for (int i=0; i<PyList_Size($input); i++)
		{
			PyObject *o = PyList_GetItem($input, i);
			if (PyInt_Check(o))
			{
				$1.push_back((particle_t)PyInt_AsLong(o));
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

%typemap(typecheck) std::list<particle_t> {
   $1 = PyList_Check($input) ? 1 : 0;
}

/*
 * Type for returning a list of particle counts.
 */
%typemap(out) std::map<particle_t,uintv_t> {
	$result = PyDict_New();
	int index=0;
	for (std::map<particle_t,uintv_t>::const_iterator it=$1.begin(); it != $1.end(); it++, index++)
	{
		PyDict_SetItem($result, PyInt_FromLong((*it).first), PyInt_FromLong((*it).second));
	}
}

%typemap(throws) Exception %{
  PyErr_SetString(PyExc_RuntimeError, $1.what());
  SWIG_fail;
%}

%typemap(throws) IOException %{
  PyErr_SetString(PyExc_RuntimeError, $1.what());
  SWIG_fail;
%}

%typemap(throws) InvalidArgException %{
  PyErr_SetString(PyExc_RuntimeError, $1.what());
  SWIG_fail;
%}

%typemap(throws) HDF5Exception %{
  PyErr_SetString(PyExc_RuntimeError, $1.what());
  SWIG_fail;
%}

%typemap(throws) InvalidSiteException %{
  PyErr_SetString(PyExc_RuntimeError, $1.what());
  SWIG_fail;
%}

%typemap(throws) InvalidParticleException %{
  PyErr_SetString(PyExc_RuntimeError, $1.what());
  SWIG_fail;
%}

namespace lm
{

namespace builder
{

struct point {
    point(si_dist_t x=0.0, si_dist_t y=0.0, si_dist_t z=0.0):x(x),y(y),z(z){}
    si_dist_t x;
    si_dist_t y;
    si_dist_t z;

    si_dist_t distanceSquared(const point & p2)
    {
        si_dist_t dx = p2.x - x;
        si_dist_t dy = p2.y - y;
        si_dist_t dz = p2.z - z;
        return (dx*dx + dy*dy + dz*dz);
    }

    si_dist_t distance(const point & p2) {return sqrt(distanceSquared(p2));}
};

struct bounding_box {
    bounding_box(si_dist_t x1=0.0, si_dist_t y1=0.0, si_dist_t z1=0.0, si_dist_t x2=0.0, si_dist_t y2=0.0, si_dist_t z2=0.0):min(x1,y1,z1),max(x2,y2,z2){}
    bounding_box(point min, point max):min(min),max(max){}
    point min, max;

    bounding_box joinWith(bounding_box j)
    {
        return bounding_box(::min(j.min.x,min.x),::min(j.min.y,min.y),::min(j.min.z,min.z),::max(j.max.x,max.x),::max(j.max.y,max.y),::max(j.max.z,max.z));
    }
};

struct vector {
    vector(si_dist_t x=0.0, si_dist_t y=0.0, si_dist_t z=0.0):x(x),y(y),z(z){}
    si_dist_t x;
    si_dist_t y;
    si_dist_t z;
};

class Shape
{
public:
    virtual bool boundingBoxesIntersect(Shape * query);
    virtual bool intersects(Shape * query) = 0;
    virtual bool contains(Shape * query) = 0;
    virtual bounding_box getBoundingBox();
    virtual site_t getType();
    virtual double getVolume();
};

class Sphere : public Shape
{
public:
    Sphere(point center, si_dist_t radius, site_t type);
    virtual ~Sphere();
    virtual bool intersects(Shape * query);
    virtual bool contains(Shape * query);
    virtual void setCenter(point center);
    virtual point getCenter();
    virtual si_dist_t getRadius();
};

class Hemisphere : public Shape
{
public:
    Hemisphere(point center, si_dist_t radius, vector orientation, site_t type);
    virtual ~Hemisphere();
    virtual bool intersects(Shape * query);
    virtual bool contains(Shape * query);
    virtual point getCenter();
    virtual si_dist_t getRadius();
    virtual vector getOrientation();
};

class Capsule : public Shape
{
public:
    Capsule(point p1, point p2, si_dist_t radius, site_t type);
    virtual ~Capsule();
    virtual bool intersects(Shape * query);
    virtual bool contains(Shape * query);
    virtual point getP1();
    virtual point getP2();
    virtual si_dist_t getRadius();
};

class CapsuleShell : public Shape
{
public:
    CapsuleShell(point p1, point p2, si_dist_t innerRadius, si_dist_t outerRadius, site_t type);
    virtual ~CapsuleShell();
    virtual bool intersects(Shape * query);
    virtual bool contains(Shape * query);
    virtual point getP1();
    virtual point getP2();
    virtual si_dist_t getInnerRadius();
    virtual si_dist_t getOuterRadius();
};

class Cuboid : public Shape
{
public:
    Cuboid(point p1, point p2, site_t type);
    virtual ~Cuboid();
    virtual bool intersects(Shape * query);
    virtual bool contains(Shape * query);
    virtual point getP1();
    virtual point getP2();
};


class LatticeBuilder
{

public:
    LatticeBuilder(si_dist_t xLen, si_dist_t yLen, si_dist_t zLen, si_dist_t collisionGridSpacing, uint32_t seedTop, uint32_t seedBottom);
    virtual ~LatticeBuilder();
    virtual void addRegion(Shape * shape);
    //virtual bool placeObject(Shape * shape);
    virtual bool placeSphere(point position, si_dist_t radius, site_t type);
    virtual void removeSphere(point position, si_dist_t radius, site_t type);
    virtual uint placeRandomSphere(si_dist_t radius, site_t type, site_t region);
    virtual void placeRandomSpheres(uint count, si_dist_t radius, site_t type, site_t region);
    virtual void fillWithRandomSpheres(double volumeFraction, si_dist_t radius, site_t type, site_t region);
    virtual void getSpatialModel(lm::io::SpatialModel * model);
    virtual void addParticles(particle_t particleType, site_t siteType, uint count);
    virtual void discretizeTo(lm::rdme::Lattice * lattice, site_t obstacleSiteType, double fractionObstacleSitesOccupied);
};


}

namespace io {

class DiffusionModel
{
public:
    DiffusionModel();
    uint32_t number_species();
    uint32_t number_site_types();
    double lattice_spacing();
    uint32_t lattice_x_size();
    uint32_t lattice_y_size();
    uint32_t lattice_z_size();
    uint32_t particles_per_site();
    
    void set_lattice_spacing(double);
    void set_lattice_x_size(uint32_t);
    void set_lattice_y_size(uint32_t);
    void set_lattice_z_size(uint32_t);
    void set_particles_per_site(uint32_t);
};

class SpatialModel
{
public:
    SpatialModel();
};

namespace hdf5 {

class SimulationFile
{
public:
    static bool isValidFile(const char * filename) throw(IOException,HDF5Exception);
    static void create(const char * filename) throw(IOException,HDF5Exception);
    
public:
    SimulationFile(const char* filename);
    virtual ~SimulationFile();
    virtual void close();
    virtual void getDiffusionModel(DiffusionModel * model);
    virtual void setDiffusionModel(lm::io::DiffusionModel * diffusionModel) throw(InvalidArgException,HDF5Exception,Exception);
    virtual void setDiffusionModelLattice(lm::io::DiffusionModel * m, lm::rdme::Lattice * lattice) throw(InvalidArgException,HDF5Exception,Exception);
    virtual void getSpatialModel(SpatialModel * model);
    virtual void setSpatialModel(SpatialModel * model);
    virtual std::vector<double> getLatticeTimes(unsigned int replicate) throw(HDF5Exception,InvalidArgException);
    virtual void getLattice(unsigned int replicate, unsigned int latticeIndex, lm::rdme::Lattice * lattice) throw(HDF5Exception,InvalidArgException);
};

}
}

namespace rdme
{

class Lattice
{
public:
    virtual site_size_t getMaxOccupancy() const;
    virtual lattice_coord_t getSize() const;
    virtual lattice_size_t getXSize() const;
    virtual lattice_size_t getYSize() const;
    virtual lattice_size_t getZSize() const;
    virtual lattice_size_t getNumberSites() const;
    virtual si_dist_t getSpacing() const;
    
    virtual site_t getSiteType(lattice_size_t x, lattice_size_t y, lattice_size_t z)=0;
    virtual site_size_t getOccupancy(lattice_size_t x, lattice_size_t y, lattice_size_t z) const throw(InvalidSiteException)=0;
    virtual particle_t getParticle(lattice_size_t x, lattice_size_t y, lattice_size_t z, site_size_t particleIndex) const throw(InvalidSiteException,InvalidParticleException)=0;
    virtual std::vector<particle_loc_t> findParticles(particle_t minParticleType, particle_t maxParticleType)=0;
    
    virtual void addParticle(lattice_size_t x, lattice_size_t y, lattice_size_t z, particle_t particle) throw(InvalidSiteException,InvalidParticleException)=0;
};

class ByteLattice : public Lattice
{
public:
    ByteLattice(lattice_coord_t size, si_dist_t spacing, uint particlesPerSite) throw(std::bad_alloc,InvalidArgException,Exception);
    ByteLattice(lattice_size_t xSize, lattice_size_t ySize, lattice_size_t zSize, si_dist_t spacing, uint particlesPerSite) throw(std::bad_alloc,InvalidArgException,Exception);
    virtual ~ByteLattice() throw(std::bad_alloc);
    virtual site_t getSiteType(lattice_size_t x, lattice_size_t y, lattice_size_t z);
    virtual site_size_t getOccupancy(lattice_size_t x, lattice_size_t y, lattice_size_t z) const throw(InvalidSiteException);
    virtual particle_t getParticle(lattice_size_t x, lattice_size_t y, lattice_size_t z, site_size_t particleIndex) const throw(InvalidSiteException,InvalidParticleException);
    virtual std::vector<particle_loc_t> findParticles(particle_t minParticleType, particle_t maxParticleType);
    
    virtual void addParticle(lattice_size_t x, lattice_size_t y, lattice_size_t z, particle_t particle) throw(InvalidSiteException,InvalidParticleException);
};

}

}
