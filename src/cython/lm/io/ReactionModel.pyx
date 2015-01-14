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
        