#pragma once

#include <unordered_map>
#include <array>

#include <tudocomp/util.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Range.hpp>

#include <tudocomp/compressors/lzw/LZWFactor.hpp>
#include <tudocomp/decompressors/LZWDecompressor.hpp>

#include <tudocomp_stat/StatPhase.hpp>

// For default params
#include <tudocomp/compressors/lz_trie/TernaryTrie.hpp>
#include <tudocomp/coders/BinaryCoder.hpp>

#include <tudocomp/compressors/lz_pointer_jumping/FixedBufferPointerJumping.hpp>
#include <tudocomp/compressors/lz_pointer_jumping/PointerJumping.hpp>

namespace tdc {

template<typename coder_t, typename dict_t>
class LZWPointerJumpingCompressor: public Compressor {
private:
    using factorid_t = lz_trie::factorid_t;
    using node_t = typename dict_t::node_t;
    using encoder_t = typename coder_t::Encoder;
    struct stats_t {
        IF_STATS(size_t dictionary_resets = 0);
        IF_STATS(size_t dict_counter_at_last_reset = 0);
        IF_STATS(size_t total_factor_count = 0);
    };

    struct lz_algo_common_t {
        size_t& m_factor_count;
        encoder_t& m_coder;
        dict_t& m_dict;
        stats_t& m_stats;

        inline lz_algo_common_t(size_t& factor_count,
                                encoder_t& coder,
                                dict_t& dict,
                                stats_t& stats):
           m_factor_count(factor_count),
           m_coder(coder),
           m_dict(dict),
           m_stats(stats) {}
        lz_algo_common_t(lz_algo_common_t const&) = delete;
        lz_algo_common_t& operator=(lz_algo_common_t const&) = delete;
    };

    struct lz_algo_t: public lz_algo_common_t {
        using lz_algo_common_t::lz_algo_common_t;
        using lz_algo_common_t::m_factor_count;
        using lz_algo_common_t::m_coder;
        using lz_algo_common_t::m_dict;
        using lz_algo_common_t::m_stats;

        using traverse_state_t = node_t;

        traverse_state_t m_traverse_state;

        inline bool initialize_traverse_state(std::istream& is) {
            char c;
            if(!is.get(c)) return true;
            node_t node = m_dict.get_rootnode(static_cast<uliteral_t>(c));

            m_traverse_state = node;

            return false;
        }

        inline static constexpr size_t initial_dict_size() {
            return ULITERAL_MAX+1;
        }
        inline void emit_factor(factorid_t node, uliteral_t c) {
            m_coder.encode(node, Range(m_factor_count + ULITERAL_MAX + 1));
            m_factor_count++;
            IF_STATS(m_stats.total_factor_count++);
            // std::cout << "FACTOR (" << node_id << ")" << std::endl;
        }
        inline void reset_dict() {
            m_dict.clear();
            for(size_t i = 0; i < ULITERAL_MAX+1; ++i) {
                node_t const node = m_dict.add_rootnode(i);
                DCHECK_EQ(node.id(), m_dict.size() - 1);
                DCHECK_EQ(node.id(), i);
            }
        };
    };

    using traverse_state_t = typename lz_algo_t::traverse_state_t;

    using pointer_jumping_t =
        lz_pointer_jumping::PointerJumping<traverse_state_t,
                                           lz_pointer_jumping::FixedBufferPointerJumping<traverse_state_t>>;

    /// Max dictionary size before reset, 0 == unlimited
    const factorid_t m_dict_max_size {0};

    /// Pointer Jumping jump width.
    const size_t m_jump_width {1};

public:
    inline LZWPointerJumpingCompressor(Config&& cfg):
        Compressor(std::move(cfg)),
        m_dict_max_size(this->config().param("dict_size").as_uint()),
        m_jump_width(this->config().param("jump_width").as_uint())
    {
        CHECK_GE(m_jump_width, 1);
        CHECK_LE(m_jump_width, pointer_jumping_t::MAX_JUMP_WIDTH);
        CHECK_EQ(m_dict_max_size, 0) << "dictionary resets are currently not supported";
    }

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
        m.param("jump_width",
            "jump width of the pointer jumping optimization"
        ).primitive(4);
        return m;
    }

    virtual void compress(Input& input, Output& out) override {
        const size_t n = input.size();
        const size_t reserved_size = isqrt(n)*2;
        auto is = input.as_stream();

        // Stats
        StatPhase phase("LZW Compression");
        stats_t stats;

        size_t factor_count = 0;
        char c; // input char used by the main loop

        // set up coder
        encoder_t coder(config().sub_config("coder"), out, NoLiterals());

        // set up dictionary (the lz trie)
        dict_t dict(config().sub_config("lz_trie"), n, reserved_size + lz_algo_t::initial_dict_size());

        // set up lz algorithm state
        lz_algo_t lz_state { factor_count, coder, dict, stats };

        // set up initial search nodes
        lz_state.reset_dict();
        bool early_exit = lz_state.initialize_traverse_state(is);
        if (early_exit) return;
        node_t& node = lz_state.m_traverse_state;
        auto add_char_to_trie = [&dict,
                                 &lz_state,
                                 &factor_count,
                                 &node](uliteral_t c)
        {
            // advance trie state with the next read character
            dict.signal_character_read();
            node_t child = dict.find_or_insert(node, c);

            if(child.is_new()) {
                // we found a leaf, output a factor
                lz_state.emit_factor(node.id(), c);
            } else {
                // traverse further
                node = child;
            }

            return child;
        };
        auto reset_search_nodes = [&dict, &factor_count, &node] (uliteral_t c) {
            // reset search node
            node = dict.get_rootnode(static_cast<uliteral_t>(c));
            DCHECK_EQ(factor_count+ULITERAL_MAX+1, dict.size());
        };

        // set up pointer jumping
        pointer_jumping_t pjm(m_jump_width);
        pjm.reset_buffer(node.id());

        // begin of main loop
        continue_while: while(is.get(c)) {
            auto action = pjm.on_insert_char(static_cast<uliteral_t>(c));
            if (action.buffer_full_and_found()) {
                // we can jump ahead
                auto entry = action.entry();
                node = entry.get();

                pjm.reset_buffer(node.id());
            } else if (action.buffer_full_and_not_found()) {
                // we need to manually add to the trie,
                // and create a new jump entry
                for(size_t i = 0; i < pjm.jump_buffer_size() - 1; i++) {
                    uliteral_t const bc = pjm.jump_buffer(i);
                    auto child = add_char_to_trie(bc);
                    if (child.is_new()) {
                        // we got a new trie node in the middle of the jump buffer,
                        // restart the jump buffer search
                        reset_search_nodes(bc);
                        pjm.shift_buffer(i + 1, node.id());
                        goto continue_while;
                    }
                }
                {
                    // the child node for the last char in the buffer
                    // is also the target node for the new jump pointer
                    uliteral_t const bc = pjm.jump_buffer(pjm.jump_buffer_size() - 1);
                    auto child = add_char_to_trie(bc);

                    // the next time we will skip over this through the jump pointer
                    DCHECK(child.is_new());

                    pjm.insert_jump_buffer(child);

                    reset_search_nodes(bc);
                    pjm.reset_buffer(node.id());
                }
            } else {
                // read next char...
            }
        }

        // process chars from last incomplete jump buffer
        for(size_t i = 0; i < pjm.jump_buffer_size(); i++) {
            uliteral_t const bc = pjm.jump_buffer(i);
            auto child = add_char_to_trie(bc);
            if (child.is_new()) {
                reset_search_nodes(bc);
            }
        }

        // take care of left-overs. We do not assume that the stream has a sentinel
        DCHECK_NE(node.id(), lz_trie::undef_id);
        lz_state.emit_factor(node.id(), '\0');

        IF_STATS(
            phase.log_stat("factor_count",
                           stats.total_factor_count);
            phase.log_stat("dictionary_reset_counter",
                           stats.dictionary_resets);
            phase.log_stat("max_factor_counter",
                           stats.dict_counter_at_last_reset);
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
