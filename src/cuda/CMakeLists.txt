# cuda src directory

#get the source files
file(GLOB lm_cuda_source lm/*.cu)
message(${lm_cuda_source}_bar)
file(GLOB lm_cuda_source1 lm/*/*.cu)
list(APPEND lm_cuda_source ${lm_cuda_source1})
message(${lm_cuda_source}_foo)