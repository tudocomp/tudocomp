#ifndef _INCLUDED_LZ78_COMPRESSOR_HPP_
#define _INCLUDED_LZ78_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

#include <tudocomp/lz78/trie.h>

namespace tudocomp {

namespace lz78 {

using ::lz78::Trie;
using ::lz78::Result;
using ::lz78::PrefixBuffer;
using ::tudocomp::Compressor;

const std::string THRESHOLD_OPTION = "lz78.threshold";
const std::string THRESHOLD_LOG = "lz78.threshold";
const std::string RULESET_SIZE_LOG = "lz78.factor_count";

/**
 * A simple compressor that echoes the input into the coder without
 * generating any factors whatsoever.
 */
template <typename C>
class Lz78Compressor: public Compressor {
public:
    using Compressor::Compressor;

    virtual void compress(Input& input, Output& out) override {
        auto guard = input.as_stream();
        PrefixBuffer buf(*guard);

        Trie trie(Trie::Lz78);
        size_t factor_counter = 0;
        C coder(*m_env, out);

        while (!buf.is_empty()) {
            Result phrase_and_size = trie.find_or_insert(buf);

            coder.encode_fact(phrase_and_size.entry);

            factor_counter++;
        }

        m_env->log_stat(RULESET_SIZE_LOG, factor_counter);

        trie.print(0);
    }

    virtual void decompress(Input& in, Output& out) override final {
        C::decode(in, out);
    }

};

}

}

#endif
