include(test_driver.cmake)

list(APPEND BASIC_DEPS malloc_count tudocomp)

run_test(vbyte_test     DEPS ${BASIC_DEPS})
run_test(rle_test       DEPS ${BASIC_DEPS})
run_test(mtf_test       DEPS ${BASIC_DEPS})
run_test(huff_test      DEPS ${BASIC_DEPS})

run_test(tudocomp_tests DEPS ${BASIC_DEPS})
run_test(ds_tests       DEPS ${BASIC_DEPS})
run_test(generic_int_vector_tests DEPS ${BASIC_DEPS})

#run_bench(int_vector_benchs DEPS ${BASIC_DEPS})

run_test(compressor_adapter_tests
    DEPS tudocomp_algorithms ${BASIC_DEPS})

#run_test(example_tests  DEPS ${BASIC_DEPS})


set(SANDBOX_CPP ${CMAKE_CURRENT_SOURCE_DIR}/sandbox_tests.cpp)
if(NOT EXISTS ${SANDBOX_CPP})
    file(READ ${CMAKE_SOURCE_DIR}/etc/sandbox_tests.cpp.template SANDBOX_TEMPLATE)
    file(WRITE ${SANDBOX_CPP} "${SANDBOX_TEMPLATE}")
endif()

run_test(sandbox_tests  DEPS ${BASIC_DEPS})

run_test(tudocomp_driver_tests
    DEPS     tudocomp_algorithms
    BIN_DEPS tudocomp_driver)
run_test(matrix_tests
    DEPS     tudocomp_algorithms
    BIN_DEPS tudocomp_driver)
