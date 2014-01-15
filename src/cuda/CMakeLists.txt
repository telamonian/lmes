# cuda src directory

SET(CUDA_NVCC_FLAGS --ptxas-options=-v;--gpu-architecture;compute_30;--gpu-code;sm_30;-DCUDA_3D_GRID_LAUNCH;-DCUDA_DOUBLE_PRECISION;-DTUNE_MPD_Y_BLOCK_Y_SIZE=16;-DTUNE_MPD_Z_BLOCK_Z_SIZE=8)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    IF(&{OSX_VERSION GREATER 8)
        SET(CUDA_NVCC_FLAGS "-ccbin /usr/bin/clang;${CUDA_NVCC_FLAGS}")
    ENDIF(&{OSX_VERSION GREATER 8)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../c)

#get the source files
file(GLOB lm_cuda_source lm/*.cu)
file(GLOB lm_cuda_source1 lm/*/*.cu)
list(APPEND lm_cuda_source ${lm_cuda_source1})
CUDA_ADD_LIBRARY(lm_cuda_lib STATIC ${lm_cuda_source})
