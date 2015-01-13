from libcpp.string cimport string
from libc.stdint cimport uint32_t  #, int64_t

cimport ReactionModel

cdef class ReactionModel:
#     cdef CppReactionModel* thisptr
    def __cinit__(self):
        self.thisptr = new CppReactionModel()
    def __dealloc__(self):
        del self.thisptr
    def number_species(self):
        return self.thisptr.number_species()
    def set_number_species(self, uint32_t numSpec):
        self.thisptr.set_number_species(numSpec)
        
# test compiled module in python with this line (assuming that foo.lm is a valid lm input file in the same dir):
#import hdf5; reactionModel = hdf5.ReactionModel(); hdf5File = hdf5.Hdf5File('foo.lm'); hdf5File.getReactionModel(reactionModel); reactionModel.number_species()