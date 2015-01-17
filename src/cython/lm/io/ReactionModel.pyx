cdef class ReactionModel:
    cdef CppReactionModel* thisptr
    def __cinit__(self):
        self.thisptr = new CppReactionModel()
    def __dealloc__(self):
        del self.thisptr
    def has_number_species(self):
        return self.thisptr.has_number_species()
    def clear_number_species(self):
        self.thisptr.clear_number_species()
    def number_species(self):
        return self.thisptr.number_species()
    def set_number_species(self, uint32 value):
        self.thisptr.set_number_species(value)
    def has_number_reactions(self):
        return self.thisptr.has_number_reactions()
    def clear_number_reactions(self):
        self.thisptr.clear_number_reactions()
    def number_reactions(self):
        return self.thisptr.number_reactions()
    def set_number_reactions(self, uint32 value):
        self.thisptr.set_number_reactions(value)
    def initial_species_count_size(self):
        return self.thisptr.initial_species_count_size()
    def clear_initial_species_count(self):
        self.thisptr.clear_initial_species_count()
    def initial_species_count(self, int index):
        return self.thisptr.initial_species_count(index)
    def set_initial_species_count(self, int index, uint32 value):
        self.thisptr.set_initial_species_count(index,value)
    def add_initial_species_count(self, uint32 value):
        self.thisptr.add_initial_species_count(value)
    def initial_species_count_backward_size(self):
        return self.thisptr.initial_species_count_backward_size()
    def clear_initial_species_count_backward(self):
        self.thisptr.clear_initial_species_count_backward()
    def initial_species_count_backward(self, int index):
        return self.thisptr.initial_species_count_backward(index)
    def set_initial_species_count_backward(self, int index, uint32 value):
        self.thisptr.set_initial_species_count_backward(index,value)
    def add_initial_species_count_backward(self, uint32 value):
        self.thisptr.add_initial_species_count_backward(value)
    def reaction_size(self):
        return self.thisptr.reaction_size()
    def clear_reaction(self):
        self.thisptr.clear_reaction()
#     def reaction(self, int index):
#         return self.thisptr.reaction(index)
#     def mutable_reaction(self, int index):
#         return self.thisptr.mutable_reaction(index)
    def add_reaction(self):
        reaction = ReactionModel_Reaction()
        reaction.thisptr = self.thisptr.add_reaction()
        return reaction
    def stoichiometric_matrix_size(self):
        return self.thisptr.stoichiometric_matrix_size()
    def clear_stoichiometric_matrix(self):
        self.thisptr.clear_stoichiometric_matrix()
    def stoichiometric_matrix(self, int index):
        return self.thisptr.stoichiometric_matrix(index)
    def set_stoichiometric_matrix(self, int index, int32 value):
        self.thisptr.set_stoichiometric_matrix(index,value)
    def add_stoichiometric_matrix(self, int32 value):
        self.thisptr.add_stoichiometric_matrix(value)
    def dependency_matrix_size(self):
        return self.thisptr.dependency_matrix_size()
    def clear_dependency_matrix(self):
        self.thisptr.clear_dependency_matrix()
    def dependency_matrix(self, int index):
        return self.thisptr.dependency_matrix(index)
    def set_dependency_matrix(self, int index, uint32 value):
        self.thisptr.set_dependency_matrix(index,value)
    def add_dependency_matrix(self, uint32 value):
        self.thisptr.add_dependency_matrix(value)
