
PROTOBUF_GEN_SRC += \
./src/c/lm/io/DiffusionModel.pb.cc \
./src/c/lm/io/FirstPassageTimes.pb.cc \
./src/c/lm/io/Lattice.pb.cc \
./src/c/lm/io/ParameterValues.pb.cc \
./src/c/lm/io/ReactionModel.pb.cc \
./src/c/lm/io/SpatialModel.pb.cc \
./src/c/lm/io/SpeciesCounts.pb.cc \
./src/c/lm/message/SimulationParameters.pb.cc

# Each subdirectory must supply rules for building sources it contributes
./src/c/%.pb.cc: ./src/protobuf/%.proto 
	@echo 'Building file: $<'
	@echo 'Invoking: $(PROTOBUF_PROTOC)'
	@echo $$PATH
	$(PROTOBUF_PROTOC) --proto_path=./src/protobuf --cpp_out=./src/c $<
	@echo 'Finished building: $<'
	@echo ' '
