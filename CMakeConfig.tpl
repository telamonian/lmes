#in order to commit future changes to this file, first run
#:$$ git update-index --no-assume-unchanged CMakeConfig.txt
#then commit normally, then run
#:$$ git update-index --assume-unchanged CMakeConfig.txt
#and commit again

#sets the prefix dir for installation of lm (ie running "make install")
#SET(CMAKE_INSTALL_PREFIX $CMAKE_INSTALL_PREFIX)

#USE_CUDA can be set to yes, no, or optional, in which case CUDA will be used if cmake can find it on your system
#SET(USE_CUDA $USE_CUDA)

#Set the verbosity level, as described in the lm::Print function. 10 is complete, 4 is normal-ish
#SET(VERBOSITY_LEVEL $VERBOSITY_LEVEL)

#uncomment and change the last part of the next line if some of your libraries/header files are on a nonstandard path
#SET(CMAKE_PREFIX_PATH $CMAKE_PREFIX_PATH)

############################
#library specific path settings
############################

#SET(CUDA_TOOLKIT_ROOT_DIR $CUDA_TOOLKIT_ROOT_DIR)
#SET(CUDA_SDK_ROOT_DIR $CUDA_SDK_ROOT_DIR)

#SET(ENV{HDF5_ROOT} $HDF5_ROOT)

#SET(MPI_C_COMPILER $MPI_C_COMPILER)
#SET(MPI_C_EXEC $MPI_C_EXEC)
#SET(MPI_CXX_COMPILER $MPI_CXX_COMPILER)
#SET(MPI_CXX_EXEC $MPI_CXX_EXEC)

#SET(PYTHON_LIBRARY $PYTHON_LIBRARY)
#SET(PYTHON_INCLUDE_DIR $PYTHON_INCLUDE_DIR)

#SET(PROTOBUF_ROOT $PROTOBUF_ROOT)

#SET(SBML_ROOT $SBML_ROOT)

############################
#deprecated library specific path settings
############################

#SET(BOOST_ROOT $BOOST_ROOT)

#SET(SWIG_EXECUTABLE $SWIG_EXECUTABLE)