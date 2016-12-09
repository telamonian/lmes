# c src directory

#################
#TARGET: lm_c_lib
#################
# list of files/dirs to exclude from the build of lm_c_lib
set(to_remove_globs lm/main/Main.cpp lm/main/MainArgs.cpp lm/neus/*)

# get the source files
file(GLOB_RECURSE lm_c_source *.cpp *.cc *.h)

# remove files from the sources list as appropriate
file(GLOB to_remove_srcs ${to_remove_globs})
list(REMOVE_ITEM lm_c_source ${to_remove_srcs})

file(GLOB_RECURSE lm_proto_src ../protobuf/*.proto)

#include_directories(${CMAKE_CURRENT_BINARY_DIR})
PROTOBUF_GENERATE_CPP(proto_srcs proto_hdrs ${lm_proto_src})
PROTOBUF_GENERATE_PYTHON(proto_pb2s ${lm_proto_src})

set_source_files_properties(${proto_srcs} PROPERTIES GENERATED TRUE)
set_source_files_properties(${proto_hdrs} PROPERTIES GENERATED TRUE)

add_library(lm_c_lib STATIC ${lm_c_source} ${proto_hdrs} ${proto_srcs})

#set(proto_hdrs ${proto_hdrs} PARENT_SCOPE)
