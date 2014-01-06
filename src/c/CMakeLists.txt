# c src directory

#get the source files
file(GLOB_RECURSE lm_c_source *.cpp *.cc *.h)
file(GLOB to_remove_mainsa lm/main/MainSA.cpp)
file(GLOB to_remove_mainmpi lm/main/MainMPI.cpp)
file(GLOB to_remove_lkd lm/io/lkd/*)
list(REMOVE_ITEM lm_c_source ${to_remove_mainsa} ${to_remove_mainmpi} ${to_remove_lkd})
add_library(lm_c_lib STATIC ${lm_c_source})