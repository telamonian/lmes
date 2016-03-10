# test if the compiler supports avx

INCLUDE(CheckCXXSourceCompiles)

# If required is yes, set varName to true.
# If required is no, set varName to false.
# If required is optional, directly check if the compiler can build avx code and set varName to the test result
function(TestAVX varName required)
    if(required STREQUAL yes)
        MESSAGE(STATUS "AVX support required by build, setting ${varName} to true")
        SET(${varName} true)

    elseif(required STREQUAL no)
        MESSAGE(STATUS "AVX support disabled by build, setting ${varName} to false")
        SET(${varName} false)

    else(required STREQUAL yes)
        setTestCompileFlags()

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

        if(${varName})
            MESSAGE(STATUS "AVX intrinsics are supported by the compiler, setting ${varName} to true")
        else(${varName})
            MESSAGE(STATUS "AVX intrinsics are not supported by the compiler, setting ${varName} to false")
        endif(${varName})

    endif(required STREQUAL yes)
    SET(${varName} ${${varName}} PARENT_SCOPE)
endfunction()

function(setTestCompileFlags)
    # convert the build type string to lower case for easy comparison
    string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
    # set some flags for the test compilation
    if(NOT CMAKE_BUILD_TYPE_LOWER)
        set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS} PARENT_SCOPE)
    elseif(CMAKE_BUILD_TYPE_LOWER STREQUAL debug)
        set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS_DEBUG} PARENT_SCOPE)
    elseif(CMAKE_BUILD_TYPE_LOWER STREQUAL release)
        set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS_RELEASE} PARENT_SCOPE)
    elseif(CMAKE_BUILD_TYPE_LOWER STREQUAL relwithdebinfo)
        set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} PARENT_SCOPE)
    elseif(CMAKE_BUILD_TYPE_LOWER STREQUAL minsizerel)
        set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS_MINSIZEREL} PARENT_SCOPE)
    else(NOT CMAKE_BUILD_TYPE_LOWER)
        message(WARNING "No AVX testing support for specified build type: ${CMAKE_BUILD_TYPE}. Test results may be incorrect.")
    endif(NOT CMAKE_BUILD_TYPE_LOWER)
endfunction()