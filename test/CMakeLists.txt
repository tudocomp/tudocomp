include(test_driver.cmake)

run_test(tudocomp_tests tudocomp)

run_test(lz78_tests tudocomp)
run_test(lzw_tests tudocomp)

run_test(lzss_tests tudocomp)

run_test(tudocomp_driver_tests tudocomp_algorithms)
    add_dependencies(tudocomp_driver_tests tudocomp_driver)

run_test(tudocomp_driver_algorithm_matrix_tests tudocomp_algorithms)
    add_dependencies(tudocomp_driver_tests tudocomp_driver)
