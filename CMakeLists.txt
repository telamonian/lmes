# the following commands are wrapped to prevent errors when this CMakeLists.txt is included as a subdirectory in another CMakeLists.txt
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    cmake_minimum_REQUIRED(VERSION 2.8.8)
endif(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)

# initialize custom modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)
include(CLion)
include(ConfigFunctions)
include(TestCompilerFeatures)
initializeCMakeConfig()
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/PreProjectConfig)

# set build type from the config file, if present. Special handling required since unlike many variables this is defined as "" by default
SetConfigDefaultIfEmpty(CMAKE_BUILD_TYPE CUSTOM_BUILD_TYPE Release CACHE)

SetConfigDefault(CMAKE_TOOLCHAIN CMAKE_TOOLCHAIN gnu)
chooseToolchain()

################
project(lm)
################
# if certain packages cannot be found, the end-user (you) can change the variables in CMakeConfig.txt to point to package paths
include(CMakeConfig.txt)
set(ENV{HDF5_ROOT} ${HDF5_ROOT})
enable_testing()

# convert the build type string to upper case for easy comparison
string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
if(CMAKE_BUILD_TYPE_UPPER)
    set(_CMAKE_BUILD_TYPE_UPPER "_${CMAKE_BUILD_TYPE_UPPER}")
endif(CMAKE_BUILD_TYPE_UPPER)

# fix some rpath zanyness with cmake install
if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    endif()
endif()
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Lattice Microbes standard architecture flag
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Intel")
    set(DEFAULT_ARCH_FLAG "-xHost")
else(${CMAKE_CXX_COMPILER_ID} MATCHES "Intel")
    set(DEFAULT_ARCH_FLAG "-march=native")
endif(${CMAKE_CXX_COMPILER_ID} MATCHES "Intel")

# Lattice Microbes standard compiler flags
set(DEFAULT_FLAGS_OPTIMIZED "-m64 -O3 -DNDEBUG -fPIC -Wall -fmessage-length=0") # -D_GLIBCXX_USE_CXX11_ABI=0")
set(DEFAULT_FLAGS_DEBUG "-m64 -g -fPIC -Wall -fmessage-length=0") # -D_GLIBCXX_USE_CXX11_ABI=0")

# Set architecture flag, used in c and c++ flags.
# Will be replaced by the CUSTOM_ARCH_FLAG variable from CMakeConfig.txt if set
SetConfigDefault(ARCH_FLAG CUSTOM_ARCH_FLAG ${DEFAULT_ARCH_FLAG})

foreach(lang C;CXX)
    # Set compiler flags for c and c++ for the various builds.
    # Will be replaced by the CUSTOM_C_FLAGS<_build-name> and CUSTOM_CXX_FLAGS<_build-name> variables from CMakeConfig.txt if set
    if(NOT CMAKE_BUILD_TYPE_UPPER)
        SetConfigDefault(CMAKE_${lang}_FLAGS                   CUSTOM_${lang}_FLAGS                   ${DEFAULT_FLAGS_OPTIMIZED})
    elseif(CMAKE_BUILD_TYPE_UPPER STREQUAL DEBUG)
        SetConfigDefault(CMAKE_${lang}_FLAGS_DEBUG             CUSTOM_${lang}_FLAGS_DEBUG             ${DEFAULT_FLAGS_DEBUG})
    elseif(CMAKE_BUILD_TYPE_UPPER STREQUAL RELEASE)
        SetConfigDefault(CMAKE_${lang}_FLAGS_RELEASE           CUSTOM_${lang}_FLAGS_RELEASE           ${DEFAULT_FLAGS_OPTIMIZED})
    elseif(CMAKE_BUILD_TYPE_UPPER STREQUAL RELWITHDEBINFO)
        SetConfigDefault(CMAKE_${lang}_FLAGS_RELWITHDEBINFO    CUSTOM_${lang}_FLAGS_RELWITHDEBINFO    ${DEFAULT_FLAGS_DEBUG})
    elseif(CMAKE_BUILD_TYPE_UPPER STREQUAL MINSIZEREL)
        SetConfigDefault(CMAKE_${lang}_FLAGS_MINSIZEREL        CUSTOM_${lang}_FLAGS_MINSIZEREL        ${DEFAULT_FLAGS_OPTIMIZED})
    else(NOT CMAKE_BUILD_TYPE_UPPER)
        message(WARNING "No support for specified build type: ${CMAKE_BUILD_TYPE}. ${lang} compiler flags may be incorrectly set.")
        SetConfigDefault(CMAKE_${lang}_FLAGS${_CMAKE_BUILD_TYPE_UPPER} CUSTOM_${lang}_FLAGS${_CMAKE_BUILD_TYPE_UPPER} ${DEFAULT_FLAGS_DEBUG})
    endif(NOT CMAKE_BUILD_TYPE_UPPER)

    # prepend the architecture flag to the rest of the flags, and append any troubleshooting or extra flags.
    set(CMAKE_${lang}_FLAGS${_CMAKE_BUILD_TYPE_UPPER} "${ARCH_FLAG} ${CMAKE_${lang}_FLAGS${_CMAKE_BUILD_TYPE_UPPER}} ${TROUBLESHOOTING_FLAGS} ${EXTRA_${lang}_FLAGS}")
endforeach(lang)

######################################
# Process the USE_CUDA flag.
######################################

# figure out if we should use CUDA or not.
set(BUILD_WITH_CUDA false)

IF (USE_CUDA STREQUAL yes)
    find_package(CUDA REQUIRED)
    set(BUILD_WITH_CUDA true)
ENDIF(USE_CUDA STREQUAL yes)

IF (USE_CUDA STREQUAL optional)
    find_package(CUDA)
    # protects against using CUDA when the headers are present without the libraries and USE_CUDA is set to optional
    IF(CUDA_FOUND AND CUDA_CUDA_LIBRARY)
        set(BUILD_WITH_CUDA true)
    ENDIF(CUDA_FOUND AND CUDA_CUDA_LIBRARY)
ENDIF(USE_CUDA STREQUAL optional)

######################################
# Process the USE_MPI flag.
######################################

# figure out if we should use MPI or not.
set(BUILD_WITH_MPI false)

IF (USE_MPI STREQUAL yes)
    find_package(MPI REQUIRED)
    set(BUILD_WITH_MPI true)
ENDIF(USE_MPI STREQUAL yes)

IF (USE_MPI STREQUAL optional)
    find_package(MPI)
    IF(MPI_C_FOUND)
        set(BUILD_WITH_MPI true)
    ENDIF(MPI_C_FOUND)
ENDIF(USE_MPI STREQUAL optional)

# if we are using MPI, make sure the MPI version is supported.
IF (BUILD_WITH_MPI)
    execute_process(COMMAND ${MPI_C_COMPILER} -v OUTPUT_VARIABLE MPI_VERSION_DUMP ERROR_QUIET)
    STRING(REGEX MATCH "mpich" MPICH_LOWER "${MPI_VERSION_DUMP}")
    STRING(REGEX MATCH "MPICH" MPICH_UPPER "${MPI_VERSION_DUMP}")
    IF(MPICH_LOWER MATCHES "mpich" OR MPICH_UPPER MATCHES "MPICH")
        message("-- Checking for MPICH - success")
    ELSE(MPICH_LOWER MATCHES "mpich" OR MPICH_UPPER MATCHES "MPICH")
        message(FATAL_ERROR "-- Checking for MPICH - FAILED (requires mpich2 > 3.0)")
    ENDIF(MPICH_LOWER MATCHES "mpich" OR MPICH_UPPER MATCHES "MPICH")
ENDIF (BUILD_WITH_MPI)

######################################
# Process the USE_SNAPPY flag.
######################################

# figure out if we should use CUDA or not.
set(BUILD_WITH_SNAPPY false)

IF (USE_SNAPPY STREQUAL yes)
    find_package(snappy REQUIRED)
    set(BUILD_WITH_SNAPPY true)
ENDIF(USE_SNAPPY STREQUAL yes)

IF (USE_SNAPPY STREQUAL optional)
    find_package(SNAPPY)
    IF(snappy_FOUND)
        set(BUILD_WITH_SNAPPY true)
    ENDIF(snappy_FOUND)
ENDIF(USE_SNAPPY STREQUAL optional)

######################################
# Process the USE_CPP11 flag.
######################################

# figure out if we should use CUDA or not.
set(BUILD_WITH_CPP11 false)

IF (USE_CPP11 STREQUAL yes)
    set(BUILD_WITH_CPP11 true)
    message("-- Enabling support for C++11")
ENDIF(USE_CPP11 STREQUAL yes)

#IF (USE_CPP11 STREQUAL optional)
#ENDIF(USE_CPP11 STREQUAL optional)

######################################
# Process the USE_PROFILING flag.
######################################

set(BUILD_WITH_PROFILING false)
if (USE_PROFILING STREQUAL yes)
    set(BUILD_WITH_PROFILING true)
endif(USE_PROFILING STREQUAL yes)

find_package(Threads REQUIRED)

# see if we have libSBML and set a flag, if we do
find_package(libsbml)
if(libsbml_FOUND)
    add_definitions(-DOPT_SBML)
endif(libsbml_FOUND)

find_package(protobuf 2.6.0 REQUIRED)
message(STATUS "protobuf headers: ${PROTOBUF_INCLUDE_DIRS}")

# HDF5 finder isn't strict enough, make sure an error is raised if HDF5 is actually not found
find_package(HDF5 COMPONENTS C HL REQUIRED)
if("${HDF5_VERSION}" STREQUAL "")
    message(FATAL_ERROR "HDF5 library not found, stopping. find_package(HDF5 REQUIRED) passed, but HDF5_VERSION was not set")
endif("${HDF5_VERSION}" STREQUAL "")
# for compatibility with HDF5 libraries older than version 1.8.13. `if` raises a cryptic error if HDF5_VERSION is (for whatever reason) undefined
if(${HDF5_VERSION} VERSION_LESS 1.8.13)
    add_definitions(-DOLD_HDFREE)
endif(${HDF5_VERSION} VERSION_LESS 1.8.13)

find_package(ZLIB REQUIRED)

######################################
# Add any needed compiler definitions.
######################################

# add the build info.
string(TIMESTAMP BUILD_INFO "%Y-%m-%d-%H-%M-%S")
add_definitions(-DBUILD_INFO="${BUILD_INFO}" -DBUILD_TYPE="${CMAKE_BUILD_TYPE}")

# add the verbosity level.
add_definitions(-DVERBOSITY_LEVEL=${VERBOSITY_LEVEL})

if(BUILD_WITH_CUDA)
    add_definitions(-DOPT_CUDA)
endif(BUILD_WITH_CUDA)

IF (BUILD_WITH_MPI)
    add_definitions(-DOPT_MPI)
ENDIF (BUILD_WITH_MPI)

if(BUILD_WITH_PROFILING)
    add_definitions(-DPROF_ENABLE -DPROF_CUDA_ENABLE=0 -DPROF_MAX_THREADS=16 -DPROF_MAX_EVENTS=1000000 -DPROF_OUT_FILE=timing.prof)
endif(BUILD_WITH_PROFILING)

if(BUILD_WITH_SNAPPY)
    add_definitions(-DOPT_SNAPPY)
endif(BUILD_WITH_SNAPPY)

if(BUILD_WITH_CPP11)
    add_definitions(-DOPT_CPP11)
endif(BUILD_WITH_CPP11)

######################################
# Process the CPU feature flags.
######################################

testAVX(BUILD_WITH_AVX ${USE_AVX})
testFMA(BUILD_WITH_FMA ${USE_FMA})
testSVML(BUILD_WITH_SVML ${USE_SVML})
if(BUILD_WITH_AVX)
    add_definitions(-DOPT_AVX)
    # add the explicit AVX support flag
    foreach(lang C;CXX)
        set(CMAKE_${lang}_FLAGS${_CMAKE_BUILD_TYPE_UPPER} "-mavx ${CMAKE_${lang}_FLAGS${_CMAKE_BUILD_TYPE_UPPER}}")
    endforeach(lang)
endif(BUILD_WITH_AVX)
if(BUILD_WITH_FMA)
    add_definitions(-DOPT_FMA)
endif(BUILD_WITH_FMA)
if(BUILD_WITH_SVML)
    add_definitions(-DOPT_SVML)
endif(BUILD_WITH_SVML)

# automagic setup for linux, os X 10.7-8 (and lower?), and os X 10.9 (and higher)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DLINUX)
    if(BUILD_WITH_CPP11)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif(BUILD_WITH_CPP11)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(-DMACOSX)
    # lattice microbes supports gcc-llvm-4.2 as the complier on os X 10.8 and below, and clang 5.0 on os X 10.9
    # in order to support clang, the -stdlib=libstdc++ flag needs to be passed to clang, and the -ccbin clang flag needs to be passed to nvcc during the CUDA compilation (see src/cuda/CMakeLists.txt)
    # if you compiled the Lattice Microbes dependencies using clang in 10.9 (in particular google protobuf) and are getting strange linking errors to a particular dependency, you may need also need to recompile said dependency using the -stdlib=libstdc++ option 
    execute_process(COMMAND sw_vers OUTPUT_VARIABLE SW_VERS_DUMP)
    STRING(REGEX REPLACE ".+ProductVersion:\t10\\.([0-9][0-9]?).*$" "\\1" OSX_VERSION "${SW_VERS_DUMP}" )
    MESSAGE(STATUS "OSX_VERSION=10.${OSX_VERSION}")
    IF(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
        if(BUILD_WITH_CPP11)
            SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
        else(BUILD_WITH_CPP11)
#            SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -stdlib=libstdc++")
#            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
        endif(BUILD_WITH_CPP11)
    ENDIF(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

######################################
# Add any needed include directories.
######################################

include_directories(src/c/
                    ${CMAKE_CURRENT_BINARY_DIR}/src/c/
                    src/protobuf/
                    ${PROTOBUF_INCLUDE_DIRS}
                    ${HDF5_INCLUDE_DIRS}
                    ${ZLIB_INCLUDE_DIRS})

if(BUILD_WITH_CUDA)
    include_directories(src/cuda/ ${CUDA_INCLUDE_DIRS})
endif(BUILD_WITH_CUDA)

IF (BUILD_WITH_MPI)
    include_directories(${MPI_C_INCLUDE_PATH})
ENDIF (BUILD_WITH_MPI)

if(BUILD_WITH_SNAPPY)
    include_directories(${snappy_INCLUDE_DIRS})
endif(BUILD_WITH_SNAPPY)


if(libsbml_FOUND)
    include_directories(${libsbml_INCLUDE_DIRS})
endif(libsbml_FOUND)

# set the list of libraries used in various lm targets across the subdirectories
set(lm_dependency_libs      ${CMAKE_THREAD_LIBS_INIT}
                            ${CMAKE_DL_LIBS}
                            ${PROTOBUF_LIBRARIES}
                            ${HDF5_LIBRARIES}
                            ${HDF5_HL_LIBRARIES}
                            ${ZLIB_LIBRARIES}
                            ${MPI_C_LIBRARIES})

# add various optional libs to lm_dependency_libs, if needed
IF(BUILD_WITH_CUDA)
    set (lm_dependency_libs ${lm_dependency_libs}
                            ${CUDA_CUDA_LIBRARY}  # general cuda libraries
                            ${CUDA_LIBRARIES})    # cuda runtime library
ENDIF(BUILD_WITH_CUDA)
IF(BUILD_WITH_MPI)
    set (lm_dependency_libs ${MPI_C_LIBRARIES})
ENDIF(BUILD_WITH_MPI)
IF(BUILD_WITH_SNAPPY)
    set (lm_dependency_libs ${snappy_LIBRARIES})
ENDIF(BUILD_WITH_SNAPPY)

# add subdirectories
add_subdirectory(src/c)
add_subdirectory(utils/c)
add_subdirectory(utils/python)
add_subdirectory(docs/experimental_code)
add_subdirectory(gtest/c)
IF(BUILD_WITH_CUDA)
    add_subdirectory(src/cuda)
    add_dependencies(lm_cuda_lib lm_c_lib)
ENDIF(BUILD_WITH_CUDA)

#set_source_files_properties(${proto_hdrs} PROPERTIES GENERATED TRUE)
#get_directory_property(proto_vars DIRECTORY src/protobuf DEFINITION proto_srcs)

################
#TARGET: lmes
################
set(lm_main_src src/c/lm/main/Main.cpp src/c/lm/main/MainArgs.cpp)

add_executable(lmes ${lm_main_src})

######################################
# Add any needed target link libraries.
######################################

# The following code is need to ensue that all objects from the custom static libraries are
# linked in to the executable, even if they are not referenced from the main program.
# Without this, static initialization of the ClassFactory does not work.
SET(LM_C_LIB lm_c_lib)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(lmes -Wl,--whole-archive)
    target_link_libraries(lmes ${LM_C_LIB})
    target_link_libraries(lmes -Wl,--no-whole-archive)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(lmes -Wl,-force_load)
    target_link_libraries(lmes ${LM_C_LIB})
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

IF(BUILD_WITH_CUDA)
    target_link_libraries(lmes lm_cuda_lib)
ENDIF(BUILD_WITH_CUDA)
target_link_libraries(lmes ${lm_dependency_libs})

install(TARGETS lmes DESTINATION bin COMPONENT lmes)

add_custom_target(install-lmes
    DEPENDS lmes
    COMMAND
        "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=lmes
        -P "${CMAKE_BINARY_DIR}/cmake_install.cmake"
)

# uninstall target
if(NOT TARGET uninstall)
    configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
            IMMEDIATE @ONLY)

    add_custom_target(uninstall
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()
