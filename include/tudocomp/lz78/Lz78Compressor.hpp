#ifndef _INCLUDED_LZ78_COMPRESSOR_HPP_
#define _INCLUDED_LZ78_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

#include <tudocomp/lz78/dictionary.hpp>

#include <tudocomp/lz78/Factor.hpp>
#include <tudocomp/lz78/Lz78BitCoder.hpp>

namespace tudocomp {

namespace lz78 {

using tudocomp::lz78::Factor;
using ::tudocomp::Compressor;
using lz78_dictionary::CodeType;
using lz78_dictionary::EncoderDictionary;
using lz78_dictionary::DMS_MAX;

inline CodeType select_size(Env& env, string_ref name) {
    auto& o = env.option(name);
    if (o.as_string() == "inf") {
        return DMS_MAX;
    } else {
        return o.as_integer();
    }
}

template <typename C>
class Lz78Compressor: public Compressor {
private:
    /// Max dictionary size before reset
    const CodeType dms {DMS_MAX};
    //const CodeType dms {256 + 10};
    /// Preallocated dictionary size,
    /// picked because (sizeof(CodeType) = 4) * 1024 == 4096,
    /// which is the default page size on linux
    const CodeType reserve_dms {1024};

public:
    inline Lz78Compressor(Env&& env):
        Compressor(std::move(env)),
        dms(select_size(this->env(), "dict_size"))
    {}

    inline static Meta meta() {
        Meta m("compressor", "lz78",
               "Lempel-Ziv 78\n\n"
               "`dict_size` has to either be \"inf\", or a positive integer,\n"
               "and determines the maximum size of the backing storage of\n"
               "the dictionary before it gets reset.");
        m.option("coder").templated<C, Lz78BitCoder>();
        m.option("dict_size").dynamic("inf");
        return m;
    }

    virtual void compress(Input& input, Output& out) override {
        auto is = input.as_stream();

        // Stats
        env().begin_stat_phase("Lz78 compression");
        uint64_t stat_dictionary_resets = 0;
        uint64_t stat_dict_counter_at_last_reset = 0;
        uint64_t stat_factor_count = 0;

        EncoderDictionary ed(EncoderDictionary::Lz78, dms, reserve_dms);
        C coder(env().env_for_option("coder"), out);

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
                stat_dictionary_resets++;
                stat_dict_counter_at_last_reset = dms;
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
                stat_factor_count++;
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
            stat_factor_count++;
        }

        env().log_stat("factor_count", stat_factor_count);
        env().log_stat("dictionary_reset_counter",
                       stat_dictionary_resets);
        env().log_stat("max_factor_counter",
                       stat_dict_counter_at_last_reset);
        env().end_stat_phase();
    }

    virtual void decompress(Input& in, Output& out) override final {
        C::decode(in, out);
    }

};

}

}

#endif
