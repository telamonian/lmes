cmake_minimum_REQUIRED(VERSION 2.8.7)
#SET(CMAKE_C_COMPILER gcc)
#SET(CMAKE_CXX_COMPILER g++)

#SET(CUDA_NVCC_FLAGS --ptxas-options=-v;--gpu-architecture;compute_30;--gpu-code;sm_30;-DMACOSX;-DCUDA_3D_GRID_LAUNCH;-DCUDA_DOUBLE_PRECISION;-DTUNE_MPD_Y_BLOCK_Y_SIZE=16;-DTUNE_MPD_Z_BLOCK_Z_SIZE=8)
#SET(CMAKE_C_FLAGS -m64 -g -fPIC -Wall -c -fmessage-length=0 CACHE STRING "cmake c flags")
#SET(CMAKE_CXX_FLAGS -m64 -g -fPIC -Wall -c -fmessage-length=0 CACHE STRING "cmake cpp flags")
#SET(CUDA_NVCC_FLAGS -m64)
#;--ptxas-options=-v;--gpu-architecture;compute_30;--gpu-code;sm_30;-DMACOSX;-DCUDA_3D_GRID_LAUNCH;-DCUDA_DOUBLE_PRECISION;-DTUNE_MPD_Y_BLOCK_Y_SIZE=16;-DTUNE_MPD_Z_BLOCK_Z_SIZE=8)

################
project(lm)
################
#if certain packages cannot be found, the end-user (you) can change the variables in CMakeConfig.txt
include(CMakeConfig.txt)

add_definitions(-DBUILD_INFO -DVERBOSITY_LEVEL=10 -DOPT_CUDA)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DLINUX)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -m64 -fPIC -Wall -fmessage-length=0")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m64 -fPIC -Wall -fmessage-length=0")
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DMACOSX)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -stdlib=libstdc++ -m64 -fPIC -Wall -fmessage-length=0")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -stdlib=libstdc++ -m64 -fPIC -Wall -fmessage-length=0")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
SET(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMakeModules )

find_package(Threads REQUIRED)
find_package(libsbml REQUIRED)
find_package(Protobuf REQUIRED)
find_package(HDF5 COMPONENTS C HL REQUIRED)
find_package(MPI REQUIRED)
find_package(cuda REQUIRED)
find_package(SWIG)
find_package(PythonInterp)
find_package(PythonLibs)
find_package(Boost)

include_directories(profile
                    src/c/
                    ${CMAKE_CURRENT_BINARY_DIR}/src/c/
                    src/cuda/
                    src/protobuf/
                    src/swig/
                    ${libsbml_INCLUDE_DIRS} 
                    ${PROTOBUF_INCLUDE_DIRS}
                    ${HDF5_INCLUDE_DIRS}
                    ${MPI_C_INCLUDE_PATH}
                    ${CUDA_INCLUDE_DIRS}
                    ${PYTHON_INCLUDE_DIRS})

#add subdirectories

add_subdirectory(src/cuda)
add_subdirectory(src/c)
#add_subdirectory(src/protobuf)
add_dependencies(lm_cuda_lib lm_c_lib)

#set_source_files_properties(${proto_srcs} ${proto_hdrs} PROPERTIES GENERATED TRUE)
#get_directory_property(proto_vars DIRECTORY src/protobuf DEFINITION proto_srcs)

set(lm_main_src src/c/lm/main/MainMPI.cpp)
add_executable(lm ${lm_main_src})

target_link_libraries(lm lm_c_lib 
                         lm_cuda_lib 
                         ${CMAKE_THREAD_LIBS_INIT}
                         ${libsbml_LIBRARIES}
                         ${PROTOBUF_LIBRARIES}
                         ${HDF5_LIBRARIES}
                         ${HDF5_HL_LIBRARIES}
                         ${MPI_C_LIBRARIES}
                         ${CUDA_LIBRARIES}
                         ${PYTHON_LIBRARY_DIRS})

enable_testing()
add_subdirectory(testing/c/gmock-1.7.0)
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
                    ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

set(lm_test_src testing/c/lm/main/LocalReplicateSupervisor.cpp)

add_executable(lm_test ${lm_test_src})
target_link_libraries(lm_test lm_c_lib 
                              lm_cuda_lib 
                              ${CMAKE_THREAD_LIBS_INIT}
                              ${libsbml_LIBRARIES}
                              ${PROTOBUF_LIBRARIES}
                              ${HDF5_LIBRARIES}
                              ${HDF5_HL_LIBRARIES}
                              ${MPI_C_LIBRARIES}
                              ${CUDA_LIBRARIES}
                              ${PYTHON_LIBRARY_DIRS}
                              gmock
                              gmock_main
                              gtest
                              gtest_main)

add_test(
    NAME lm_test
    COMMAND lm_test
)
