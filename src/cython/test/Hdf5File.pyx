from libcpp.string cimport string
from libc.stdint cimport uint32_t  #, int64_t

cimport _Hdf5File
cimport Hdf5File
cimport ReactionModel

cdef class Hdf5File:
    def __cinit__(self, string fname):
        self.thisptr = new _Hdf5File.Hdf5File(fname)
    def __dealloc__(self):
        del self.thisptr
    def close(self):
        self.close()
    def getReactionModel(self, ReactionModel.ReactionModel reactionModel):
        self.thisptr.getReactionModel(reactionModel.thisptr)
    def setReactionModel(self, ReactionModel.ReactionModel reactionModel):
        self.thisptr.setReactionModel(reactionModel.thisptr)
        
# test compiled module in python with this line (assuming that foo.lm is a valid lm input file in the same dir):
#import Hdf5File; import ReactionModel; reactionModel = ReactionModel.ReactionModel(); hdf5File = Hdf5File.Hdf5File('foo.lm'); hdf5File.getReactionModel(reactionModel); reactionModel.number_species()