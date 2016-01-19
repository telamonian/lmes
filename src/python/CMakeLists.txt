# python src directory

#################
#TARGET: python_protobuf
#################

# get the source .proto files
file(GLOB lm_proto_src ../protobuf/*/*/*.proto)

PROTOBUF_GENERATE_PYTHON(proto_pb2s ${lm_proto_src})

set_source_files_properties(${proto_pb2s} PROPERTIES GENERATED TRUE)

add_custom_target(python_protobuf SOURCES ${proto_pb2s})
