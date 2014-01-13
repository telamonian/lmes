# protobuf src directory

#get the source files
file(GLOB_RECURSE lm_protobuf_source ./ *.proto)
PROTOBUF_GENERATE_CPP(proto_srcs proto_hdrs ${lm_protobuf_source})

set(PROTOBUF_INCLUDE_DIRS ${PROTOBUF_INCLUDE_DIRS} PARENT_SCOPE)
set(PROTOBUF_LIBRARIES ${PROTOBUF_LIBRARIES} PARENT_SCOPE)
set(proto_srcs ${proto_srcs} PARENT_SCOPE)
set(proto_hdrs ${proto_hdrs} PARENT_SCOPE)