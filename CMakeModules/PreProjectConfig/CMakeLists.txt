# import all of the config variables
include(${CMAKE_CONFIG_PATH})

# export only the variables we need before the "project(...)" line
set(CMAKE_TOOLCHAIN ${CMAKE_TOOLCHAIN} PARENT_SCOPE)