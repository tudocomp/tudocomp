#ifndef _INCLUDED_LZ78_DECODE_BUFFER_HPP_
#define _INCLUDED_LZ78_DECODE_BUFFER_HPP_

#include <tudocomp/compressors/lz78/LZ78Factor.hpp>

namespace tdc {
namespace lz78 {

/// Helper class for decoding a stream of Lz78 rules
class DecodeBuffer {
private:
    std::vector<Factor> dict;
    std::vector<uliteral_t> dict_walk_buf;

public:
    inline void decode(Factor entry, std::ostream& out) {
        dict.push_back(entry);

        len_t index = entry.index;
        uliteral_t chr = entry.chr;

        dict_walk_buf.clear();

        while (index != 0) {
            dict_walk_buf.push_back(chr);
            chr = Factor(dict[index - 1]).chr;
            index = Factor(dict[index - 1]).index;
        }

        out << chr;
        for (size_t i = dict_walk_buf.size(); i > 0; i--) {
            out << dict_walk_buf[i - 1];
        }
    }
};

}} //ns

#endif
