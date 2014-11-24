cmake_minimum_REQUIRED(VERSION 2.8.7)
SET(CMAKE_C_COMPILER gcc)
SET(CMAKE_CXX_COMPILER g++)

################
project(lm)
################
enable_testing()
#if certain packages cannot be found, the end-user (you) can change the variables in CMakeConfig.txt to point to package paths
include(CMakeConfig.txt)

#package finding stuff
SET(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMakeModules )

find_package(MPI REQUIRED)
message("testing if mpi is mpich...")
execute_process(COMMAND ${MPI_C_COMPILER} -v OUTPUT_VARIABLE MPI_VERSION_DUMP ERROR_QUIET)
STRING(REGEX MATCH "mpich" MPICH_LOWER "${MPI_VERSION_DUMP}")
STRING(REGEX MATCH "MPICH" MPICH_UPPER "${MPI_VERSION_DUMP}")
IF(MPICH_LOWER MATCHES "mpich" OR MPICH_UPPER MATCHES "MPICH")
    message("...mpi is mpich, proceeding")
ELSE(MPICH_LOWER MATCHES "mpich" OR MPICH_UPPER MATCHES "MPICH")
    message(FATAL_ERROR "...mpi implementation found is not mpich, stopping. Lattice Microbes only supports mpich2 >3.0 at this time. If multiple mpi implementations are installed, please specify path to mpich in CMakeConfig.txt.")
ENDIF(MPICH_LOWER MATCHES "mpich" OR MPICH_UPPER MATCHES "MPICH")

#conditionals for dealing with CUDA
IF (USE_CUDA STREQUAL yes)
    find_package(cuda REQUIRED)
ENDIF(USE_CUDA STREQUAL yes)
IF (USE_CUDA STREQUAL optional)
    find_package(cuda)
ENDIF(USE_CUDA STREQUAL optional)
#protects against using CUDA when the headers are present without the libraries and USE_CUDA is set to optional
IF(CUDA_FOUND AND NOT CUDA_CUDA_LIBRARY)
    set(CUDA_FOUND false)
ENDIF(CUDA_FOUND AND NOT CUDA_CUDA_LIBRARY)

find_package(Threads REQUIRED)
find_package(libsbml REQUIRED)
find_package(protobuf REQUIRED)
find_package(HDF5 COMPONENTS C HL REQUIRED)
find_package(PythonInterp)
find_package(PythonLibs)
#find_package(SWIG)
#find_package(Boost)

#automagic setup for linux, os X 10.7-8 (and lower?), and os X 10.9
add_definitions(-DBUILD_INFO -DVERBOSITY_LEVEL=${VERBOSITY_LEVEL})
IF(USE_CUDA STREQUAL yes OR (CUDA_FOUND AND USE_CUDA STREQUAL optional))
    add_definitions(-DOPT_CUDA)
ENDIF(USE_CUDA STREQUAL yes OR (CUDA_FOUND AND USE_CUDA STREQUAL optional))

IF(DEFINED CUSTOM_C_FLAGS)
    SET(CMAKE_C_FLAGS ${CUSTOM_C_FLAGS})
    MESSAGE(STATUS "Using custom C flags: ${CMAKE_C_FLAGS}")
ELSE(DEFINED CUSTOM_C_FLAGS)
    SET(CMAKE_C_FLAGS "-g -m64 -fPIC -Wall -fmessage-length=0")
ENDIF(DEFINED CUSTOM_C_FLAGS)

IF(DEFINED CUSTOM_CXX_FLAGS)
    SET(CMAKE_CXX_FLAGS ${CUSTOM_CXX_FLAGS})
    MESSAGE(STATUS "Using custom C++ flags: ${CMAKE_CXX_FLAGS}")
ELSE(DEFINED CUSTOM_CXX_FLAGS)
    SET(CMAKE_CXX_FLAGS "-g -m64 -fPIC -Wall -fmessage-length=0")
ENDIF(DEFINED CUSTOM_CXX_FLAGS)

#for now, no c++11 support
#SET(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DLINUX)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(-DMACOSX)
    #lattice microbes supports gcc-llvm-4.2 as the complier on os X 10.8 and below, and clang 5.0 on os X 10.9
    #in order to support clang, the -stdlib=libstdc++ flag needs to be passed to clang, and the -ccbin clang flag needs to be passed to nvcc during the CUDA compilation (see src/cuda/CMakeLists.txt)
    #if you compiled the Lattice Microbes dependencies using clang in 10.9 (in particular google protobuf) and are getting strange linking errors to a particular dependency, you may need also need to recompile said dependency using the -stdlib=libstdc++ option 
    execute_process(COMMAND sw_vers OUTPUT_VARIABLE SW_VERS_DUMP)
    STRING(REGEX REPLACE ".+ProductVersion:\t10\\.([0-9]).+" "\\1" OSX_VERSION "${SW_VERS_DUMP}" )
    MESSAGE(STATUS "OSX_VERSION=10.${OSX_VERSION}")
    IF(OSX_VERSION GREATER 8)
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -stdlib=libstdc++")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
    ENDIF(OSX_VERSION GREATER 8)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

include_directories(profile
                    src/c/
                    ${CMAKE_CURRENT_BINARY_DIR}/src/c/
                    src/cuda/
                    src/cython/
                    src/protobuf/
                    src/swig/
                    ${libsbml_INCLUDE_DIRS} 
                    ${PROTOBUF_INCLUDE_DIRS}
                    ${HDF5_INCLUDE_DIRS}
                    ${MPI_C_INCLUDE_PATH}
                    ${CUDA_INCLUDE_DIRS}
                    ${PYTHON_INCLUDE_DIRS})

#add subdirectories
#add_subdirectory(src/protobuf)

set(CYTHON_OTHER_LIBS ${CMAKE_THREAD_LIBS_INIT}
                      ${libsbml_LIBRARIES}
                      ${PROTOBUF_LIBRARIES}
                      ${HDF5_LIBRARIES}
                      ${HDF5_HL_LIBRARIES}
                      ${MPI_C_LIBRARIES}
                      ${PYTHON_LIBRARY_DIRS})

add_subdirectory(src/c)
IF(USE_CUDA STREQUAL yes OR (CUDA_FOUND AND USE_CUDA STREQUAL optional))
    add_subdirectory(src/cuda)
    add_dependencies(lm_cuda_lib lm_c_lib)
ENDIF(USE_CUDA STREQUAL yes OR (CUDA_FOUND AND USE_CUDA STREQUAL optional)) 
add_subdirectory(gtest/c)

#set_source_files_properties(${proto_hdrs} PROPERTIES GENERATED TRUE)
#get_directory_property(proto_vars DIRECTORY src/protobuf DEFINITION proto_srcs)

################
#TARGET: lm
################
set(lm_main_src src/c/lm/main/MainMPI.cpp)

add_executable(lm ${lm_main_src})

# The following code is need to ensue that all objects from the custom static libraries are
# linked in to the executable, even if they are not referenced from the main program.
# Without this, static initialization of the ClassFactory does not work.
SET(LM_C_LIB lm_c_lib)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(lm -Wl,--whole-archive)
    target_link_libraries(lm ${LM_C_LIB})
    target_link_libraries(lm -Wl,--no-whole-archive)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(lm -Wl,-force_load)
    target_link_libraries(lm ${LM_C_LIB})
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

IF(USE_CUDA STREQUAL yes OR (CUDA_FOUND AND USE_CUDA STREQUAL optional))
    target_link_libraries(lm lm_cuda_lib
                             ${CUDA_LIBRARIES})
ENDIF(USE_CUDA STREQUAL yes OR (CUDA_FOUND AND USE_CUDA STREQUAL optional)) 

target_link_libraries(lm ${CMAKE_THREAD_LIBS_INIT}
                         ${libsbml_LIBRARIES}
                         ${PROTOBUF_LIBRARIES}
                         ${HDF5_LIBRARIES}
                         ${HDF5_HL_LIBRARIES}
                         ${MPI_C_LIBRARIES}
                         ${PYTHON_LIBRARY_DIRS})

install(TARGETS lm DESTINATION bin)

################
#TARGET: cython
################
if (MAKE_CYTHON STREQUAL yes)
    add_custom_target( ReplicatePythonSourceTree ALL ${CMAKE_COMMAND} -P
        ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/ReplicatePythonSourceTree.cmake
        ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
    set(ENV{HDF5_ROOT} ${HDF5_ROOT})
    include(UseCython)  ###cython
    add_subdirectory(src/cython)
endif (MAKE_CYTHON STREQUAL yes)