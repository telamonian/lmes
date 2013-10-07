cmake_minimum_required(VERSION 2.8.10)
SET(CMAKE_C_COMPILER gcc)
SET(CMAKE_CXX_COMPILER g++)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(BOOST_ROOT ~/usr)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    SET(PYTHON_INCLUDE_DIR /usr/local/Cellar/python/2.7.5/Frameworks/Python.framework/Headers)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

#ADD_DEFINITIONS(-DPROF_ENABLE)

project(lm C CXX)
SET(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMakeModules )

find_package (Threads)
#target_link_libraries (myapp ${CMAKE_THREAD_LIBS_INIT})
find_package(Protobuf required)
#include_directories(${PROTOBUF_INCLUDE_DIRS})
#include_directories(${CMAKE_CURRENT_BINARY_DIR})
#PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS foo.proto)
#add_executable(bar bar.cc ${PROTO_SRCS} ${PROTO_HDRS})
#target_link_libraries(bar ${PROTOBUF_LIBRARIES})
find_package(libsbml required) #will this work?
find_package(hdf5 required)
find_package(MPI required)
find_package(cuda required)
find_package(SWIG)
find_package(PythonInterp)
find_package(PythonLibs)
find_package(Boost)

# project options
#OPTION(INCLUDE_CUDA "Set to OFF to no search for CUDA" ON)

#find cuda
#IF(INCLUDE_CUDA)
#    FIND_PACKAGE(CUDA)
#
#    IF(CUDA_FOUND)
#        MESSAGE("CUDA has been found")
#    ELSE(CUDA_FOUND)
#        MESSAGE(FATAL_ERROR "CUDA could not be found")
#    ENDIF(CUDA_FOUND)
#    
#ENDIF(INCLUDE_CUDA)

#set(Boost_USE_STATIC_LIBS OFF)
#set(Boost_USE_MULTITHREAD ON)

#include_directories(/Library/Python/2.7/site-packages/numpy-1.8.0.dev_5c944b9_20120828-py2.7-macosx-10.8-x86_64.egg/numpy/core/include/)
#INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS} ${NUMPY_INCLUDE_DIRS})
#link_directories(${PYTHON_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS})

#add subdirectories
ADD_SUBDIRECTORY(src/c)
ADD_SUBDIRECTORY(src/protobuf)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${lm_protobuf_source})
