from libcpp.string cimport string
from libc.stdint cimport uint32_t  #, int64_t

cdef extern from "lm/io/ReactionModel.pb.h" namespace "lm::io":
    cdef cppclass ReactionModel:
        ReactionModel()
        uint32_t number_species()
        void set_number_species(uint32_t)