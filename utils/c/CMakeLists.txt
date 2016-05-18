# utils c directory

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(lm_utils_libs  ${PROTOBUF_LIBRARIES}
                  ${HDF5_LIBRARIES}
                  ${HDF5_HL_LIBRARIES}
                  ${ZLIB_LIBRARIES}
                  ${MPI_C_LIBRARIES})

################
#TARGET: utils
################
add_custom_target(utils)
                                     
file(GLOB lm_setdm_source lm_setdm.cpp util.cpp)
add_executable(lm_setdm EXCLUDE_FROM_ALL ${lm_setdm_source})
target_link_libraries(lm_setdm lm_c_lib ${lm_utils_libs})
                               
file(GLOB lm_setp_source lm_setp.cpp)
add_executable(lm_setp ${lm_setp_source})
target_link_libraries(lm_setp lm_c_lib ${lm_utils_libs})
                               
file(GLOB lm_setrm_source lm_setrm.cpp util.cpp)
add_executable(lm_setrm ${lm_setrm_source})
target_link_libraries(lm_setrm lm_c_lib ${lm_utils_libs})

file(GLOB sfile_to_lm_source sfile_to_lm.cpp)
add_executable(sfile_to_lm ${sfile_to_lm_source})
target_link_libraries(sfile_to_lm lm_c_lib ${lm_utils_libs})

set(util_targets lm_setp
                 lm_setrm
                 sfile_to_lm)

if(libsbml_FOUND)
    file(GLOB lm_sbml_import_source lm_sbml_import.cpp)
    add_executable(lm_sbml_import ${lm_sbml_import_source})
    target_link_libraries(lm_sbml_import ${libsbml_LIBRARIES})
     IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
         target_link_libraries(lm_sbml_import -Wl,--whole-archive)
	 target_link_libraries(lm_sbml_import lm_c_lib) 
         target_link_libraries(lm_sbml_import -Wl,--no-whole-archive)
         target_link_libraries(lm_sbml_import ${lm_utils_libs})
     ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
         target_link_libraries(lm_sbml_import -Wl,-force_load)
         target_link_libraries(lm_sbml_import ${lm_utils_libs})
     ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    list(APPEND util_targets lm_sbml_import)
endif(libsbml_FOUND)

add_dependencies(utils ${util_targets})

install(TARGETS ${util_targets} DESTINATION bin COMPONENT utils OPTIONAL)

add_custom_target(install-utils
    DEPENDS utils
    COMMAND
        "${CMAKE_COMMAND}" -DCMAKE_INSTALL_COMPONENT=utils
        -P "${CMAKE_BINARY_DIR}/cmake_install.cmake")
