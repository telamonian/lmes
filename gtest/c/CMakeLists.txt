# gtest c src directory
add_subdirectory(gmock-1.7.0)
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
                    ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR})

#################
#TARGET: lm_gtest
#################
#get the source files
file(GLOB_RECURSE lm_gtest_c_source lm/*.cpp lm/*.cc lm/*.h)
file(GLOB_RECURSE to_remove_off off_*)
list(REMOVE_ITEM lm_gtest_c_source ${to_remove_off})

add_executable(lm_gtest ${lm_gtest_c_source})
set_target_properties(gmock gmock_main gtest gtest_main lm_gtest PROPERTIES EXCLUDE_FROM_ALL TRUE)

#set_property(TARGET lm_gtest APPEND_STRING PROPERTY COMPILE_FLAGS " -w")
IF(USE_CUDA STREQUAL yes OR (CUDA_FOUND AND USE_CUDA STREQUAL optional))
    target_link_libraries(lm_gtest lm_cuda_lib
                                   ${CUDA_LIBRARIES})
ENDIF(USE_CUDA STREQUAL yes OR (CUDA_FOUND AND USE_CUDA STREQUAL optional)) 
target_link_libraries(lm_gtest lm_c_lib 
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
    NAME lm_gtest
    COMMAND lm_gtest
)