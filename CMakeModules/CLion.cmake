# a module with functions that extend/fix issues with CMake integration if you're using the CLion IDE

function(include_directories_clion_generated)
    string(REGEX MATCH "[Cc][Ll]ion" BUILT_WITH_CLION "${lm_BINARY_DIR}")
    if(BUILT_WITH_CLION)
        foreach(build_type Debug;MinSizeRel;Release;RelWithDebInfo)
            include_directories(${CMAKE_BINARY_DIR}/../${build_type}/src/c/)
        endforeach(build_type)
    endif(BUILT_WITH_CLION)
endfunction()