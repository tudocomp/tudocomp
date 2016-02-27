#ifndef _INCLUDED_LZ78_DECODE_BUFFER_HPP_
#define _INCLUDED_LZ78_DECODE_BUFFER_HPP_

#include <tudocomp/lz78/factors.h>

namespace tudocomp {

namespace lz78 {

// TODO: Adjust namespace nesting
using ::lz78::Entries;
using ::lz78::Entry;

/// Helper class for decoding a stream of Lz78 rules
class Lz78DecodeBuffer {
private:
    Entries dict;
    std::vector<uint8_t> dict_walk_buf;

public:
    inline void decode(Entry entry, std::ostream& out) {
        dict.push_back(entry);

        size_t index = entry.index;
        uint8_t chr = entry.chr;

        dict_walk_buf.clear();

        while (index != 0) {
            dict_walk_buf.push_back(chr);
            chr = Entry(dict[index - 1]).chr;
            index = Entry(dict[index - 1]).index;
        }

        out << chr;
        for (size_t i = dict_walk_buf.size(); i > 0; i--) {
            out << dict_walk_buf[i - 1];
        }
    }
};

}

}

#endif
