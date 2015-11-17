#include "tudocomp.h"
#include <vector>
#include <cstdint>
#include <sstream>

namespace tudocomp {

DEFINE_ALGO_REGISTRY(TUDOCOMP_ALGOS, Compressor)
REGISTER_ALGO(TUDOCOMP_ALGOS, Compressor,
              SubCompressor, "asdf", "asdf", "asdf")

DEFINE_ALGO_REGISTRY(LZ77_RULE_COMP_ALGOS, Lz77RuleCompressor)
DEFINE_ALGO_REGISTRY(LZ77_RULE_CODE_ALGOS, Lz77RuleCoder)

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
