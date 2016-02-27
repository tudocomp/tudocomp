#ifndef _INCLUDED_LZW_COMPRESSOR_HPP_
#define _INCLUDED_LZW_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

#include <tudocomp/lz78/trie.h>
#include <tudocomp/lzw/factor.h>

namespace tudocomp {

namespace lzw {

using ::lz78::Trie;
using ::lz78::Result;
using ::tudocomp::Compressor;
using ::lzw::LzwEntry;

const std::string THRESHOLD_OPTION = "lzw.threshold";
const std::string THRESHOLD_LOG = "lzw.threshold";
const std::string RULESET_SIZE_LOG = "lzw.factor_count";

/**
 * A simple compressor that echoes the input into the coder without
 * generating any factors whatsoever.
 */
template<typename C>
class LzwCompressor: public Compressor {
public:
    using Compressor::Compressor;

    virtual void compress(Input& input, Output& out) override {
        auto guard = input.as_view();
        boost::string_ref input_ref = *guard;

        lz78::Trie trie;

        for (uint32_t i = 0; i <= 0xff; i++) {
            trie.insert(i);
        }

        C coder(*m_env, out);
        uint64_t factor_count = 0;

        for (size_t i = 0; i < input.size(); i++) {
            auto s = input_ref.substr(i);

            lz78::Result phrase_and_size = trie.find_or_insert(s);

            DLOG(INFO) << "looking at " << input_ref.substr(0, i)
                << " | " << s << " -> " << phrase_and_size.size;

            if (phrase_and_size.size > 1) {
                i += phrase_and_size.size - 2;
            } else if (phrase_and_size.size == 1) {
                i += 1;
            }

            LzwEntry e;
            if (phrase_and_size.entry.index != 0) {
                e = phrase_and_size.entry.index - 1;
            } else {
                e = phrase_and_size.entry.chr;
            }

            coder.encode_fact(e);
            factor_count++;
        }
        m_env->log_stat(RULESET_SIZE_LOG, factor_count);
        trie.root.print(0);
    }

    virtual void decompress(Input& in, Output& out) override {
        C::decode(in, out);
    }

};

}

}

#endif
