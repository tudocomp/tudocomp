#ifndef _INCLUDED_LZ78_COMPRESSOR_HPP_
#define _INCLUDED_LZ78_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

#include <tudocomp/lz78/dictionary.hpp>

#include <tudocomp/lz78/Factor.hpp>

namespace tudocomp {

namespace lz78 {

using tudocomp::lz78::Factor;
using ::tudocomp::Compressor;
using lz78_dictionary::CodeType;
using lz78_dictionary::EncoderDictionary;
using lz78_dictionary::DMS_MAX;

const std::string THRESHOLD_OPTION = "lz78.threshold";
const std::string THRESHOLD_LOG = "lz78.threshold";
const std::string RULESET_SIZE_LOG = "lz78.factor_count";

/**
 * A simple compressor that echoes the input into the coder without
 * generating any factors whatsoever.
 */
template <typename C>
class Lz78Compressor: public Compressor {
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

        EncoderDictionary ed(EncoderDictionary::Lz78, dms, reserve_dms);
        C coder(*m_env, out);
        uint64_t factor_count = 0;

        CodeType last_i {dms}; // needed for the end of the string
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

            last_i = i;
            if ((i = ed.search_and_insert(temp, b)) == dms)
            {
                CodeType fact = temp;
                if (fact == dms) {
                    fact = 0;
                }
                coder.encode_fact(Factor { fact, b });
                factor_count++;
                i = dms;
            }

            if (rbwf)
            {
                coder.dictionary_reset();
                rbwf = false;
            }
        }
        if (i != dms) {
            CodeType fact = last_i;
            uint8_t b = c;
            if (fact == dms) {
                fact = 0;
            }
            coder.encode_fact(Factor { fact, b });
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
