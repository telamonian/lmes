from libcpp.string cimport string
from libc.stdint cimport uint32_t  #, int64_t

cimport _Hdf5File

cdef class Hdf5File:
    cdef _Hdf5File.Hdf5File* thisptr