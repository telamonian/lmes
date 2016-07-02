# experimental_code c directory

include(SourceHelper)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

################
#TARGET: experimental_code
################
add_executable_standalone(TARGET MapMacro SRCS MapMacro.cpp PASS_THROUGHS EXCLUDE_FROM_ALL)
add_executable_standalone(TARGET standaloneLimitChecking SRCS standaloneLimitChecking.cpp PASS_THROUGHS EXCLUDE_FROM_ALL)
add_executable_standalone(TARGET standaloneOptionParsing SRCS standaloneOptionParsing.cpp PASS_THROUGHS EXCLUDE_FROM_ALL)
