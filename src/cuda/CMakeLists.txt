# cuda src directory

#set(CUDA_SEPARABLE_COMPILATION ON)
#set(CUDA_HOST_COMPILER "")
#option(CUDA_USE_STATIC_CUDA_RUNTIME "" OFF)

####################
#TARGET: lm_cuda_lib
####################
# static linking currently does not seem to work with some of the functions from the cuda

# set the NVCC specific flags
set(CUDA_NVCC_FLAGS --ptxas-options=-v;--gpu-architecture;compute_30;--gpu-code;sm_30;-DCUDA_3D_GRID_LAUNCH;-DCUDA_DOUBLE_PRECISION;-DTUNE_MPD_Y_BLOCK_Y_SIZE=16;-DTUNE_MPD_Z_BLOCK_Z_SIZE=8)   #;${CUDA_NVCC_FLAGS})

# automatic pass through of host compiler flags can be disabled by uncommenting the following line
#set(CUDA_PROPAGATE_HOST_FLAGS OFF)

# for some reason the cuda code has hundreds of unused constants, functions, etc, so turn off the warnings about these
set(wno_unused_flags "-Wno-unused-const-variable,-Wno-unused-function")
set(CUDA_NVCC_FLAGS --compiler-options;${wno_unused_flags};--linker-options;${wno_unused_flags};${CUDA_NVCC_FLAGS})

# turn off the flags for fancy CPU-based vector math, since they involve built-ins that NVCC doesn't know about
set(fancy_math_definitions OPT_AVX;OPT_FMA;OPT_SVML)
get_property(nvcc_definitions DIRECTORY PROPERTY COMPILE_DEFINITIONS)
list(REMOVE_ITEM nvcc_definitions ${fancy_math_definitions})
set_property(DIRECTORY PROPERTY COMPILE_DEFINITIONS ${nvcc_definitions})

# handle hateful libstdc++/libc++ string handling bug that always seems to crop up with Clang
#if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#    if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
##        set(CUDA_NVCC_FLAGS --compiler-options;-stdlib=libstdc++;--linker-options;-stdlib=libstdc++;${CUDA_NVCC_FLAGS})
#        set(CUDA_NVCC_FLAGS -ccbin;clang++;${CUDA_NVCC_FLAGS})
#    endif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
#endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

#get the source files
file(GLOB lm_cuda_source lm/*.cu lm/*.h)
file(GLOB lm_cuda_source1 lm/*/*.cu lm/*/*.h)
list(APPEND lm_cuda_source ${lm_cuda_source1})
CUDA_ADD_LIBRARY(lm_cuda_lib STATIC ${lm_cuda_source})
