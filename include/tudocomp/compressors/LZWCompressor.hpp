#ifndef _INCLUDED_LZW_COMPRESSOR_HPP_
#define _INCLUDED_LZW_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

#include <tudocomp/compressors/lz78/LZ78Dictionary.hpp>
#include <tudocomp/compressors/lzw/LZWDecoding.hpp>
#include <tudocomp/compressors/lzw/LZWFactor.hpp>

#include <tudocomp/coders/BitOptimalCoder.hpp> //default

namespace tdc {

template<typename coder_t>
class LZWCompressor: public Compressor {
private:
    static inline lz78::CodeType select_size(Env& env, string_ref name) {
        auto& o = env.option(name);
        if (o.as_string() == "inf") {
            return lz78::DMS_MAX;
        } else {
            return o.as_integer();
        }
    }

    /// Max dictionary size before reset
    const lz78::CodeType m_dict_max_size {lz78::DMS_MAX};
    //const CodeType dms {256 + 10};
    /// Preallocated dictionary size,
    /// picked because (sizeof(CodeType) = 4) * 1024 == 4096,
    /// which is the default page size on linux
    const lz78::CodeType reserve_dms {1024};
public:
    inline LZWCompressor(Env&& env):
        Compressor(std::move(env)),
        m_dict_max_size(select_size(this->env(), "dict_size"))
    {}

    inline static Meta meta() {
        Meta m("compressor", "lzw",
               "Lempel-Ziv-Welch\n\n"
               "`dict_size` has to either be \"inf\", or a positive integer,\n"
               "and determines the maximum size of the backing storage of\n"
               "the dictionary before it gets reset.");
        m.option("coder").templated<coder_t, BitOptimalCoder>();
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

        lz78::EncoderDictionary ed(lz78::EncoderDictionary::Lzw, 0, reserve_dms);
        typename coder_t::Encoder coder(env().env_for_option("coder"), out, NoLiterals());

        lz78::factorid_t node {0}; // LZ78 node
        char c;

        while (is.get(c)) {
			lz78::factorid_t child = ed.search_and_insert(node, static_cast<uliteral_t>(c));
			std::cout << " child " << child << " #factor " << factor_count << " size " << ed.size() << " node " << node << std::endl;


			if(node != 0 && child == 0) {
                coder.encode(node, Range(factor_count + uliteral_max + 1));
                stat_factor_count++;
                factor_count++;
				DCHECK_EQ(factor_count+uliteral_max+1, ed.size());
                node = static_cast<uliteral_t>(c); //LZW
				// dictionary's maximum size was reached
				if(ed.size() == m_dict_max_size) {
					ed.reset();
					factor_count = 0; //coder.dictionary_reset();
					stat_dictionary_resets++;
					stat_dict_counter_at_last_reset = m_dict_max_size;
				}
			} else { // traverse further
				node = child;
			}
        }

		std::cout << "LZW: " << node << std::endl;
		// take care of left-overs. We do not assume that the stream has a sentinel
//        if(node != static_cast<uliteral_t>(c)) {
         if(node != 0 || factor_count > 0) {
            //coder.encode_fact(i);
            coder.encode(node, Range(factor_count + uliteral_max + 1)); //LZW
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
		
        lzw::decode_step([&](lz78::CodeType& entry, bool reset, bool &file_corrupted) -> lzw::Factor {
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
        }, out, m_dict_max_size, reserve_dms);
    }

};

}

#endif
