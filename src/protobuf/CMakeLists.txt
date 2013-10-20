# protobuf src directory

#get the source files
file(GLOB_RECURSE lm_protobuf_source ./ *.proto)
PROTOBUF_GENERATE_CPP(proto_srcs proto_hdrs ${lm_protobuf_source})
