include(test/testsuite.cmake)

list(APPEND BASIC_DEPS tudocomp_stat tudocomp)

run_doc_snippet(io                  DEPS ${BASIC_DEPS})
run_doc_snippet(bit_io              DEPS ${BASIC_DEPS})
run_doc_snippet(int_vector          DEPS ${BASIC_DEPS})
run_doc_snippet(algorithm_impl      DEPS ${BASIC_DEPS})
run_doc_snippet(coder_impl          DEPS ${BASIC_DEPS})
run_doc_snippet(compressor_impl     DEPS ${BASIC_DEPS})
run_doc_snippet(generator_impl      DEPS ${BASIC_DEPS})
run_doc_snippet(stats               DEPS ${BASIC_DEPS})

run_test(vbyte_test     DEPS ${BASIC_DEPS})
run_test(rle_test       DEPS ${BASIC_DEPS})
run_test(mtf_test       DEPS ${BASIC_DEPS})
run_test(huff_test      DEPS ${BASIC_DEPS})
run_test(arithm_tests   DEPS ${BASIC_DEPS})
run_test(coder_tests    DEPS ${BASIC_DEPS})
run_test(cedar_tests    DEPS ${BASIC_DEPS})
run_test(lz78u_tests    DEPS ${BASIC_DEPS})
run_test(st_tests       DEPS ${BASIC_DEPS})
run_test(maxlcp_tests   DEPS ${BASIC_DEPS})
run_test(plcpcomp_tests DEPS ${BASIC_DEPS})

run_test(lz78_trie_tests DEPS ${BASIC_DEPS})

run_test(lzss_test      DEPS ${BASIC_DEPS})

run_test(tudocomp_tests DEPS ${BASIC_DEPS})
run_test(input_output_tests DEPS ${BASIC_DEPS})
run_test(ds_tests       DEPS ${BASIC_DEPS})
run_test(lcpsada_tests  DEPS ${BASIC_DEPS})
run_test(generic_int_vector_tests DEPS ${BASIC_DEPS})
run_test(esp_tests      DEPS ${BASIC_DEPS})

#Disabled due to breakage on this branch:
#run_test(paper_tests    DEPS ${BASIC_DEPS})
#run_bench(int_vector_benchs DEPS ${BASIC_DEPS})
#run_test(compressor_adapter_tests DEPS tudocomp_algorithms ${BASIC_DEPS})
#run_test(example_tests  DEPS ${BASIC_DEPS})

run_test(lfs_tests     DEPS ${BASIC_DEPS})
run_test(lfs2_tests     DEPS ${BASIC_DEPS})



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
