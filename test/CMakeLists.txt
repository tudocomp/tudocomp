include(test_driver.cmake)

run_test(tudocomp_tests tudocomp)
run_test(tudocomp_driver_tests tudocomp_algorithms)
    add_dependencies(tudocomp_driver_tests tudocomp_driver)

#run_test(esacomp_tests esacomp)
#run_test(esacomp_impl_tests esacomp_impl)
#run_test(esacomp_dummy_tests esacomp_dummy)
#run_test(esacomp_suffix_array_compressor_tests esacomp_suffix_array_compressor)
#run_test(esacomp_lz_factor_tests esacomp_lz_factor)

run_test(lz78_tests tudocomp)
run_test(lz78_impl_tests tudocomp)

run_test(lzw_impl_tests tudocomp)

run_test(dev_tests tudocomp)
