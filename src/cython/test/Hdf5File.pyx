from libcpp.string cimport string
from libc.stdint cimport uint32_t  #, int64_t

cimport Hdf5File
cimport ReactionModel

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
        
# test compiled module in python with this line (assuming that foo.lm is a valid lm input file in the same dir):
#import hdf5; reactionModel = hdf5.ReactionModel(); hdf5File = hdf5.Hdf5File('foo.lm'); hdf5File.getReactionModel(reactionModel); reactionModel.number_species()