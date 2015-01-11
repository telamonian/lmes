from libcpp.string cimport string
from libc.stdint cimport uint32_t  #, int64_t

cimport hdf5_lib

cdef class ReactionModel:
    cdef hdf5_lib.ReactionModel* thisptr
    def __cinit__(self):
        self.thisptr = new hdf5_lib.ReactionModel()
    def __dealloc__(self):
        del self.thisptr
    def number_species(self):
        return self.thisptr.number_species()
    cpdef set_number_species(self, uint32_t numSpec):
        self.thisptr.set_number_species(numSpec)

cdef class Hdf5File:
    cdef hdf5_lib.Hdf5File* thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self, string fname):
        self.thisptr = new hdf5_lib.Hdf5File(fname)
    def __dealloc__(self):
        del self.thisptr
    def close(self):
        self.close()
    def getReactionModel(self, ReactionModel reactionModel):
        self.thisptr.getReactionModel(reactionModel.thisptr)
    def setReactionModel(self, ReactionModel reactionModel):
        self.thisptr.setReactionModel(reactionModel.thisptr)
        
# test compiled module in python with this line (assuming that foo.lm is a valid lm input file in the same dir):
#import hdf5; reactionModel = hdf5.ReactionModel(); hdf5File = hdf5.Hdf5File('foo.lm'); hdf5File.getReactionModel(reactionModel); reactionModel.number_species()