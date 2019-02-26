add_executable(
    bit_vector_example

    bit_vector.cpp
)

target_link_libraries(
    bit_vector_example

    tudocomp
    bit_span
)

cotire(bit_vector_example)

add_dependencies(examples bit_vector_example)

