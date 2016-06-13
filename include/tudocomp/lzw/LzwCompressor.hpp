#ifndef _INCLUDED_LZW_COMPRESSOR_HPP_
#define _INCLUDED_LZW_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

#include <tudocomp/lzw/Factor.hpp>

#include <tudocomp/lz78/dictionary.hpp>

namespace tudocomp {

namespace lzw {

using ::tudocomp::Compressor;
using lz78_dictionary::CodeType;
using lz78_dictionary::EncoderDictionary;
using lz78_dictionary::DMS_MAX;

const std::string THRESHOLD_OPTION = "lzw.threshold";
const std::string THRESHOLD_LOG = "lzw.threshold";
const std::string RULESET_SIZE_LOG = "lzw.factor_count";

/**
 * A simple compressor that echoes the input into the coder without
 * generating any factors whatsoever.
 */
template<typename C>
class LzwCompressor: public Compressor {
private:
    /// Max dictionary size before reset
    const CodeType dms {DMS_MAX};
    //const CodeType dms {256 + 10};
    /// Preallocated dictionary size
    const CodeType reserve_dms {1024};
public:
    using Compressor::Compressor;

    virtual void compress(Input& input, Output& out) override {
        auto guard = input.as_stream();
        auto& is = *guard;

        EncoderDictionary ed(EncoderDictionary::Lzw, dms, reserve_dms);
        C coder(*m_env, out);
        uint64_t factor_count = 0;

        CodeType i {dms}; // Index
        char c;
        bool rbwf {false}; // Reset Bit Width Flag

        while (is.get(c)) {
            uint8_t b = c;

            // dictionary's maximum size was reached
            if (ed.size() == dms)
            {
                ed.reset();
                rbwf = true;
            }

            const CodeType temp {i};

            if ((i = ed.search_and_insert(temp, b)) == dms)
            {
                coder.encode_fact(temp);
                factor_count++;
                i = b;
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
        //TODO update m_env->log_stat(RULESET_SIZE_LOG, factor_count);
    }

    virtual void decompress(Input& in, Output& out) override final {
        C::decode(in, out, dms, reserve_dms);
    }

};

}

}

#endif
