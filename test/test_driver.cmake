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
    add_executable(${test_target}
        EXCLUDE_FROM_ALL
        test_driver.cpp
        ${test_target}.cpp
    )
    target_link_libraries(${test_target}
        gtest
        glog
        ${ARGN}
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
        POST_BUILD
        COMMAND make ${test_target} #TODO: Windows?
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Running ${test_target} ..." VERBATIM
    )
endmacro()

