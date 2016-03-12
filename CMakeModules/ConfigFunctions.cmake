# a module with functions that make working with the CMakeConfig.txt file easier

# Cmake "cat" function definition. Appends the contents of file at path IN_FILE to file at path OUT_FILE
function(cat IN_FILE OUT_FILE)
    file(READ ${IN_FILE} CONTENTS)
    file(APPEND ${OUT_FILE} "${CONTENTS}")
    file(APPEND ${OUT_FILE} "\n")
endfunction()

function(initializeCMakeConfig)
    set(configTemplatePath ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/CMakeConfig.tmpl)
    set(configPath ${CMAKE_CURRENT_SOURCE_DIR}/CMakeConfig.txt)

    file(GLOB existingCMakeConfig ${configPath})
    if(NOT existingCMakeConfig)
        file(WRITE ${configPath} "")
        cat(${configTemplatePath} ${configPath})
    endif(NOT existingCMakeConfig)
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

