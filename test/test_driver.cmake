# Grab gtest
find_or_download_package(GTest GTEST gtest)

# Custom test target to run the googletest tests
add_custom_target(check)
add_custom_command(
    TARGET check
    POST_BUILD
    COMMENT "All tests were successful!" VERBATIM
)

# will compile and run ${test_target}.cpp
# and add all further arguments as dependencies
macro(run_test test_target)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs DEPS BIN_DEPS)
    cmake_parse_arguments(TEST_TARGET "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    add_executable(${test_target}
        EXCLUDE_FROM_ALL
        test_driver.cpp
        ${test_target}.cpp
    )
    target_link_libraries(${test_target}
        gtest
        glog
        ${TEST_TARGET_DEPS}
    )
    add_custom_command(
        TARGET ${test_target}
        POST_BUILD
        COMMAND ${test_target} #cmake 2.6 required
        DEPEND ${test_target}
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Running ${test_target} ..." VERBATIM
    )
    add_custom_command(
        TARGET check
        PRE_BUILD
        COMMAND make ${test_target} #TODO: Windows?
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Running ${test_target} ..." VERBATIM
    )
    foreach(bin_dep ${TEST_TARGET_BIN_DEPS})
        # TODO: Maybe replace by a custom command that
        # invokes make directly, to ensure all dependcies
        # are actually build
        add_dependencies(${test_target} ${bin_dep})
    endforeach(bin_dep)
endmacro()

