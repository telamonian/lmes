from libcpp.string cimport string
from libc.stdint cimport uint32_t  #, int64_t

cimport ReactionModel

cdef extern from "lm/io/hdf5/SimulationFile.h" namespace "lm::io::hdf5":
    cdef cppclass CppHdf5File "lm::io::hdf5::Hdf5File":
        CppHdf5File(string)
        void close()
        void getReactionModel(ReactionModel.CppReactionModel*)
        void setReactionModel(ReactionModel.CppReactionModel*)