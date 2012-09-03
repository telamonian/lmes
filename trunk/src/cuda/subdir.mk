
# Add inputs and outputs from these tool invocations to the build variables 
ifeq ($(USE_CUDA),1)

OBJS += \
./$(BUILD_DIR)/src/cuda/lm/Cuda.o \
./$(BUILD_DIR)/src/cuda/lm/rdme/CudaByteLattice.o \
./$(BUILD_DIR)/src/cuda/lm/rdme/MpdRdmeSolver.o \
./$(BUILD_DIR)/src/cuda/lm/rng/XORWow.o

CPP_DEPS += \
./$(BUILD_DIR)/src/cuda/lm/Cuda.d \
./$(BUILD_DIR)/src/cuda/lm/rdme/CudaByteLattice.d \
./$(BUILD_DIR)/src/cuda/lm/rdme/MpdRdmeSolver.d \
./$(BUILD_DIR)/src/cuda/lm/rng/XORWow.d

endif

# Each subdirectory must supply rules for building sources it contributes
./$(BUILD_DIR)/src/cuda/%.o: ./src/cuda/%.cu 
	@echo 'Building file: $<'
	@echo 'Invoking: $(NVCC)'
	mkdir -p $(@D)
	$(CUDA_NVCC) $(CUDA_FLAGS) $(INCLUDE_DIRS) -M "$<" -odir $(@D) -o "$(@:%.o=%.d)" 
	$(CUDA_NVCC) $(CUDA_FLAGS) $(INCLUDE_DIRS) -c "$<" -o "$@" 
ifeq ($(CUDA_GENERATE_PTX_CODE),1)
	$(CUDA_NVCC) $(CUDA_FLAGS) $(INCLUDE_DIRS) -ptx --opencc-options=-LIST:source=on  "$<" -o "$(@:%.o=%.ptx)" 
endif
ifeq ($(CUDA_GENERATE_BIN_CODE),1)
	$(CUDA_NVCC) $(CUDA_FLAGS) $(INCLUDE_DIRS) -cubin "$<" -o "$(@:%.o=%.cubin)" 
endif
ifeq ($(CUDA_GENERATE_ASM_CODE),1)
	/usr/local/decuda/decuda.py -o "$(@:%.o=%.asm)" "$(@:%.o=%.cubin)"
endif
	@echo 'Finished building: $<'
	@echo ' '
