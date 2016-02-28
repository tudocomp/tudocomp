#ifndef _INCLUDED_LZW_COMPRESSOR_HPP_
#define _INCLUDED_LZW_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

#include <tudocomp/lz78/trie.h>
#include <tudocomp/lzw/factor.h>

namespace tudocomp {

namespace lzw {

using ::lz78::Trie;
using ::lz78::Result;
using ::lz78::PrefixBuffer;
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
        auto guard = input.as_stream();
        PrefixBuffer buf(*guard);

        lz78::Trie trie(lz78::Trie::Lzw);

        for (uint32_t i = 0; i <= 0xff; i++) {
            trie.insert(i);
        }

        C coder(*m_env, out);
        uint64_t factor_count = 0;

        while (!buf.is_empty()) {
            lz78::Result phrase_and_size = trie.find_or_insert(buf);

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
        trie.print(0);
    }

    virtual void decompress(Input& in, Output& out) override final {
        C::decode(in, out);
    }

};

}

}

#endif
