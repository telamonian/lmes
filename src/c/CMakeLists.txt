# c src directory

#get the source files
file(GLOB_RECURSE lm_c_source *.cpp *.cc *.h)
file(GLOB to_remove_main lm/main/MainSA.cpp)
file(GLOB to_remove_lkd lm/io/lkd/*)
list(REMOVE_ITEM lm_c_source ${to_remove_main} ${to_remove_lkd})
