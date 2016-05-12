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
    set(configPath ${CMAKE_CURRENT_SOURCE_DIR}/CMakeConfig.txt)

    file(GLOB existingCMakeConfig ${configPath})
    if(existingCMakeConfig)
        message(STATUS "Using existing config file: ${configPath}")
    else(existingCMakeConfig)
        message(STATUS "No config file detected, creating new one at ${configPath}. Edit this file to change build/install settings.")
        file(WRITE ${configPath} "")
        cat(${configTemplatePath} ${configPath})
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
endfunction()

