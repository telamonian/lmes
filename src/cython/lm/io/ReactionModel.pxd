cdef extern from "lm/io/ReactionModel.pb.h" namespace "lm::io":
    cdef cppclass CppReactionModel "lm::io::ReactionModel":
        CppReactionModel()
        uint32_t number_species()
        void set_number_species(uint32_t)
        