cdef class ReactionModel_Reaction:
    cdef CppReactionModel_Reaction* thisptr
    def __cinit__(self):
        self.thisptr = new CppReactionModel_Reaction()
    def __dealloc__(self):
        del self.thisptr
    def has_type(self):
        return self.thisptr.has_type()
    def clear_type(self):
        self.thisptr.clear_type()
    def type(self):
        return self.thisptr.type()
    def set_type(self, uint32 value):
        self.thisptr.set_type(value)
    def rate_constant_size(self):
        return self.thisptr.rate_constant_size()
    def clear_rate_constant(self):
        self.thisptr.clear_rate_constant()
    def rate_constant(self, int index):
        return self.thisptr.rate_constant(index)
    def set_rate_constant(self, int index, double value):
        self.thisptr.set_rate_constant(index,value)
    def add_rate_constant(self, double value):
        self.thisptr.add_rate_constant(value)
    def has_rate_has_noise(self):
        return self.thisptr.has_rate_has_noise()
    def clear_rate_has_noise(self):
        self.thisptr.clear_rate_has_noise()
    def rate_has_noise(self):
        return self.thisptr.rate_has_noise()
    def set_rate_has_noise(self, bool value):
        self.thisptr.set_rate_has_noise(value)
    def has_rate_noise_variance(self):
        return self.thisptr.has_rate_noise_variance()
    def clear_rate_noise_variance(self):
        self.thisptr.clear_rate_noise_variance()
    def rate_noise_variance(self):
        return self.thisptr.rate_noise_variance()
    def set_rate_noise_variance(self, double value):
        self.thisptr.set_rate_noise_variance(value)
    def has_rate_noise_tau(self):
        return self.thisptr.has_rate_noise_tau()
    def clear_rate_noise_tau(self):
        self.thisptr.clear_rate_noise_tau()
    def rate_noise_tau(self):
        return self.thisptr.rate_noise_tau()
    def set_rate_noise_tau(self, double value):
        self.thisptr.set_rate_noise_tau(value)
