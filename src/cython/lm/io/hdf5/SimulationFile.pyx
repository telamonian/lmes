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
        