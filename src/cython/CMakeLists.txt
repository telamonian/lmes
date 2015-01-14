## Generate a single pair of .pyx and .pxd files from cython source tree
# Cmake "cat" function definition
function(cat IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

# Prepare temporary .pyx and .pxd files to "cat" to:
file(WRITE lm_cython.pyx.tmp "")
file(WRITE lm_cython.pxd.tmp "")

# Get a list of all the .pyx and .pxd files by recursively globbing down the cython source tree
file(GLOB_RECURSE lm_cython_pyxs *.pyx)
file(GLOB_RECURSE lm_cython_pxds *.pxd)

# Call the "cat" function for each of the cython source files
foreach(lm_cython_pyx ${lm_cython_pyxs})
  cat(${lm_cython_pyx} lm_cython.pyx)
endforeach()
foreach(lm_cython_pxds ${lm_cython_pxds})
  cat(${lm_cython_pxd} lm_cython.pxd)
endforeach()

# Copy the temporary .pyx and .pxd files to the final location
configure_file(lm_cython.pyx.tmp lm_cython.pyx COPYONLY)
configure_file(lm_cython.pxd.tmp lm_cython.pxd COPYONLY)

set(lm_cython_libs lm_c_lib
                   ${CMAKE_THREAD_LIBS_INIT}
                   ${libsbml_LIBRARIES}
                   ${PROTOBUF_LIBRARIES}
                   ${HDF5_LIBRARIES}
                   ${HDF5_HL_LIBRARIES}
                   ${MPI_C_LIBRARIES}
                   ${PYTHON_LIBRARY_DIRS})

# Specify that the .pyx file is a C++ file.
set_source_files_properties(
    lm_cython.pyx
    PROPERTIES CYTHON_IS_CXX TRUE)

## Build the actual cython .so file
# Multi-file cython modules do not appear to be working at the moment.
cython_add_module(lm_cython lm_cython.pyx)
target_link_libraries(lm_cython ${lm_cython_libs})

# If the pyx file is a C++ file, we should specify that here.
#set_source_files_properties(
#    test/Hdf5File.pyx
#    test/ReactionModel.pyx
#    PROPERTIES CYTHON_IS_CXX TRUE)
#
# Multi-file cython modules do not appear to be working at the moment.
#cython_add_module(ReactionModel test/ReactionModel.pyx)
#cython_add_module(Hdf5File test/Hdf5File.pyx)
#
#add_dependencies(Hdf5File ReactionModel)
#
#SET(LM_C_LIB lm_c_lib)
#IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#    target_link_libraries(ReactionModel -Wl,--whole-archive)
#    target_link_libraries(ReactionModel ${LM_C_LIB})
#    target_link_libraries(ReactionModel -Wl,--no-whole-archive)
#ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#    target_link_libraries(ReactionModel -Wl,-force_load)
#    target_link_libraries(ReactionModel ${LM_C_LIB})
#ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#    target_link_libraries(Hdf5File -Wl,--whole-archive)
#    target_link_libraries(Hdf5File ${LM_C_LIB})
#    target_link_libraries(Hdf5File -Wl,--no-whole-archive)
#ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#    target_link_libraries(Hdf5File -Wl,-force_load)
#    target_link_libraries(Hdf5File ${LM_C_LIB})
#ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#
#target_link_libraries(ReactionModel ${lm_cython_libs})
#target_link_libraries(Hdf5File ${lm_cython_libs})
#
#cython_add_module(lm_cython test/ReactionModel.pyx)
#target_link_libraries(lm_cython ${lm_cython_libs})

#cython_add_modulehdf5 hdf5.pyx ${CMAKE_CURRENT_SOURCE_DIR}/src/c/lm/io/hdf5/SimulationFile.cpp)
