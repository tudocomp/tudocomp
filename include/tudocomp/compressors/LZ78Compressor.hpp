#ifndef _INCLUDED_LZ78_COMPRESSOR_HPP_
#define _INCLUDED_LZ78_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

#include <tudocomp/compressors/lz78/LZ78Dictionary.hpp>

#include <tudocomp/Range.hpp>
#include <tudocomp/coders/BitOptimalCoder.hpp> //default

namespace tdc {

	namespace lz78 {
		class Decompressor {
			std::vector<lz78::factorid_t> indices;
			std::vector<uliteral_t> literals;

			public:
			inline void decompress(lz78::factorid_t index, uliteral_t literal, std::ostream& out) {
				indices.push_back(index);
				literals.push_back(literal);
				std::vector<uliteral_t> buffer;

				while(index != 0) {
					buffer.push_back(literal);
					literal = literals[index - 1];
					index = indices[index - 1];
				}

				out << literal;
				for(size_t i = buffer.size(); i > 0; i--) {
					out << buffer[i - 1];
				}
			}

		};
	}//ns


template <typename coder_t>
class LZ78Compressor: public Compressor {
private:
    static inline lz78::factorid_t select_size(Env& env, string_ref name) {
        auto& o = env.option(name);
        if (o.as_string() == "inf") {
            return 0;
        } else {
            return o.as_integer();
        }
    }

    /// Max dictionary size before reset
    const lz78::factorid_t m_dict_max_size {0};

public:
    inline LZ78Compressor(Env&& env):
        Compressor(std::move(env)),
        m_dict_max_size(select_size(this->env(), "dict_size"))
    {}

    inline static Meta meta() {
        Meta m("compressor", "lz78",
               "Lempel-Ziv 78\n\n"
               "`dict_size` has to either be \"inf\", or a positive integer,\n"
               "and determines the maximum size of the backing storage of\n"
               "the dictionary before it gets reset.");
        m.option("coder").templated<coder_t, BitOptimalCoder>();
        m.option("dict_size").dynamic("inf");
        return m;
    }

    virtual void compress(Input& input, Output& out) override {
		const size_t reserved_size = isqrt(input.size())*2;
        auto is = input.as_stream();

        // Stats
        env().begin_stat_phase("Lz78 compression");
        uint64_t stat_dictionary_resets = 0;
        uint64_t stat_dict_counter_at_last_reset = 0;
        uint64_t stat_factor_count = 0;
        uint64_t factor_count = 0;

        lz78::EncoderDictionary dict(lz78::EncoderDictionary::Lz78, 0, reserved_size);
        typename coder_t::Encoder coder(env().env_for_option("coder"), out, NoLiterals());

        // Define ranges
        lz78::factorid_t node {0}; // LZ78 node
        lz78::factorid_t parent {0}; // parent of node, needed for the last factor
        char c;

        while (is.get(c)) {
			lz78::factorid_t child = dict.search_and_insert(node, static_cast<uliteral_t>(c));
			if(child == lz78::undef_id) {
                coder.encode(node, Range(factor_count));
                coder.encode(static_cast<uliteral_t>(c), literal_r);
                factor_count++;
                stat_factor_count++;
				parent = node = 0; // return to the root
				DCHECK_EQ(factor_count+1, dict.size());
				// dictionary's maximum size was reached
				if(tdc_unlikely(dict.size() == m_dict_max_size)) { // if m_dict_max_size == 0 this will never happen
					dict.reset();
					factor_count = 0; //coder.dictionary_reset();
					stat_dictionary_resets++;
					stat_dict_counter_at_last_reset = m_dict_max_size;
				}
			} else { // traverse further
				parent = node;
				node = child;
			}
        }

		// take care of left-overs. We do not assume that the stream has a sentinel
        if(node != 0) {
            coder.encode(parent, Range(factor_count));
            coder.encode(c, literal_r);
			DCHECK_EQ(dict.search_and_insert(parent, static_cast<uliteral_t>(c)), node);
            factor_count++;
            stat_factor_count++;
        }

        coder.finalize();

        env().log_stat("factor_count", stat_factor_count);
        env().log_stat("dictionary_reset_counter",
                       stat_dictionary_resets);
        env().log_stat("max_factor_counter",
                       stat_dict_counter_at_last_reset);
        env().end_stat_phase();
    }

    virtual void decompress(Input& input, Output& output) override final {
        auto out = output.as_stream();
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);

        lz78::Decompressor decomp;
        uint64_t factor_count = 0;

        while (!decoder.eof()) {
			const lz78::factorid_t index = decoder.template decode<lz78::factorid_t>(Range(factor_count));
            const uliteral_t chr = decoder.template decode<uliteral_t>(literal_r);
			decomp.decompress(index, chr, out);
            factor_count++;
        }

        out.flush();
    }

};


}//ns
#endif
