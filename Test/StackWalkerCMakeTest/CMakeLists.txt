cmake_minimum_required(VERSION 3.15)

project(
    StackWalkerCMakeTest
    DESCRIPTION "StackWalker CMake usage test"
    LANGUAGES CXX
)

##
# Helper function to check if a variable is defined.
#
function(check_var var)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "${var} is not defined")
    endif()
endfunction()

check_var(StackWalker_FIND_VERSION)

# find StackWalker
#
# consumed variables:
#
#   StackWalker_FIND_VERSION
#
find_package(StackWalker ${StackWalker_FIND_VERSION} REQUIRED)

# smoke test that expected variables exist
check_var(StackWalker_FOUND)
check_var(StackWalker_VERSION)

# StackWalker_FOUND must be TRUE
if(NOT StackWalker_FOUND)
    message(FATAL_ERROR "StackWalker_FOUND is unexpectedly FALSE")
endif()
# exported target must be available
if(NOT TARGET StackWalker::StackWalker)
    message(FATAL_ERROR "Expected target StackWalker::StackWalker missing")
endif()

message(STATUS "StackWalker version: ${StackWalker_VERSION}")

# compile options
if(MSVC)
    add_compile_options(/Wall /wd4710 /wd4711)
endif()

# StackWalkerDemo: demo program to test external StackWalker usage
add_executable(StackWalkerDemo StackWalkerDemo.cpp)
target_link_libraries(StackWalkerDemo PRIVATE StackWalker::StackWalker)
# reduce Windows.h include size
target_compile_definitions(StackWalkerDemo PRIVATE WIN32_MEAN_AND_LEAN)

include(CTest)

add_test(NAME StackWalkerDemo COMMAND StackWalkerDemo)
