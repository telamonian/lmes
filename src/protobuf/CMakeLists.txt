# c src directory

#get the source files
file(GLOB_RECURSE lm_protobuf_source ./ *.proto)

#build and link
#IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#    CUDA_ADD_LIBRARY(hazard SHARED ${cudasource}
#				 OPTIONS -arch sm_30 -lineinfo)
#	PYTHON_ADD_MODULE(stocuda SHARED ${source})
#
#ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
#	CUDA_ADD_LIBRARY(hazard ${cudasource}
#					 OPTIONS -arch sm_30 -lineinfo)
#	PYTHON_ADD_MODULE(stocuda ${source})
#
#ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    
#target_link_libraries(stocuda
#        ${Boost_LIBRARIES}
#        ${PYTHON_LIBRARIES}
#        hazard
#    )