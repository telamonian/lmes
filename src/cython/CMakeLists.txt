set(lm_cython_libs lm_c_lib
                   ${CMAKE_THREAD_LIBS_INIT}
                   ${libsbml_LIBRARIES}
                   ${PROTOBUF_LIBRARIES}
                   ${HDF5_LIBRARIES}
                   ${HDF5_HL_LIBRARIES}
                   ${MPI_C_LIBRARIES}
                   ${PYTHON_LIBRARY_DIRS})
                  
# If the pyx file is a C++ file, we should specify that here.
set_source_files_properties(
    test/Hdf5File.pyx
    test/ReactionModel.pyx
    PROPERTIES CYTHON_IS_CXX TRUE)

# Multi-file cython modules do not appear to be working at the moment.
cython_add_module(lm_cython test/Hdf5File.pyx
                            test/ReactionModel.pyx)
target_link_libraries(lm_cython ${lm_cython_libs})

#cython_add_modulehdf5 hdf5.pyx ${CMAKE_CURRENT_SOURCE_DIR}/src/c/lm/io/hdf5/SimulationFile.cpp)
