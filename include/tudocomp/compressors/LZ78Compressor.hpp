#pragma once

#include <tudocomp/Compressor.hpp>
#include <tudocomp/compressors/lz78/LZ78Coding.hpp>
#include <tudocomp/Range.hpp>

#include <tudocomp_stat/StatPhase.hpp>

// For default params
#include <tudocomp/compressors/lz78/TernaryTrie.hpp>
#include <tudocomp/coders/BinaryCoder.hpp>

#include <tudocomp/decompressors/LZ78Decompressor.hpp>

namespace tdc {

template <typename coder_t, typename dict_t>
class LZ78Compressor: public Compressor {
private:
    using node_t = typename dict_t::node_t;

    /// Max dictionary size before reset
    const lz78::factorid_t m_dict_max_size {0}; //! Maximum dictionary size before reset, 0 == unlimited

public:
    inline LZ78Compressor(Config&& cfg):
        Compressor(std::move(cfg)),
        m_dict_max_size(this->config().param("dict_size").as_uint())
    {}

    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "lz78",
            "Computes the Lempel-Ziv 78 factorization of the input.");
        m.param("coder", "The output encoder.")
            .strategy<coder_t>(TypeDesc("coder"), Meta::Default<BinaryCoder>());
        m.param("lz78trie", "The trie data structure implementation.")
            .strategy<dict_t>(TypeDesc("lz78trie"),
                Meta::Default<lz78::TernaryTrie>());
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
        StatPhase phase1("Lz78 compression");

        IF_STATS(size_t stat_dictionary_resets = 0);
        IF_STATS(size_t stat_dict_counter_at_last_reset = 0);
        IF_STATS(size_t stat_factor_count = 0);
        size_t factor_count = 0;

        size_t remaining_characters = n; // position in the text
        dict_t dict(config().sub_config("lz78trie"), n,
            remaining_characters, reserved_size);

        auto reset_dict = [&dict] () {
            dict.clear();
            node_t node = dict.add_rootnode(0);
            DCHECK_EQ(node.id(), dict.size() - 1);
            DCHECK_EQ(node.id(), 0);
        };
        reset_dict();

        typename coder_t::Encoder coder(
            config().sub_config("coder"), out, NoLiterals());

        // Define ranges
        node_t node = dict.get_rootnode(0);
        node_t parent = node; // parent of node, needed for the last factor
        DCHECK_EQ(node.id(), 0);
        DCHECK_EQ(parent.id(), 0);

        char c;
        while(is.get(c)) {
            --remaining_characters;
            node_t child = dict.find_or_insert(node, static_cast<uliteral_t>(c));
            if(child.is_new()) {
                lz78::encode_factor(coder, node.id(), static_cast<uliteral_t>(c), factor_count);
                factor_count++;
                IF_STATS(stat_factor_count++);
                parent = node = dict.get_rootnode(0); // return to the root
                DCHECK_EQ(node.id(), 0);
                DCHECK_EQ(parent.id(), 0);
                DCHECK_EQ(factor_count+1, dict.size());
                // dictionary's maximum size was reached
                if(tdc_unlikely(dict.size() == m_dict_max_size)) { // if m_dict_max_size == 0 this will never happen
                    DCHECK(false); // broken right now
                    reset_dict();
                    factor_count = 0; //coder.dictionary_reset();
                    IF_STATS(stat_dictionary_resets++);
                    IF_STATS(stat_dict_counter_at_last_reset = m_dict_max_size);
                }
            } else { // traverse further
                parent = node;
                node = child;
            }
        }

        // take care of left-overs. We do not assume that the stream has a sentinel
        if(node.id() != 0) {
            lz78::encode_factor(coder, parent.id(), c, factor_count);
//            // TODO: this check only works if the trie is not the MonteCarloTrie !
//            DCHECK_EQ(dict.find_or_insert(parent, static_cast<uliteral_t>(c)).id(), node.id());
            factor_count++;
            IF_STATS(stat_factor_count++);
        }

        IF_STATS(
        phase1.log_stat("factor_count", stat_factor_count);
        phase1.log_stat("dictionary_reset_counter",
                       stat_dictionary_resets);
        phase1.log_stat("max_factor_counter",
                       stat_dict_counter_at_last_reset);
        )
    }

    inline std::unique_ptr<Decompressor> decompressor() const override {
        return Algorithm::instance<LZ78Decompressor<coder_t>>();
    }
};


}//ns

