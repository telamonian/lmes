# c src directory

#get the source files
file(GLOB_RECURSE lm_c_source *.cpp *.cc *.h)
file(GLOB to_remove_mainsa lm/main/MainSA.cpp)
file(GLOB to_remove_mainmpi lm/main/MainMPI.cpp)
file(GLOB to_remove_lkd lm/io/lkd/*)
list(REMOVE_ITEM lm_c_source ${to_remove_mainsa} ${to_remove_mainmpi} ${to_remove_lkd})

file(GLOB lm_proto_src ../protobuf/*/*/*.proto)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
PROTOBUF_GENERATE_CPP(proto_srcs proto_hdrs ${lm_proto_src})

add_library(lm_c_lib STATIC ${lm_c_source} ${proto_srcs} ${proto_hdrs})