# utils c directory

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

################
#TARGET: utils
################
file(GLOB lm_python_source lm_python.cpp)
add_executable(lm_python EXCLUDE_FROM_ALL ${lm_python_source})
target_link_libraries(lm_python lm_c_lib
                                ${libsbml_LIBRARIES}
                                ${PROTOBUF_LIBRARIES}
                                ${HDF5_LIBRARIES}
                                ${HDF5_HL_LIBRARIES})

file(GLOB lm_sbml_import_source lm_sbml_import.cpp)
add_executable(lm_sbml_import EXCLUDE_FROM_ALL ${lm_sbml_import_source})
target_link_libraries(lm_sbml_import lm_c_lib
                                     ${libsbml_LIBRARIES}
                                     ${PROTOBUF_LIBRARIES}
                                     ${HDF5_LIBRARIES}
                                     ${HDF5_HL_LIBRARIES})
                                     
file(GLOB lm_setdm_source lm_setdm.cpp util.cpp)
add_executable(lm_setdm EXCLUDE_FROM_ALL ${lm_setdm_source})
target_link_libraries(lm_setdm lm_c_lib
                               ${libsbml_LIBRARIES}
                               ${PROTOBUF_LIBRARIES}
                               ${HDF5_LIBRARIES}
                               ${HDF5_HL_LIBRARIES})
                               
file(GLOB lm_setp_source lm_setp.cpp)
add_executable(lm_setp EXCLUDE_FROM_ALL ${lm_setp_source})
target_link_libraries(lm_setp lm_c_lib
                              ${libsbml_LIBRARIES}
                              ${PROTOBUF_LIBRARIES}
                              ${HDF5_LIBRARIES}
                              ${HDF5_HL_LIBRARIES})
                               
file(GLOB lm_setrm_source lm_setrm.cpp util.cpp)
add_executable(lm_setrm EXCLUDE_FROM_ALL ${lm_setrm_source})
target_link_libraries(lm_setrm lm_c_lib
                               ${libsbml_LIBRARIES}
                               ${PROTOBUF_LIBRARIES}
                               ${HDF5_LIBRARIES}
                               ${HDF5_HL_LIBRARIES})
