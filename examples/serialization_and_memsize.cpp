#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cstdint>

#include <tudocomp/util/compact_hash/map/typedefs.hpp>
#include <tudocomp/util/serialization.hpp>
#include <tudocomp/util/heap_size.hpp>

template<typename val_t>
using map_type = tdc::compact_hash::map::sparse_elias_hashmap_t<val_t>;

int main() {
    // creates a hash table with default capacity and initial bit widths
    auto map = map_type<int>();
    for(int i = 0; i < 1000; ++i) {
        auto key = i;
        auto val = i*i + 42;

        map.insert_kv_width(key, std::move(val), tdc::bits_for(key), tdc::bits_for(val));
    }

    std::cout << "elements in map: " << map.size() << std::endl;
    std::cout << "key width: " << map.key_width() << " bits" << std::endl;
    std::cout << "value width: " << map.value_width() << " bits" << std::endl;

    // this could just be an `ofstream` for outputting to a file.
    std::stringstream output_stream;

    // compute size of the datastructure
    auto heap_object_size = tdc::heap_size_compute(map);

    // serialize the datastructure
    auto written_object_size = tdc::serialize_write(output_stream, map);

    std::cout << "total heap size of initial map: " << heap_object_size.size_in_bytes() << std::endl;
    std::cout << "serialized bytes: " << written_object_size.size_in_bytes() << std::endl;

    auto deserialized_map = tdc::serialize_read<map_type<int>>(output_stream);
    auto heap_object_size2 = tdc::heap_size_compute(deserialized_map);

    std::cout << "total heap size of deserialized map: " << heap_object_size2.size_in_bytes() << std::endl;
}
