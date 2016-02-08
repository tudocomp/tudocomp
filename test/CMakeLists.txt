include(test_driver.cmake)

run_test(tudocomp_tests tudocomp)
run_test(esacomprule_tests esacomprule)
run_test(lz78rule_tests lz78rule)

# Compile and run the "dummy_tests.cpp" file, which
# has a dependency on the "dummy" subproject.
run_test(dummy_tests dummy)

run_test(sa_compressor_tests suffix_array_compressor)
run_test(esacomp_tests esacomp)
run_test(lz_compressor_tests esacomp lz_compressor)
run_test(tudocomp_driver_tests tudocomp tudocomp_algorithms)
add_dependencies(tudocomp_driver_tests tudocomp_driver)
