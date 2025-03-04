cmake_minimum_required(VERSION ${CMAKE_MINIMUM_REQUIRED_VERSION})

# location of test install
set(_test_install_dir ${PROJECT_BINARY_DIR}/test_install)

# pre-install cleanup test fixture
# use newer rm -rf instead
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.17)
    add_test(
        NAME StackWalkerPrepareInstall
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${_test_install_dir}
    )
# use old remove_directory
else()
    add_test(
        NAME StackWalkerPrepareInstall
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${_test_install_dir}
    )
endif()
# sets up test_install_clean fixture
set_tests_properties(
    StackWalkerPrepareInstall PROPERTIES
    FIXTURES_SETUP test_install_clean
)

# test installation rules
if(StackWalker_MULTI_CONFIG)
    add_test(
        NAME StackWalkerInstallTest
        COMMAND ${CMAKE_COMMAND}
                --install ${PROJECT_BINARY_DIR} --prefix ${_test_install_dir}
                --config $<CONFIG>
    )
else()
    add_test(
        NAME StackWalkerInstallTest
        COMMAND ${CMAKE_COMMAND}
                --install ${PROJECT_BINARY_DIR} --prefix ${_test_install_dir}
    )
endif()
# require cleaned install directory
set_tests_properties(
    StackWalkerInstallTest PROPERTIES
    FIXTURES_REQUIRED test_install_clean
    FIXTURES_SETUP test_install
)

# location of external usage test build
set(_test_usage_dir ${PROJECT_BINARY_DIR}/test_cmake_usage)

# pre-build cleanup test fixture
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.17)
    add_test(
        NAME StackWalkerPrepareCMakeUsage
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${_test_usage_dir}
    )
else()
    add_test(
        NAME StackWalkerPrepareCMakeUsage
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${_test_usage_dir}
    )
endif()
# note: does not require test_install_clean fixture
set_tests_properties(
    StackWalkerPrepareCMakeUsage PROPERTIES
    FIXTURES_SETUP test_cmake_usage_clean
)

# test CMake config, e.g. locating the library and configuring a build
# note: since CMake 3.12, better to use StackWalker_ROOT
# note: -S, -B only available for CMake >= 3.13
add_test(
    NAME StackWalkerCMakeConfigTest
    COMMAND ${CMAKE_COMMAND}
            -S ${CMAKE_CURRENT_SOURCE_DIR}/StackWalkerCMakeTest
            -B ${_test_usage_dir}
            -DStackWalker_ROOT=${_test_install_dir}
            -DStackWalker_FIND_VERSION=${PROJECT_VERSION}
)
# need both install and clean config
set_tests_properties(
    StackWalkerCMakeConfigTest PROPERTIES
    FIXTURES_REQUIRED "test_install;test_cmake_usage_clean"
    FIXTURES_SETUP test_cmake_config
)

# test building with an external project
if(StackWalker_MULTI_CONFIG)
    add_test(
        NAME StackWalkerCMakeBuildTest
        COMMAND ${CMAKE_COMMAND} --build ${_test_usage_dir} --config $<CONFIG> -j
    )
else()
    add_test(
        NAME StackWalkerCMakeBuildTest
        COMMAND ${CMAKE_COMMAND} --build ${_test_usage_dir} -j
    )
endif()
# config must have succeeded
set_tests_properties(
    StackWalkerCMakeBuildTest PROPERTIES
    FIXTURES_REQUIRED test_cmake_config
    FIXTURES_SETUP test_cmake_build
)

# test running the external program
# note: --test-dir only available in 3.20
if(StackWalker_MULTI_CONFIG)
    add_test(
        NAME StackWalkerCMakeCTestTest
        COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG>
        WORKING_DIRECTORY ${_test_usage_dir}
    )
else()
    add_test(
        NAME StackWalkerCMakeCTestTest
        COMMAND ${CMAKE_CTEST_COMMAND}
        WORKING_DIRECTORY ${_test_usage_dir}
    )
endif()
# build must have succeeded
set_tests_properties(
    StackWalkerCMakeCTestTest PROPERTIES
    FIXTURES_REQUIRED test_cmake_build
    FIXTURES_SETUP test_cmake_ctest
)
