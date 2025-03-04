cmake_minimum_required(VERSION 3.15)

project(
    StackWalker
    VERSION 1.0.0
    DESCRIPTION "Walking the callstack in Windows applications"
    HOMEPAGE_URL "https://github.com/JochenKalmbach/StackWalker"
    LANGUAGES CXX
)

# get whether we are using multi-config generator or not
get_property(StackWalker_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(StackWalker_MULTI_CONFIG)
    message(STATUS "Build config: Multi")
else()
    # only works for single-config generators like Ninja
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
    endif()
    message(STATUS "Build config: ${CMAKE_BUILD_TYPE}")
endif()

option(StackWalker_DISABLE_TESTS "Disable tests" OFF)

# set flat output structure (with per-config subdirectories for multi-config)
set(ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

###############################
# Check compiler's capabilities
###############################

include(CheckCXXCompilerFlag)

# note: Clang not supported yet
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_COMPILER_IS_CLANG TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_COMPILER_IS_MSVC TRUE)
endif()

# print compiler
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
# if available, print version string (usually available)
if(DEFINED CMAKE_CXX_COMPILER_VERSION)
    message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
endif()

if(CMAKE_COMPILER_IS_MSVC)
    # MSVC_TOOLSET_VERSION is only available from cmake 3.12
    # --> https://cmake.org/cmake/help/v3.12/variable/MSVC_TOOLSET_VERSION.html
    # --> https://gitlab.kitware.com/KindDragon/cmake/commit/f2a61e0a4790d4d52a2412c8017be2b92e9af26f?view=inline
    if(NOT DEFINED MSVC_TOOLSET_VERSION)
        if(MSVC_VERSION GREATER_EQUAL 1920)
            set(MSVC_TOOLSET_VERSION "") # leave unknown
        elseif(MSVC_VERSION GREATER_EQUAL 1910)
            # VS 2017
            set(MSVC_TOOLSET_VERSION 141)
        elseif(MSVC_VERSION EQUAL 1900)
            # VS 2015
            set(MSVC_TOOLSET_VERSION 140)
        elseif(MSVC_VERSION EQUAL 1800)
            # VS 2013
            set(MSVC_TOOLSET_VERSION 120)
        elseif(MSVC_VERSION EQUAL 1700)
            # VS 2012
            set(MSVC_TOOLSET_VERSION 110)
        elseif(MSVC_VERSION EQUAL 1600)
            # VS 2010
            set(MSVC_TOOLSET_VERSION 100)
        elseif(MSVC_VERSION EQUAL 1500)
            # VS 2008
            set(MSVC_TOOLSET_VERSION 90)
        elseif(MSVC_VERSION EQUAL 1400)
            # VS 2005
            set(MSVC_TOOLSET_VERSION 80)
        else()
            # We don't support MSVC_TOOLSET_VERSION for earlier compiler.
            # FATAL_ERROR here since MSVC_TOOLSET_VERSION is used for PDB name
            message(FATAL_ERROR "MSVC version ${MSVC_VERSION} is not supported")
        endif()
    endif()

    # MSVC flags (private to the build)
    add_compile_options(/W4 /Zi /wd4740)
    add_compile_definitions(
        # note: _SECURE_SCL superseded by _ITERATOR_DEBUG_LEVEL
        _SECURE_SCL=0
        _CRT_SECURE_NO_DEPRECATE
        _CRT_NONSTDC_NO_DEPRECATE
        _SCL_SECURE_NO_DEPRECATE
    )
else()
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER_ID} is not supported yet")
endif()

# StackWalker main library (no support for shared build)
set(sw_lib StackWalker)
add_library(${sw_lib} STATIC Main/StackWalker/StackWalker.cpp)
target_include_directories(
    ${sw_lib} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Main/StackWalker>
    $<INSTALL_INTERFACE:include>
)
# provide extra suffices (allows side-by-side Debug/Release install)
set_target_properties(
    ${sw_lib} PROPERTIES
    DEBUG_POSTFIX d
    # note: RelWithDebInfo is a little redundant (already using /Zi)
    RELWITHDEBINFO_POSTFIX rd
    # for static libraries, use COMPILE_PDB_NAME_<CONFIG> not PDB_NAME_<CONFIG>
    COMPILE_PDB_NAME ${sw_lib}
    COMPILE_PDB_NAME_DEBUG ${sw_lib}d
    COMPILE_PDB_NAME_RELWITHDEBINFO ${sw_lib}rd
)
# install rule for StackWalker library (with export)
# note: for CMake 3.15 and above, using relative prefixes enables using
# --prefix with cmake --install to change the install prefix
install(TARGETS ${sw_lib} EXPORT ${PROJECT_NAME}Targets DESTINATION lib)
# install rule for the StackWalker header
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/Main/StackWalker/StackWalker.h"
    DESTINATION include
)
# install rules for PDB files (generator expression for suffices)
set(StackWalker_PDB_GEN "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${sw_lib}")
string(
    APPEND StackWalker_PDB_GEN
    "$<$<CONFIG:Debug>:d>$<$<CONFIG:RelWithDebInfo>:rd>.pdb"
)
# note: install in bin because Visual Studio will look in the directory
# that a DLL/EXE is in (e.g. if StackWalker was a DLL)
install(FILES "${StackWalker_PDB_GEN}" DESTINATION bin)

# build StackWalker test program
set(sw_test StackWalkerTest)
if(StackWalker_DISABLE_TESTS)
    message(STATUS "Skipping ${sw_test} (StackWalker_DISABLE_TESTS is TRUE)")
else()
    enable_testing()
    # StackWalker test/demo program
    add_executable(${sw_test} Main/StackWalker/main.cpp)
    target_link_libraries(${sw_test} PRIVATE ${sw_lib})
    # for CTest
    # note: remove? because the program requires interaction
    add_test(NAME ${sw_test} COMMAND ${sw_test})
endif()

# add any other tests
add_subdirectory(Test)

##########################
# CMake config packaging #
##########################

include(CMakePackageConfigHelpers)

# relative install location for CMake files
set(sw_cmake_install_dir lib/cmake/${PROJECT_NAME})
# configure CMake config script for find_package
set(sw_config_script ${PROJECT_NAME}Config.cmake)
configure_package_config_file(
    CMake/${sw_config_script}.in
    "${CMAKE_CURRENT_BINARY_DIR}/${sw_config_script}"
    # install with other CMake scripts
    INSTALL_DESTINATION ${sw_cmake_install_dir}
)
# CMake version config file
set(sw_version_script ${PROJECT_NAME}ConfigVersion.cmake)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${sw_version_script}"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
# target export install rule (conventional to use project name in this fashion)
install(
    EXPORT ${PROJECT_NAME}Targets
    DESTINATION ${sw_cmake_install_dir}
    NAMESPACE ${PROJECT_NAME}::
)
# install CMake config and version config scripts
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${sw_config_script}"
        "${CMAKE_CURRENT_BINARY_DIR}/${sw_version_script}"
    DESTINATION ${sw_cmake_install_dir}
)
