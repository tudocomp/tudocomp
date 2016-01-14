include(test_driver.cmake)

run_test(tudocomp_tests tudocomp)
run_test(lz77rule_tests lz77rule)
run_test(lz78rule_tests lz78rule)

# Compile and run the "dummy_tests.cpp" file, which
# has a dependency on the "dummy" subproject.
run_test(dummy_tests dummy)

run_test(sa_compressor_tests suffix_array_compressor)
run_test(esacomp_tests esacomp)
run_test(lz_compressor_tests esacomp lz_compressor)
run_test(tudocomp_driver_tests tudocomp tudocomp_driver_registry)
add_dependencies(tudocomp_driver_tests tudocomp_driver)

#macro(add_test_exec_dep test_target exec_target)
###set( FAKE ${exec_target}_path.h )
##add_custom_command(PRE_BUILD
##                   OUTPUT cmake_${exec_target}_path.h
##                   DEPENDS ${exec_target}
##                   COMMAND echo const std::string cmake_${exec_target}_path = "$<TARGET_FILE:${exec_target}>" > ${exec_target}_path.h)
##add_library(cmake_${exec_target}_path INTERFACE)
##add_dependencies(cmake_${exec_target}_path DEPENDS ${exec_target}_path.h)
###target_sources(cmake_${exec_target}_path INTERFACE ${exec_target}_path.h)
##target_link_libraries(${test_target} cmake_${exec_target}_path)
#    add_custom_target(
#        cmake_touch_${test_target}
#        COMMAND touch ${CMAKE_SOURCE_DIR}/test/${test_target}.cpp
#        COMMAND echo ${CMAKE_SOURCE_DIR}/test/${test_target}.cpp
#    )
#    add_dependencies(${exec_target} cmake_touch_${test_target})
#endmacro()

#add_test_exec_dep(tudocomp_driver_tests tudocomp_driver)
#add_test_exec_dep(tudocomp_driver_tests tudocomp_tests)
