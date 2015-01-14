from libcpp.string cimport string
from libc.stdint cimport uint32_t  #, int64_t

cimport _ReactionModel

cdef extern from "lm/io/hdf5/SimulationFile.h" namespace "lm::io::hdf5":
    cdef cppclass Hdf5File:
        Hdf5File(string)
        void close()
        void getReactionModel(_ReactionModel.ReactionModel*)
        void setReactionModel(_ReactionModel.ReactionModel*)