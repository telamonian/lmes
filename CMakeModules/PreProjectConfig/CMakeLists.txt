# import all of the config variables
include(${CMAKE_CONFIG_PATH})

# export only the variables we need before the "project(...)" line
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} PARENT_SCOPE)
set(CMAKE_TOOLCHAIN ${CMAKE_TOOLCHAIN} PARENT_SCOPE)
