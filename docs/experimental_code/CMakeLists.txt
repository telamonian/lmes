# experimental_code c directory

include(SourceHelper)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

################
#TARGET: experimental_code
################

add_executable_standalone(TARGET standaloneOptionParsing SRCS standaloneOptionParsing.cpp PASS_THROUGHS EXCLUDE_FROM_ALL)

#file(GLOB standaloneLimitChecking_source standaloneLimitChecking.h)
#add_executable(standaloneLimitChecking EXCLUDE_FROM_ALL ${standaloneLimitChecking_source})
#target_link_libraries(standaloneLimitChecking lm_c_lib
#                                              ${lm_dependency_libs})