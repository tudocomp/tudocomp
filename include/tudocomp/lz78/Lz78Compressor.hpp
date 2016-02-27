#ifndef _INCLUDED_LZ78_COMPRESSOR_HPP_
#define _INCLUDED_LZ78_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

#include <tudocomp/lz78/trie.h>

namespace tudocomp {

namespace lz78 {

using ::lz78::Trie;
using ::lz78::Result;
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
        auto guard = input.as_view();
        auto input_ref = *guard;

        Trie trie;
        size_t factor_counter = 0;
        C coder(*m_env, out);

        for (size_t i = 0; i < input_ref.size(); i++) {
            auto s = input_ref.substr(i);

            Result phrase_and_size = trie.find_or_insert(s);

            DLOG(INFO) << "looking at " << input_ref.substr(0, i)
                << "|" << s << " -> " << phrase_and_size.size;

            i += phrase_and_size.size - 1;

            coder.encode_fact(phrase_and_size.entry);

        }

        m_env->log_stat(RULESET_SIZE_LOG, factor_counter);

        trie.root.print(0);
    }

    virtual void decompress(Input& in, Output& out) override {
        C::decode(in, out);
    }

};

}

}

#endif
