from libcpp.string cimport string
from libc.stdint cimport uint32_t  #, int64_t

cdef extern from "lm/io/hdf5/SimulationFile.h" namespace "lm::io::hdf5":
    cdef cppclass CppHdf5File "lm::io::hdf5::Hdf5File":
        CppHdf5File(string)
        void close()
        void getReactionModel(CppReactionModel*)
        void setReactionModel(CppReactionModel*)
        
cdef extern from "lm/io/ReactionModel.pb.h" namespace "lm::io":
    cdef cppclass CppReactionModel "lm::io::ReactionModel":
        CppReactionModel()
        uint32_t number_species()
        void set_number_species(uint32_t)