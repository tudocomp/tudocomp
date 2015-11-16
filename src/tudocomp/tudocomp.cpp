#include "tudocomp.h"
#include <vector>
#include <cstdint>
#include <sstream>

namespace tudocomp {

DEFINE_ALGO_REGISTRY(TUDOCOMP_ALGOS, Compressor)
REGISTER_ALGO(TUDOCOMP_ALGOS, Compressor,
              SubCompressor, "asdf", "asdf", "asdf")

void DecodeBuffer::decodeCallback(size_t source) {
    if (pointers.count(source) > 0) {
        std::vector<size_t> list = pointers[source];
        for (size_t target : list) {
            // decode this char
            text.at(target) = text.at(source);
            byte_is_decoded.at(target) = true;

            // decode all chars depending on this location
            decodeCallback(target);
        }

        pointers.erase(source);
    }
}

}
