include(test_driver.cmake)

run_test(tudocomp_tests DEPS tudocomp)

run_test(lz78_tests     DEPS tudocomp)
run_test(lzw_tests      DEPS tudocomp)
run_test(lzss_tests     DEPS tudocomp)

set(SANDBOX_CPP ${CMAKE_CURRENT_SOURCE_DIR}/sandbox_tests.cpp)
if(NOT EXISTS ${SANDBOX_CPP})
    file(READ ${CMAKE_SOURCE_DIR}/etc/sandbox_tests.cpp.template SANDBOX_TEMPLATE)
    file(WRITE ${SANDBOX_CPP} "${SANDBOX_TEMPLATE}")
endif()

run_test(sandbox_tests  DEPS tudocomp)

run_test(tudocomp_driver_tests
    DEPS     tudocomp_algorithms
    BIN_DEPS tudocomp_driver)
run_test(tudocomp_driver_algorithm_matrix_tests
    DEPS     tudocomp_algorithms
    BIN_DEPS tudocomp_driver)
