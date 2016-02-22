# test if the compiler supports avx

INCLUDE(CheckCXXSourceCompiles)

function(TestAVX varName)
    CHECK_CXX_SOURCE_COMPILES(
       "#include <immintrin.h>
        #include <stdio.h>

        int main()
        {
            float a = 16.0f;
            float b = 9.0f;

            __m256 AVX0 = _mm256_setzero_ps();
            __m256 AVXa = _mm256_set1_ps(a);
            __m256 AVXb = _mm256_set1_ps(b);
            __m256 AVXv = _mm256_add_ps(AVXa, AVXb);
            
            float temp[8] __attribute((aligned(32)));
            _mm256_store_ps(&temp[0], AVXv);
            printf(\"tempavx is %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\",
                temp[0], temp[1], temp[2], temp[3],
                temp[4], temp[5], temp[6], temp[7]);
        
            return 0;
        }" ${varName})

    SET(${varName} ${${varName}}${${varName}} PARENT_SCOPE)
    if(${varName})
        MESSAGE(STATUS "AVX intrinsics are supported by the compiler")
    else(${varName})
        MESSAGE(STATUS "AVX intrinsics are not supported by the compiler")
    endif(${varName})
endfunction()