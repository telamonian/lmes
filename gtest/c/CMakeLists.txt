# gtest c src directory
add_subdirectory(gmock-1.7.0)
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
                    ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR})

#################
#TARGET: gtest_lm
#################
#get the source files
file(GLOB_RECURSE gtest_lm_c_source lm/*.cpp lm/*.cc lm/*.h)
file(GLOB_RECURSE to_remove_off off_*)
list(REMOVE_ITEM gtest_lm_c_source ${to_remove_off})

add_executable(gtest_lm ${gtest_lm_c_source})
#set_property(TARGET gtest_lm APPEND_STRING PROPERTY COMPILE_FLAGS " -w")
IF(USE_CUDA STREQUAL yes OR (CUDA_FOUND AND USE_CUDA STREQUAL optional))
    target_link_libraries(gtest_lm lm_cuda_lib
                                   ${CUDA_LIBRARIES})
ENDIF(USE_CUDA STREQUAL yes OR (CUDA_FOUND AND USE_CUDA STREQUAL optional)) 
target_link_libraries(gtest_lm lm_c_lib 
                               ${CMAKE_THREAD_LIBS_INIT}
                               ${libsbml_LIBRARIES}
                               ${PROTOBUF_LIBRARIES}
                               ${HDF5_LIBRARIES}
                               ${HDF5_HL_LIBRARIES}
                               ${MPI_C_LIBRARIES}
                               ${PYTHON_LIBRARY_DIRS}
                               gmock
                               gmock_main
                               gtest
                               gtest_main)

add_test(
    NAME gtest_lm
    COMMAND gtest_lm
)