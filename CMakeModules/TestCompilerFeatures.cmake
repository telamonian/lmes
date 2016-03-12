# a set of functions to test if the fully configured compiler supports various features
#
# All the functions named test<some-feature> take two arguments, varName and required
# The result of the test will be stored at the parent scope in a variable named varName
# If required is yes, set varName to true.
# If required is no, set varName to false.
# If required is optional, directly check if the compiler can build avx code and set varName to the test result

INCLUDE(CheckCXXSourceCompiles)

########################
# test functions
########################

# test if the compiler supports avx
function(testAVX varName required)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/TestCompilerSnippets/avxTestSnippet.cpp avxTestSnippet)
    featureTest(${varName} ${required} AVX "${avxTestSnippet}")
    set(${varName} ${${varName}} PARENT_SCOPE)
endfunction()

# test if the compiler supports fma
function(testFMA varName required)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/TestCompilerSnippets/fmaTestSnippet.cpp fmaTestSnippet)
    featureTest(${varName} ${required} FMA "${fmaTestSnippet}")
    set(${varName} ${${varName}} PARENT_SCOPE)
endfunction()

# test if the compiler supports svml. currently this is just a stub that tests if the compiler is ICC
function(testSVML varName required)
    set(featureName SVML)

    if(required STREQUAL yes)
        message(STATUS "${featureName} support required by user option, setting ${varName} to true")
        set(${varName} true)

    elseif(required STREQUAL no)
        message(STATUS "${featureName} support disabled by user option, setting ${varName} to false")
        set(${varName} false)

    else(required STREQUAL yes)
        setTestCompileFlags()

        if(${CMAKE_CXX_COMPILER_ID} MATCHES "icc")
            set(${varName} true)
        else(${CMAKE_CXX_COMPILER_ID} MATCHES "icc")
            set(${varName} false)
        endif(${CMAKE_CXX_COMPILER_ID} MATCHES "icc")

        if(${varName})
            message(STATUS "${featureName} is supported by the compiler, setting ${varName} to true")
        else(${varName})
            message(STATUS "${featureName} is not supported by the compiler, setting ${varName} to false")
        endif(${varName})

    endif(required STREQUAL yes)
    set(${varName} ${${varName}} PARENT_SCOPE)
endfunction()

#################################
# the remaining functions are meant to be internal to the module
#################################
function(featureTest varName required featureName testSnippet)
    if(required STREQUAL yes)
        message(STATUS "${featureName} support required by user option, setting ${varName} to true")
        set(${varName} true)

    elseif(required STREQUAL no)
        message(STATUS "${featureName} support disabled by user option, setting ${varName} to false")
        set(${varName} false)

    else(required STREQUAL yes)
        setTestCompileFlags()

        CHECK_CXX_SOURCE_COMPILES("${testSnippet}" ${varName})

        if(${varName})
            message(STATUS "${featureName} is supported by the compiler, setting ${varName} to true")
        else(${varName})
            message(STATUS "${featureName} is not supported by the compiler, setting ${varName} to false")
        endif(${varName})

    endif(required STREQUAL yes)
    set(${varName} ${${varName}} PARENT_SCOPE)
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