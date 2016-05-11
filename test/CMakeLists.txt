include(test_driver.cmake)

run_test(tudocomp_tests tudocomp)
run_test(lz78_tests tudocomp)
run_test(lzw_tests tudocomp)

run_test(lzss_tests tudocomp)

set(SANDBOX_CPP "${CMAKE_CURRENT_SOURCE_DIR}/sandbox_tests.cpp")
if(NOT EXISTS ${SANDBOX_CPP})
    file(WRITE ${SANDBOX_CPP} "#include <gtest/gtest.h>\n\n/* implement your quick tests here! */")
endif()

run_test(sandbox_tests tudocomp)

run_test(tudocomp_driver_tests tudocomp_algorithms)
    add_dependencies(tudocomp_driver_tests tudocomp_driver)

run_test(tudocomp_driver_algorithm_matrix_tests tudocomp_algorithms)
    add_dependencies(tudocomp_driver_algorithm_matrix_tests tudocomp_driver)
