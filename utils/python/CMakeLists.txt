# python src directory

#################
#TARGET: python_protobuf
#################

# get the source .proto files
file(GLOB lm_proto_src ../../src/protobuf/*/*/*.proto)

PROTOBUF_GENERATE_PYTHON(proto_pb2s ${lm_proto_src})

set_source_files_properties(${proto_pb2s} PROPERTIES GENERATED TRUE)

add_custom_target(protobuf_python SOURCES ${proto_pb2s})
