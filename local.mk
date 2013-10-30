
BUILD_DIR := Build-barkeri
RM := rm -rf
CC := gcc
CCFLAGS := -m64 -g -fPIC -Wall -c -fmessage-length=0 -pthread -DMACOSX
CCDEPENDFLAGS := -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"
CXX := g++
CXXFLAGS := -m64 -g -fPIC -Wall -c -fmessage-length=0 -pthread -DMACOSX
CXXDEPENDFLAGS := -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"
LD := g++
LDFLAGS := -pthread
LIBS := -lpthread
SHLD := g++
SHLDFLAGS := -bundle -fPIC
SHLDOPTO := -o
USE_VERBOSITY_LEVEL := 9
PROTOBUF_PROTOC := /Users/eroberts/usr/Darwin-i386/bin/protoc
PROTOBUF_INCLUDE_DIR := -I/Users/eroberts/usr/Darwin-i386/include
PROTOBUF_LIB_DIR := -L/Users/eroberts/usr/Darwin-i386/lib
PROTOBUF_LIB := -lprotobuf
HDF5_INCLUDE_DIR := -I/Users/eroberts/usr/Darwin-i386/include
HDF5_LIB_DIR := -L/Users/eroberts/usr/Darwin-i386/lib
HDF5_LIB := -lhdf5 -lhdf5_hl
USE_MPI := 1
MPI_COMPILE_FLAGS = -DOMPI_SKIP_MPICXX=1 $(shell /usr/local/bin/mpicc --showme:compile)
MPI_LINK_FLAGS = $(shell /usr/local/bin/mpicc --showme:link)
MPI_LIB_DIR :=
MPI_LIB :=
USE_PYTHON := 1
PYTHON_SWIG := /Users/eroberts/usr/Darwin-i386/bin/swig
PYTHON_INCLUDE_DIR := -I/usr/include/python2.7
PYTHON_LIB_DIR := -L/usr/lib
PYTHON_LIB := -lpython2.7
USE_CUDA := 1
CUDA_NVCC := /usr/local/cuda/bin/nvcc
CUDA_FLAGS := -m64 --ptxas-options=-v --gpu-architecture compute_30 --gpu-code sm_30 -DMACOSX -DCUDA_3D_GRID_LAUNCH -DCUDA_DOUBLE_PRECISION -DTUNE_MPD_Y_BLOCK_Y_SIZE=16 -DTUNE_MPD_Z_BLOCK_Z_SIZE=8
CUDA_INCLUDE_DIR := -I/usr/local/cuda/include
CUDA_LIB_DIR := -L/usr/local/cuda/lib
CUDA_LIB := -lcuda -lcudart
CUDA_GENERATE_PTX_CODE := 1
CUDA_GENERATE_BIN_CODE := 0
CUDA_GENERATE_ASM_CODE := 0
USE_SBML := 1
SBML_INCLUDE_DIR := -I/Users/eroberts/usr/Darwin-i386/include
SBML_LIB_DIR := -L/Users/eroberts/usr/Darwin-i386/lib
SBML_LIB := -lsbml
USE_PROF := 0
PROF_CUDA_ENABLE := 0
PROF_MAX_THREADS := 15
PROF_MAX_EVENTS := 1000000
PROF_OUT_FILE := timing.prof
USE_BOOST := 0
BOOST_TEST_CXXFLAGS := -DBOOST_TEST_DYN_LINK
BOOST_TEST_INCLUDE_DIR := -I/Network/Servers/sol.scs.uiuc.edu/Volumes/HomeRAID2/Homes/erobert3/usr/Darwin-i386/include
BOOST_TEST_LIB_DIR := -L/Network/Servers/sol.scs.uiuc.edu/Volumes/HomeRAID2/Homes/erobert3/usr/Darwin-i386/lib
BOOST_TEST_LIB := -lboost_unit_test_framework
USE_VMD := 1
VMD_INCLUDE_DIR := -I/Applications/VMD.app/Contents/vmd/plugins/include
VMD_INSTALL_DIR := /Users/eroberts/usr/Darwin-i386/lib/vmd/molfile
INSTALL_PREFIX := /Users/eroberts/usr/Darwin-i386

