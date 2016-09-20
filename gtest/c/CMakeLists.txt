# gtest c src directory
add_subdirectory(gmock-1.7.0)
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}
                    ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR})

#################
#TARGET: lmes_gtest
#################
#get the source files
file(GLOB_RECURSE lmes_gtest_c_source lm/*.cpp lm/*.cc lm/*.h)

# remove all files with certain prefixes
file(GLOB_RECURSE to_remove off_* old_*)
if(NOT "${to_remove}" STREQUAL "")
    # REMOVE_ITEM causes problems if to_remove is empty, so guard against that
    list(REMOVE_ITEM lmes_gtest_c_source ${to_remove})
endif(NOT "${to_remove}" STREQUAL "")

add_executable(lmes_gtest ${lmes_gtest_c_source})
set_target_properties(gmock gmock_main gtest gtest_main lmes_gtest PROPERTIES EXCLUDE_FROM_ALL TRUE)

SET(LM_C_LIB lm_c_lib)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(lmes_gtest -Wl,--whole-archive)
    target_link_libraries(lmes_gtest ${LM_C_LIB})
    target_link_libraries(lmes_gtest -Wl,--no-whole-archive)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(lmes_gtest -Wl,-force_load)
    target_link_libraries(lmes_gtest ${LM_C_LIB})
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
IF(BUILD_WITH_CUDA)
    target_link_libraries(lmes_gtest lm_cuda_lib)
ENDIF(BUILD_WITH_CUDA)

target_link_libraries(lmes_gtest ${lm_dependency_libs}
                                 gmock
                                 gmock_main
                                 gtest
                                 gtest_main)

##set_property(TARGET lmes_gtest APPEND_STRING PROPERTY COMPILE_FLAGS " -w")
#IF(USE_CUDA STREQUAL yes OR (CUDA_FOUND AND USE_CUDA STREQUAL optional))
#    target_link_libraries(lmes_gtest lm_cuda_lib
#                                   ${CUDA_LIBRARIES})
#ENDIF(USE_CUDA STREQUAL yes OR (CUDA_FOUND AND USE_CUDA STREQUAL optional)) 
#target_link_libraries(lmes_gtest lm_c_lib 
#                               ${CMAKE_THREAD_LIBS_INIT}
#                               ${libsbml_LIBRARIES}
#                               ${PROTOBUF_LIBRARIES}
#                               ${HDF5_LIBRARIES}
#                               ${HDF5_HL_LIBRARIES}
#                               ${MPI_C_LIBRARIES}
#                               ${PYTHON_LIBRARY_DIRS}
#                               gmock
#                               gmock_main
#                               gtest
#                               gtest_main)

add_test(
    NAME lmes_gtest
    COMMAND lmes_gtest
)