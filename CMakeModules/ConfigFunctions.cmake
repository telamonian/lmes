# a module with functions that make working with the CMakeConfig.txt file easier

# Cmake "cat" function definition. Appends the contents of file at path IN_FILE to file at path OUT_FILE
function(cat IN_FILE OUT_FILE)
    file(READ ${IN_FILE} CONTENTS)
    file(APPEND ${OUT_FILE} "${CONTENTS}")
    file(APPEND ${OUT_FILE} "\n")
endfunction()

function(chooseToolchain)
    # set the toolchain and corresponding default compiler paths
    if(${CMAKE_TOOLCHAIN} STREQUAL gnu)
        set(CMAKE_C_COMPILER gcc)
        set(CMAKE_CXX_COMPILER g++)
    elseif(${CMAKE_TOOLCHAIN} STREQUAL clang)
        set(CMAKE_C_COMPILER clang)
        set(CMAKE_CXX_COMPILER clang++)
    elseif(${CMAKE_TOOLCHAIN} STREQUAL intel)
        set(CMAKE_C_COMPILER icc)
        set(CMAKE_CXX_COMPILER icpc)
    else(${CMAKE_TOOLCHAIN} STREQUAL gnu)
        # warn if CMAKE_TOOLCHAIN has been set to an unrecognized value
        message(WARNING "No support for specified CMAKE_TOOLCHAIN: ${CMAKE_TOOLCHAIN}. Compilers set to CMake default.")
    endif(${CMAKE_TOOLCHAIN} STREQUAL gnu)

    # allow for specific compiler path overrides
    foreach(lang C;CXX)
        if(DEFINED CMAKE_${lang}_COMPILER)
            SetConfigDefault(CMAKE_${lang}_COMPILER CUSTOM_${lang}_COMPILER ${CMAKE_${lang}_COMPILER})
            set(CMAKE_${lang}_COMPILER ${CMAKE_${lang}_COMPILER} PARENT_SCOPE)
        endif(DEFINED CMAKE_${lang}_COMPILER)
    endforeach(lang)
endfunction()

function(initializeCMakeConfig)
    set(configTemplatePath ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/CMakeConfig.tmpl)
    set(CMAKE_CONFIG_PATH ${CMAKE_SOURCE_DIR}/CMakeConfig.txt)
    set(CMAKE_CONFIG_PATH ${CMAKE_CONFIG_PATH} PARENT_SCOPE)

    file(GLOB existingCMakeConfig ${CMAKE_CONFIG_PATH})
    if(existingCMakeConfig)
        message(STATUS "Using existing config file: ${CMAKE_CONFIG_PATH}")
    else(existingCMakeConfig)
        message(STATUS "No config file detected, creating new one at ${CMAKE_CONFIG_PATH}. Edit this file to change build/install settings.")
        file(WRITE ${CMAKE_CONFIG_PATH} "")
        cat(${configTemplatePath} ${CMAKE_CONFIG_PATH})
    endif(existingCMakeConfig)
endfunction()

# a function that allows for easy set up of default variables while allowing the end-user to override said defaults in CMakeConfig.txt
# example:
# SetDefaultWithOverride(CMAKE_C_FLAGS CUSTOM_C_FLAGS
#                        "-g -march=native -fPIC -Wall -fmessage-length=0")
#
# the above is equivalent to the code:
# IF(DEFINED CUSTOM_C_FLAGS)
#     SET(CMAKE_C_FLAGS ${CUSTOM_C_FLAGS})
#     MESSAGE(STATUS "Using custom value for CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
# ELSE(DEFINED CUSTOM_C_FLAGS)
#     SET(CMAKE_C_FLAGS "-g -march=native -fPIC -Wall -fmessage-length=0")
# ENDIF(DEFINED CUSTOM_C_FLAGS)
#
function(SetConfigDefault varName configVarName default)
    if(DEFINED ${configVarName})
        set(${varName} ${${configVarName}})
        message(STATUS "Using custom value for ${varName}: ${${varName}}")
    else(DEFINED ${configVarName})
        set(${varName} ${default})
        message(STATUS "Using default value for ${varName}: ${${varName}}")
    endif(DEFINED ${configVarName})
    set(${varName} ${${varName}} PARENT_SCOPE)

    # if requested, set the var in the CMakeCache as well
    if(ARGV3 STREQUAL "CACHE")
        # wrap setting the cache to prevent unnecessary updates to cmake build
        get_property(cache_var CACHE ${varName} PROPERTY VALUE)
        get_property(cache_docstring CACHE ${varName} PROPERTY HELPSTRING)
        if(NOT ${varName} STREQUAL cache_var)
            set(${varName} ${${varName}} CACHE STRING "${cache_docstring}" FORCE)
        endif(NOT ${varName} STREQUAL cache_var)
    endif(ARGV3 STREQUAL "CACHE")
endfunction()

# a version of SetConfigDefault that considers the empty string to be the same as an undefined value. Useful for variables that default to "", e.g. CMAKE_BUILD_TYPE
function(SetConfigDefaultIgnoreEmpty varName configVarName default)
    if(${configVarName} STREQUAL "")
        string(RANDOM LENGTH 16 configVarName)
    endif(${configVarName} STREQUAL "")
    SetConfigDefault(${varName} ${configVarName} ${default} ${ARGV3})
endfunction()

# a version of SetConfigDefault that will only set varName if it is empty or undefined
function(SetConfigDefaultIfEmpty varName configVarName default)
    if(${varName} STREQUAL "" OR (NOT DEFINED ${varName}))
        SetConfigDefault(${varName} ${configVarName} ${default} ${ARGV3})
    endif(${varName} STREQUAL "" OR (NOT DEFINED ${varName}))
endfunction()
