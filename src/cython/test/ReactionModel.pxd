from libcpp.string cimport string
from libc.stdint cimport uint32_t  #, int64_t

cdef extern from "lm/io/ReactionModel.pb.h" namespace "lm::io":
    cdef cppclass CppReactionModel "lm::io::ReactionModel":
        CppReactionModel()
        uint32_t number_species()
        void set_number_species(uint32_t)

cdef class ReactionModel:
    cdef CppReactionModel* thisptr
    void __cinit__(self)
    void __dealloc__(self)
    void number_species(self)
    void set_number_species(self, uint32_t numSpec)