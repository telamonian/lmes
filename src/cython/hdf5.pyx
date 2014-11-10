from libcpp.string cimport string
from libc.stdint cimport uint32_t  #, int64_t

cdef extern from "lm/io/ReactionModel.pb.h" namespace "lm::io":
    cdef cppclass ReactionModel:
        ReactionModel()
        uint32_t number_species()
        void set_number_species(uint32_t)

cdef class PyReactionModel:
    cdef ReactionModel *thisptr
    def __cinit__(self):
        self.thisptr = new ReactionModel()
    def __dealloc__(self):
        del self.thisptr
    def number_species(self):
        return self.thisptr.number_species()
    cpdef set_number_species(self, uint32_t numSpec):
        self.thisptr.set_number_species(numSpec)

cdef extern from "lm/io/hdf5/SimulationFile.h" namespace "lm::io::hdf5":
    cdef cppclass Hdf5File:
        Hdf5File(string)
        void close()
        void getReactionModel(ReactionModel*)
        void setReactionModel(ReactionModel*)

cdef class PyHdf5File:
    cdef Hdf5File *thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self, string fname):
        self.thisptr = new Hdf5File(fname)
    def __dealloc__(self):
        del self.thisptr
    def close(self):
        self.close()
    cdef getReactionModel(self, ReactionModel* reactionModel):
        self.getReactionModel(reactionModel)
    cdef setReactionModel(self, ReactionModel* reactionModel):
        self.setReactionModel(reactionModel)