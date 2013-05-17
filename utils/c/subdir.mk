
# Add inputs and outputs from these tool invocations to the build variables 
UTIL_BINS += \
./$(BUILD_DIR)/bin/lm_setp \
./$(BUILD_DIR)/bin/lm_setdm \
./$(BUILD_DIR)/bin/lm_setrm

UTIL_OBJS += \
./$(BUILD_DIR)/utils/c/util.o

CPP_DEPS += \
./$(BUILD_DIR)/utils/c/lm_setp.d \
./$(BUILD_DIR)/utils/c/lm_setdm.d \
./$(BUILD_DIR)/utils/c/lm_setrm.d \
./$(BUILD_DIR)/utils/c/util.d

# Add the python utilities to the build list.
ifeq ($(USE_PYTHON),1)
UTIL_BINS += ./$(BUILD_DIR)/bin/lm_python
CPP_DEPS += ./$(BUILD_DIR)/utils/c/lm_setp.d
endif

# Add the SBML utilities to the build list.
ifeq ($(USE_SBML),1)
UTIL_BINS += ./$(BUILD_DIR)/bin/lm_sbml_import
CPP_DEPS += ./$(BUILD_DIR)/utils/c/lm_sbml_import.d
endif

# Each subdirectory must supply rules for building sources it contributes
./$(BUILD_DIR)/utils/c/%.o: ./utils/c/%.cpp 
	@echo 'Building file: $<'
	@echo 'Invoking: $(CXX)'
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(UTIL_INCLUDE_DIRS) $(INCLUDE_DIRS) $(CXXDEPENDFLAGS) -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
