# a module with functions that make working with source files and setting up simple targets easier

# Simple function to add standalone executable target that can optionally depend on the rest of the lm code
function(add_executable_standalone)
    set(options DEPENDS_ON_LM NEEDS_CLASSFACTORY)
    set(oneValueArgs TARGET)
    set(multiValueArgs SRCS PASS_THROUGHS)
    cmake_parse_arguments("" "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})



    # set up the srcFiles and add the executable target
    file(GLOB srcFiles ${_SRCS})
    add_executable(${_TARGET} ${_PASS_THROUGHS} ${srcFiles})

    # link the lm libs, if needed
    if(_DEPENDS_ON_LM)
        if(_NEEDS_CLASSFACTORY)
            target_link_lm_libraries_w_classfactory(${_TARGET})
        else(_NEEDS_CLASSFACTORY)
            target_link_lm_libraries(${_TARGET})
        endif(_NEEDS_CLASSFACTORY)
    endif(_DEPENDS_ON_LM)

#    foreach(lib_name ${lm_dependency_libs})
#        message(${lib_name})
#    endforeach(lib_name)

    # link the rest of the dependencies
    target_link_libraries(${_TARGET} ${lm_dependency_libs})
endfunction()

# version of target_link_lm_libraries that enables the use of dynamic ClassFactory stuff at runtime
function(target_link_lm_libraries_w_classfactory TARGET)
    # The following code is need to ensue that all objects from the custom static libraries are
    # linked in to the executable, even if they are not referenced from the main program.
    # Without this, static initialization of the ClassFactory does not work.
    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_link_libraries(${TARGET} -Wl,--whole-archive)
        target_link_libraries(${TARGET} lm_c_lib)
        target_link_libraries(${TARGET} -Wl,--no-whole-archive)
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_link_libraries(${TARGET} -Wl,-force_load)
        target_link_libraries(${TARGET} lm_c_lib)
    endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    if(BUILD_WITH_CUDA)
        target_link_libraries(${TARGET} lm_cuda_lib)
    endif(BUILD_WITH_CUDA)
endfunction()

# use this version of target_link_lm_libraries if your target executable doesn't need the ClassFactory stuff
function(target_link_lm_libraries TARGET)
    target_link_libraries(${TARGET} lm_c_lib)

    if(BUILD_WITH_CUDA)
        target_link_libraries(${TARGET} lm_cuda_lib)
    endif(BUILD_WITH_CUDA)
endfunction()
