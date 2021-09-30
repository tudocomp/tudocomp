#include <iostream>
#include <vector>

#include <cstdint>
#include <tudocomp/util/compact_hash/map/typedefs.hpp>
#include <algorithm>

template<typename val_t>
using map_type = tdc::compact_hash::map::plain_elias_hashmap_t<val_t>;

int main() {
    // creates a hash table with zero entries, set the bit-width of the keys to four
    auto map = map_type<int>(0, 4);
    for(int i = 0; i <= 15; ++i) { // interval [0..15] can be represented by four bits
        map.insert(i, std::move(i*i)); // insert key i, value i*i
        std::cout << i << " -> " << map[i] << std::endl; // map[i] returns value i*i with key i

    }
}
