# c src directory

#get the source files
file(GLOB_RECURSE lm_c_source *.cpp *.cc *.h)
file(GLOB to_remove_mainsa lm/main/MainSA.cpp)
file(GLOB to_remove_mainmpi lm/main/MainMPI.cpp)
file(GLOB to_remove_lkd lm/io/lkd/*)
list(REMOVE_ITEM lm_c_source ${to_remove_mainsa} ${to_remove_mainmpi} ${to_remove_lkd})

file(GLOB lm_proto_src ../protobuf/*/*/*.proto)

#include_directories(${CMAKE_CURRENT_BINARY_DIR})
PROTOBUF_GENERATE_CPP(proto_srcs proto_hdrs ${lm_proto_src})

#message(srcs_${proto_srcs})
#message(hdrs_${proto_hdrs})

set_source_files_properties(${proto_srcs} PROPERTIES GENERATED TRUE)
set_source_files_properties(${proto_hdrs} PROPERTIES GENERATED TRUE)

add_library(lm_c_lib STATIC ${lm_c_source} ${proto_hdrs} ${proto_srcs})
#set(proto_hdrs ${proto_hdrs} PARENT_SCOPE)