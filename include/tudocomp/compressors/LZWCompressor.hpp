#ifndef _INCLUDED_LZW_COMPRESSOR_HPP_
#define _INCLUDED_LZW_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

#include <tudocomp/lzw/decode.hpp>
#include <tudocomp/lzw/Factor.hpp>
#include <tudocomp/lz78/dictionary.hpp>

namespace tdc {

template<typename coder_t>
class LZWCompressor: public Compressor {
private:
    static inline lz78_dictionary::CodeType select_size(Env& env, string_ref name) {
        auto& o = env.option(name);
        if (o.as_string() == "inf") {
            return lz78_dictionary::DMS_MAX;
        } else {
            return o.as_integer();
        }
    }

    /// Max dictionary size before reset
    const lz78_dictionary::CodeType dms {lz78_dictionary::DMS_MAX};
    //const CodeType dms {256 + 10};
    /// Preallocated dictionary size,
    /// picked because (sizeof(CodeType) = 4) * 1024 == 4096,
    /// which is the default page size on linux
    const lz78_dictionary::CodeType reserve_dms {1024};
public:
    inline LZWCompressor(Env&& env):
        Compressor(std::move(env)),
        dms(select_size(this->env(), "dict_size"))
    {}

    inline static Meta meta() {
        Meta m("compressor", "lzw",
               "Lempel-Ziv-Welch\n\n"
               "`dict_size` has to either be \"inf\", or a positive integer,\n"
               "and determines the maximum size of the backing storage of\n"
               "the dictionary before it gets reset.");
        m.option("coder").templated<coder_t>();
        m.option("dict_size").dynamic("inf");
        return m;
    }

    virtual void compress(Input& input, Output& out) override {
        auto is = input.as_stream();

        // Stats
        env().begin_stat_phase("LZW Compression");
        uint64_t stat_dictionary_resets = 0;
        uint64_t stat_dict_counter_at_last_reset = 0;
        uint64_t stat_factor_count = 0;
        uint64_t factor_count = 0;

        lz78_dictionary::EncoderDictionary ed(lz78_dictionary::EncoderDictionary::Lzw, dms, reserve_dms);
        typename coder_t::Encoder coder(env().env_for_option("coder"), out, NoLiterals());

        lz78_dictionary::CodeType i {dms}; // Index
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

            const lz78_dictionary::CodeType temp {i};

            if ((i = ed.search_and_insert(temp, b)) == dms)
            {
                coder.encode(temp, Range(factor_count + 256));
                //coder.encode_fact(temp);
                stat_factor_count++;
                factor_count++;
                i = b;
            }

            if (rbwf)
            {
                factor_count = 0; //coder.dictionary_reset();
                rbwf = false;
            }
        }
        if (i != dms) {
            //coder.encode_fact(i);
            coder.encode(i, Range(factor_count + 256));
            stat_factor_count++;
            factor_count++;
        }

        env().log_stat("factor_count", stat_factor_count);
        env().log_stat("dictionary_reset_counter",
                       stat_dictionary_resets);
        env().log_stat("max_factor_counter",
                       stat_dict_counter_at_last_reset);
        env().end_stat_phase();
    }

    virtual void decompress(Input& input, Output& output) override final {
        //TODO C::decode(in, out, dms, reserve_dms);
        auto out = output.as_stream();
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);

        uint64_t counter = 0;
        lzw::decode_step([&](lz78_dictionary::CodeType& entry, bool reset, bool &file_corrupted) -> lzw::Factor {
            if (reset) {
                counter = 0;
            }

            if(decoder.eof()) {
                return false;
            }

            lzw::Factor factor(decoder.template decode<uint64_t>(Range(counter + 256)));
            counter++;
            entry = factor;
            return true;
        }, out, dms, reserve_dms);
    }

};

}

#endif
