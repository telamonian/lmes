
# Add inputs and outputs from these tool invocations to the build variables 
INSTALL_LIBS += \
./$(BUILD_DIR)/lib/lm.py

SWIG_GEN_SRC += \
./$(BUILD_DIR)/src/swig/lm_wrap.cpp \
./$(BUILD_DIR)/lib/lm.py

OBJS += \
./$(BUILD_DIR)/src/swig/lm_wrap.o 

CPP_DEPS += \
./$(BUILD_DIR)/src/swig/lm_wrap.d 


# Each subdirectory must supply rules for building sources it contributes
./$(BUILD_DIR)/src/swig/%_wrap.cpp: ./src/swig/%.i
	@echo 'Building file: $<'
	@echo 'Invoking: $(PYTHON_SWIG)'
	mkdir -p $(@D)
	mkdir -p ./$(BUILD_DIR)/lib
	$(PYTHON_SWIG) -c++ -python -o "$@" -outdir ./$(BUILD_DIR)/lib "$<"
	@echo 'Finished building: $<'
	@echo ' '

./$(BUILD_DIR)/src/swig/%.o: ./$(BUILD_DIR)/src/swig/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: $(CXX)'
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) $(CXXDEPENDFLAGS) -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
