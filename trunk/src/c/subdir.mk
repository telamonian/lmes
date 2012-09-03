
# Add inputs and outputs from these tool invocations to the build variables 
MAIN += ./$(BUILD_DIR)/lm

# If we are building with MPI, use the MPI main program, otherwise use the standalone version.
ifeq ($(USE_MPI),0)
MAIN_OBJS := ./$(BUILD_DIR)/src/c/lm/main/MainSA.o
else
MAIN_OBJS := ./$(BUILD_DIR)/src/c/lm/main/MainMPI.o \
./$(BUILD_DIR)/src/c/lm/main/MPIRemoteDataOutputQueue.o
endif

MAIN_OBJS += \
./$(BUILD_DIR)/src/c/lm/main/CheckpointSignaler.o \
./$(BUILD_DIR)/src/c/lm/main/LocalDataOutputWorker.o \
./$(BUILD_DIR)/src/c/lm/main/Main.o \
./$(BUILD_DIR)/src/c/lm/main/ReplicateRunner.o \
./$(BUILD_DIR)/src/c/lm/main/SignalHandler.o

OBJS += \
./$(BUILD_DIR)/src/c/lm/Print.o \
./$(BUILD_DIR)/src/c/lm/builder/Capsule.o \
./$(BUILD_DIR)/src/c/lm/builder/CapsuleShell.o \
./$(BUILD_DIR)/src/c/lm/builder/Cuboid.o \
./$(BUILD_DIR)/src/c/lm/builder/Hemisphere.o \
./$(BUILD_DIR)/src/c/lm/builder/LatticeBuilder.o \
./$(BUILD_DIR)/src/c/lm/builder/Shape.o \
./$(BUILD_DIR)/src/c/lm/builder/Sphere.o \
./$(BUILD_DIR)/src/c/lm/cme/CMESolver.o \
./$(BUILD_DIR)/src/c/lm/cme/FluctuatingNRSolver.o \
./$(BUILD_DIR)/src/c/lm/cme/GillespieDSolver.o \
./$(BUILD_DIR)/src/c/lm/cme/HillSwitch.o \
./$(BUILD_DIR)/src/c/lm/cme/LacHillSwitch.o \
./$(BUILD_DIR)/src/c/lm/cme/NextReactionSolver.o \
./$(BUILD_DIR)/src/c/lm/cme/SelfRegulatingGeneSwitch.o \
./$(BUILD_DIR)/src/c/lm/cme/TwoStateExpression.o \
./$(BUILD_DIR)/src/c/lm/cme/TwoStateHillLoopSwitch.o \
./$(BUILD_DIR)/src/c/lm/cme/TwoStateHillSwitch.o \
./$(BUILD_DIR)/src/c/lm/io/DiffusionModel.pb.o \
./$(BUILD_DIR)/src/c/lm/io/FirstPassageTimes.pb.o \
./$(BUILD_DIR)/src/c/lm/io/Lattice.pb.o \
./$(BUILD_DIR)/src/c/lm/io/ParameterValues.pb.o \
./$(BUILD_DIR)/src/c/lm/io/ReactionModel.pb.o \
./$(BUILD_DIR)/src/c/lm/io/SpatialModel.pb.o \
./$(BUILD_DIR)/src/c/lm/io/SpeciesCounts.pb.o \
./$(BUILD_DIR)/src/c/lm/io/SimulationParameters.o \
./$(BUILD_DIR)/src/c/lm/io/hdf5/SimulationFile.o \
./$(BUILD_DIR)/src/c/lm/io/hdf5/SimulationFile_create.o \
./$(BUILD_DIR)/src/c/lm/main/DataOutputQueue.o \
./$(BUILD_DIR)/src/c/lm/main/ResourceAllocator.o \
./$(BUILD_DIR)/src/c/lm/me/MESolver.o \
./$(BUILD_DIR)/src/c/lm/me/MESolverFactory.o \
./$(BUILD_DIR)/src/c/lm/message/SimulationParameters.pb.o \
./$(BUILD_DIR)/src/c/lm/rdme/ByteLattice.o \
./$(BUILD_DIR)/src/c/lm/rdme/Lattice.o \
./$(BUILD_DIR)/src/c/lm/rdme/NextSubvolumeSolver.o \
./$(BUILD_DIR)/src/c/lm/rdme/RDMESolver.o \
./$(BUILD_DIR)/src/c/lm/rng/RandomGenerator.o \
./$(BUILD_DIR)/src/c/lm/rng/XORShift.o \
./$(BUILD_DIR)/src/c/lm/thread/Thread.o \
./$(BUILD_DIR)/src/c/lm/thread/Worker.o \
./$(BUILD_DIR)/src/c/lm/thread/WorkerManager.o

CPP_DEPS += \
./$(BUILD_DIR)/src/c/lm/Print.d \
./$(BUILD_DIR)/src/c/lm/builder/Capsule.d \
./$(BUILD_DIR)/src/c/lm/builder/CapsuleShell.d \
./$(BUILD_DIR)/src/c/lm/builder/Cuboid.d \
./$(BUILD_DIR)/src/c/lm/builder/Hemisphere.d \
./$(BUILD_DIR)/src/c/lm/builder/LatticeBuilder.d \
./$(BUILD_DIR)/src/c/lm/builder/Shape.d \
./$(BUILD_DIR)/src/c/lm/builder/Sphere.d \
./$(BUILD_DIR)/src/c/lm/cme/CMESolver.d \
./$(BUILD_DIR)/src/c/lm/cme/FluctuatingNRSolver.d \
./$(BUILD_DIR)/src/c/lm/cme/GillespieDSolver.d \
./$(BUILD_DIR)/src/c/lm/cme/HillSwitch.d \
./$(BUILD_DIR)/src/c/lm/cme/LacHillSwitch.d \
./$(BUILD_DIR)/src/c/lm/cme/NextReactionSolver.d \
./$(BUILD_DIR)/src/c/lm/cme/SelfRegulatingGeneSwitch.d \
./$(BUILD_DIR)/src/c/lm/cme/TwoStateExpression.d \
./$(BUILD_DIR)/src/c/lm/cme/TwoStateHillLoopSwitch.d \
./$(BUILD_DIR)/src/c/lm/cme/TwoStateHillSwitch.d \
./$(BUILD_DIR)/src/c/lm/io/FirstPassageTimes.pb.d \
./$(BUILD_DIR)/src/c/lm/io/DiffusionModel.pb.d \
./$(BUILD_DIR)/src/c/lm/io/Lattice.pb.d \
./$(BUILD_DIR)/src/c/lm/io/ParameterValues.pb.d \
./$(BUILD_DIR)/src/c/lm/io/SpatialModel.pb.d \
./$(BUILD_DIR)/src/c/lm/io/SpeciesCounts.pb.d \
./$(BUILD_DIR)/src/c/lm/io/hdf5/SimulationFile.d \
./$(BUILD_DIR)/src/c/lm/io/hdf5/SimulationFile_create.d \
./$(BUILD_DIR)/src/c/lm/main/CheckpointSignaler.d \
./$(BUILD_DIR)/src/c/lm/main/DataOutputQueue.d \
./$(BUILD_DIR)/src/c/lm/main/LocalDataOutputWorker.d \
./$(BUILD_DIR)/src/c/lm/main/SignalHandler.d \
./$(BUILD_DIR)/src/c/lm/main/MPIRemoteDataOutputQueue.d \
./$(BUILD_DIR)/src/c/lm/main/ReplicateRunner.d \
./$(BUILD_DIR)/src/c/lm/main/ResourceAllocator.d \
./$(BUILD_DIR)/src/c/lm/me/MESolver.d \
./$(BUILD_DIR)/src/c/lm/me/MESolverFactory.d \
./$(BUILD_DIR)/src/c/lm/message/SimulationParameters.pb.d \
./$(BUILD_DIR)/src/c/lm/rdme/ByteLattice.d \
./$(BUILD_DIR)/src/c/lm/rdme/Lattice.d \
./$(BUILD_DIR)/src/c/lm/rdme/NextSubvolumeSolver.d \
./$(BUILD_DIR)/src/c/lm/rdme/RDMESolver.d \
./$(BUILD_DIR)/src/c/lm/rng/RandomGenerator.d \
./$(BUILD_DIR)/src/c/lm/rng/XORShift.d \
./$(BUILD_DIR)/src/c/lm/thread/Thread.d \
./$(BUILD_DIR)/src/c/lm/thread/Worker.d \
./$(BUILD_DIR)/src/c/lm/thread/WorkerManager.d

# If we are building with MPI, include the MPI objects.
ifeq ($(USE_MPI),1)
OBJS += ./$(BUILD_DIR)/src/c/lm/MPI.o
CPP_DEPS += ./$(BUILD_DIR)/src/c/lm/MPI.d
endif


# Each subdirectory must supply rules for building sources it contributes
./$(BUILD_DIR)/src/c/%.o: ./src/c/%.cpp 
	@echo 'Building file: $<'
	@echo 'Invoking: $(CXX)'
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) $(CXXDEPENDFLAGS) -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

./$(BUILD_DIR)/src/c/%.o: ./src/c/%.cc 
	@echo 'Building file: $<'
	@echo 'Invoking: $(CXX)'
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) $(CXXDEPENDFLAGS) -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

./$(BUILD_DIR)/src/c/%.o: ./src/c/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: $(CC)'
	mkdir -p $(@D)
	$(CC) $(CCFLAGS) $(INCLUDE_DIRS) $(CCDEPENDFLAGS) -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
