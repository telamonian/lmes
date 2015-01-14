from libcpp.string cimport string
from libc.stdint cimport uint32_t  #, int64_t

cimport _ReactionModel

cdef class ReactionModel:
    cdef _ReactionModel.ReactionModel* thisptr