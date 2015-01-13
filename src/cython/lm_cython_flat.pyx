from libcpp.string cimport string
from libc.stdint cimport uint32_t  #, int64_t

cimport lm_cython_flat

cdef class Hdf5File:
    cdef CppHdf5File* thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self, string fname):
        self.thisptr = new CppHdf5File(fname)
    def __dealloc__(self):
        del self.thisptr
    def close(self):
        self.close()
    def getReactionModel(self, ReactionModel reactionModel):
        self.thisptr.getReactionModel(reactionModel.thisptr)
    def setReactionModel(self, ReactionModel reactionModel):
        self.thisptr.setReactionModel(reactionModel.thisptr)
        
cdef class ReactionModel:
    cdef CppReactionModel* thisptr
    def __cinit__(self):
        self.thisptr = new CppReactionModel()
    def __dealloc__(self):
        del self.thisptr
    def number_species(self):
        return self.thisptr.number_species()
    def set_number_species(self, numSpec):
        self.thisptr.set_number_species(numSpec)
        
# test compiled module in python with this line (assuming that foo.lm is a valid lm input file in the same dir):
#import lm_cython_flat; reactionModel = lm_cython_flat.ReactionModel(); hdf5File = lm_cython_flat.Hdf5File('foo.lm'); hdf5File.getReactionModel(reactionModel); reactionModel.number_species()