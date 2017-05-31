#pragma once

#include <tudocomp/Compressor.hpp>

#include <tudocomp/compressors/lzw/LZWDecoding.hpp>
#include <tudocomp/compressors/lzw/LZWFactor.hpp>

#include <tudocomp/Range.hpp>
#include <tudocomp/Coder.hpp>

#include <tudocomp_stat/StatPhase.hpp>

// For default params
#include <tudocomp/compressors/lz78/TernaryTrie.hpp>
#include <tudocomp/coders/BitCoder.hpp>

namespace tdc {

	class BitCoder;
	namespace lz78 {
		class TernaryTrie;
	}

template<typename coder_t, typename dict_t>
class LZWCompressor: public Compressor {
private:
    using node_t = typename dict_t::node_t;

    const lz78::factorid_t m_dict_max_size {0}; //! Maximum dictionary size before reset, 0 == unlimited
public:
    inline LZWCompressor(Env&& env):
        Compressor(std::move(env)),
        m_dict_max_size(env.option("dict_size").as_integer())
    {}

    inline static Meta meta() {
        Meta m("compressor", "lzw", "Lempel-Ziv-Welch\n\n" LZ78_DICT_SIZE_DESC);
        m.option("coder").templated<coder_t, BitCoder>("coder");
        m.option("lz78trie").templated<dict_t, lz78::TernaryTrie>("lz78trie");
        m.option("dict_size").dynamic(0);
        return m;
    }

    virtual void compress(Input& input, Output& out) override {
		const size_t n = input.size();
		const size_t reserved_size = isqrt(n)*2;
        auto is = input.as_stream();

        // Stats
        StatPhase phase("LZW Compression");
        IF_STATS(size_t stat_dictionary_resets = 0);
        IF_STATS(size_t stat_dict_counter_at_last_reset = 0);
        IF_STATS(size_t stat_factor_count = 0);
        size_t factor_count = 0;

		size_t remaining_characters = n; // position in the text
        dict_t dict(env().env_for_option("lz78trie"), n, remaining_characters, reserved_size+ULITERAL_MAX+1);
		auto reset_dict = [&dict] () {
			dict.clear();
            std::stringstream ss;
			for(size_t i = 0; i < ULITERAL_MAX+1; ++i) {
				const node_t node = dict.add_rootnode(i);
				DCHECK_EQ(node.id(), dict.size() - 1);
                DCHECK_EQ(node.id(), i);
                ss << node.id() << ", ";
			}
		};
		reset_dict();

        typename coder_t::Encoder coder(env().env_for_option("coder"), out, NoLiterals());

        char c;
		if(!is.get(c)) return;

		node_t node = dict.get_rootnode(static_cast<uliteral_t>(c));

		while(is.get(c)) {
			--remaining_characters;
			node_t child = dict.find_or_insert(node, static_cast<uliteral_t>(c));
			DVLOG(2) << " child " << child.id() << " #factor " << factor_count << " size " << dict.size() << " node " << node.id();

			if(child.id() == lz78::undef_id) {
                coder.encode(node.id(), Range(factor_count + ULITERAL_MAX + 1));
                IF_STATS(stat_factor_count++);
                factor_count++;
				DCHECK_EQ(factor_count+ULITERAL_MAX+1, dict.size());
                node = dict.get_rootnode(static_cast<uliteral_t>(c));
				// dictionary's maximum size was reached
				if(dict.size() == m_dict_max_size) {
					DCHECK_GT(dict.size(),0);
					reset_dict();
					factor_count = 0; //coder.dictionary_reset();
					IF_STATS(stat_dictionary_resets++);
					IF_STATS(stat_dict_counter_at_last_reset = m_dict_max_size);
				}
			} else { // traverse further
				node = child;
			}
        }

		DLOG(INFO) << "End node id of LZW parsing " << node.id();
		// take care of left-overs. We do not assume that the stream has a sentinel
		DCHECK_NE(node.id(), lz78::undef_id);
		coder.encode(node.id(), Range(factor_count + ULITERAL_MAX + 1)); //LZW
		IF_STATS(stat_factor_count++);
		factor_count++;

		IF_STATS(
        phase.log_stat("factor_count", stat_factor_count);
        phase.log_stat("dictionary_reset_counter", stat_dictionary_resets);
        phase.log_stat("max_factor_counter", stat_dict_counter_at_last_reset);
		)
    }

    virtual void decompress(Input& input, Output& output) override final {
		const size_t reserved_size = input.size();
        //TODO C::decode(in, out, dms, reserved_size);
        auto out = output.as_stream();
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);

        size_t counter = 0;

		//TODO file_corrupted not used!
        lzw::decode_step([&](lz78::factorid_t& entry, bool reset, bool &file_corrupted) -> bool {
            if (reset) {
                counter = 0;
            }

            if(decoder.eof()) {
                return false;
            }

            lzw::Factor factor(decoder.template decode<index_fast_t>(Range(counter + ULITERAL_MAX + 1)));
            counter++;
            entry = factor;
            return true;
        }, out, m_dict_max_size == 0 ? lz78::DMS_MAX : m_dict_max_size, reserved_size);
    }

};

}

