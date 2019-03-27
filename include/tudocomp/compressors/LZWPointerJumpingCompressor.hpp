#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Range.hpp>

#include <tudocomp/compressors/lzw/LZWFactor.hpp>
#include <tudocomp/decompressors/LZWDecompressor.hpp>

#include <tudocomp_stat/StatPhase.hpp>

// For default params
#include <tudocomp/compressors/lz_trie/TernaryTrie.hpp>
#include <tudocomp/coders/BinaryCoder.hpp>

namespace tdc {

template<typename coder_t, typename dict_t>
class LZWPointerJumpingCompressor: public Compressor {
private:
    using node_t = typename dict_t::node_t;

    /// Max dictionary size before reset
    const lz_trie::factorid_t m_dict_max_size {0}; //! Maximum dictionary size before reset, 0 == unlimited

public:
    inline LZWPointerJumpingCompressor(Config&& cfg):
        Compressor(std::move(cfg)),
        m_dict_max_size(this->config().param("dict_size").as_uint())
    {}

    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "lzw_pj",
            "Computes the Lempel-Ziv-Welch factorization of the input.");
        m.param("coder", "The output encoder.")
            .strategy<coder_t>(TypeDesc("coder"), Meta::Default<BinaryCoder>());
        m.param("lz_trie", "The trie data structure implementation.")
            .strategy<dict_t>(TypeDesc("lz_trie"),
                Meta::Default<lz_trie::TernaryTrie>());
        m.param("dict_size",
            "the maximum size of the dictionary's backing storage before it "
            "gets reset (0 = unlimited)"
        ).primitive(0);
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

        dict_t dict(config().sub_config("lz_trie"), n, reserved_size+ULITERAL_MAX+1);
        auto reset_dict = [&dict] {
            dict.clear();
            for(size_t i = 0; i < ULITERAL_MAX+1; ++i) {
                const node_t node = dict.add_rootnode(i);
                DCHECK_EQ(node.id(), dict.size() - 1);
                DCHECK_EQ(node.id(), i);
            }
        };
        reset_dict();

        typename coder_t::Encoder coder(config().sub_config("coder"), out, NoLiterals());
        auto new_factor = [&](uint64_t node_id) {
            coder.encode(node_id, Range(factor_count + ULITERAL_MAX + 1));
            factor_count++;
            IF_STATS(stat_factor_count++);
        };

        // setup initial state for the node search
        char c;

        // try to get first char from input, set up initial search node
        if(!is.get(c)) return;
        node_t node = dict.get_rootnode(static_cast<uliteral_t>(c));

        while(is.get(c)) {
            // advance trie state with the next read character
            dict.signal_character_read();
            node_t child = dict.find_or_insert(node, static_cast<uliteral_t>(c));
            if(child.is_new()) {
                // we found a leaf, output a factor
                new_factor(node.id());

                // reset search node
                node = dict.get_rootnode(static_cast<uliteral_t>(c));
                DCHECK_EQ(factor_count+ULITERAL_MAX+1, dict.size());

                // check if dictionary's maximum size was reached
                // (this will never happen if m_dict_max_size == 0)
                if(tdc_unlikely(dict.size() == m_dict_max_size)) {
                    DCHECK_GT(dict.size(),0U);
                    reset_dict();
                    factor_count = 0;
                    IF_STATS(stat_dictionary_resets++);
                    IF_STATS(stat_dict_counter_at_last_reset = m_dict_max_size);
                }
            } else {
                // traverse further
                node = child;
            }
        }

        // take care of left-overs. We do not assume that the stream has a sentinel
        DCHECK_NE(node.id(), lz_trie::undef_id);
        new_factor(node.id());

        IF_STATS(
            phase.log_stat("factor_count",
                           stat_factor_count);
            phase.log_stat("dictionary_reset_counter",
                           stat_dictionary_resets);
            phase.log_stat("max_factor_counter",
                           stat_dict_counter_at_last_reset);
        )
    }

    inline std::unique_ptr<Decompressor> decompressor() const override {
        // FIXME: construct AST and pass it
        std::stringstream cfg;
        cfg << "dict_size=" << to_string(m_dict_max_size);
        return Algorithm::instance<LZWDecompressor<coder_t>>(cfg.str());
    }
};

}//ns
