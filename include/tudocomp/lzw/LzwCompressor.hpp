#ifndef _INCLUDED_LZW_COMPRESSOR_HPP_
#define _INCLUDED_LZW_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

#include <tudocomp/lz78/trie.h>
#include <tudocomp/lzw/factor.h>

#include <tudocomp/lz78/dictionary.hpp>

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
        using lz78_dictionary::CodeType;
        using lz78_dictionary::EncoderDictionary;

        const CodeType dms {std::numeric_limits<CodeType>::max()};
        const CodeType reserve_dms {0};

        auto guard = input.as_stream();
        auto& is = *guard;

        EncoderDictionary ed(EncoderDictionary::Lzw, dms, reserve_dms);
        C coder(*m_env, out);
        uint64_t factor_count = 0;

        CodeType i {dms}; // Index
        char c;
        bool rbwf {false}; // Reset Bit Width Flag

        while (is.get(c)) {
            // dictionary's maximum size was reached
            if (ed.size() == dms)
            {
                ed.reset();
                rbwf = true;
            }

            const CodeType temp {i};

            if ((i = ed.search_and_insert(temp, c)) == dms)
            {
                coder.encode_fact(temp);
                factor_count++;
                i = ed.search_initials(c);
            }

            if (rbwf)
            {
                coder.dictionary_reset();
                rbwf = false;
            }
        }
        if (i != dms) {
            coder.encode_fact(i);
            factor_count++;
        }
        m_env->log_stat(RULESET_SIZE_LOG, factor_count);
    }

    virtual void decompress(Input& in, Output& out) override final {
        C::decode(in, out);
    }

};

}

}

#endif
