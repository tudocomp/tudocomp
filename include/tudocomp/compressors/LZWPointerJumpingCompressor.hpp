#pragma once

#include <unordered_map>
#include <array>

#include <tudocomp/util.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Range.hpp>

#include <tudocomp/compressors/lz_common/LZWAlgoState.hpp>

#include <tudocomp_stat/StatPhase.hpp>

// For default params
#include <tudocomp/compressors/lz_trie/TernaryTrie.hpp>
#include <tudocomp/coders/BinaryCoder.hpp>

#include <tudocomp/compressors/lz_pointer_jumping/FixedBufferPointerJumping.hpp>
#include <tudocomp/compressors/lz_pointer_jumping/PointerJumping.hpp>

namespace tdc {
namespace lz_common {
    struct LZWPointerJumping {
        template<typename encoder_t, typename dict_t, typename stats_t>
        using lz_state_t = LZWAlgoState<encoder_t, dict_t, stats_t>;

        template<typename coder_t>
        using Decompressor = LZWDecompressor<coder_t>;

        inline static char const* meta_name() {
            return "lzw_pj";
        }
        inline static char const* meta_desc() {
            return "Computes the Lempel-Ziv-Welch factorization of the input.";
        }
        inline static char const* stat_phase_name() {
            return "LZW Compression";
        }
    };
}

template<typename coder_t, typename dict_t>
class LZWPointerJumpingCompressor: public Compressor {
private:
    using lz_algo_t = lz_common::LZWPointerJumping;

    using factorid_t = lz_trie::factorid_t;
    using node_t = typename dict_t::node_t;
    using encoder_t = typename coder_t::Encoder;
    struct stats_t {
        IF_STATS(size_t dictionary_resets = 0);
        IF_STATS(size_t dict_counter_at_last_reset = 0);
        IF_STATS(size_t total_factor_count = 0);
    };

    using lz_state_t = typename lz_algo_t::lz_state_t<encoder_t, dict_t, stats_t>;

    using traverse_state_t = typename lz_state_t::traverse_state_t;

    using pointer_jumping_t =
        lz_pointer_jumping::PointerJumping<lz_pointer_jumping::FixedBufferPointerJumping<traverse_state_t>>;

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
        Meta m(Compressor::type_desc(),
               lz_algo_t::meta_name(),
               lz_algo_t::meta_desc());
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
        ).primitive(2);
        return m;
    }

    virtual void compress(Input& input, Output& out) override {
        const size_t n = input.size();
        const size_t reserved_size = isqrt(n)*2;
        auto is = input.as_stream();

        // Stats
        StatPhase phase(lz_algo_t::stat_phase_name());
        stats_t stats;

        size_t factor_count = 0;

        // set up coder
        encoder_t coder(config().sub_config("coder"), out, NoLiterals());

        // set up dictionary (the lz trie)
        dict_t dict(config().sub_config("lz_trie"), n, reserved_size + lz_state_t::initial_dict_size());

        // set up lz algorithm state
        lz_state_t lz_state { factor_count, coder, dict, stats };
        node_t const& node = lz_state.get_current_node();

        // set up initial search nodes
        lz_state.reset_dict();
        bool early_exit = lz_state.initialize_traverse_state(is);
        if (early_exit) return;
        auto find_or_insert = [&dict, &lz_state, &factor_count, &node](uliteral_t c) {
            // advance trie state with the next read character
            dict.signal_character_read();
            node_t child = dict.find_or_insert(node, c);

            if(child.is_new()) {
                // we found a leaf, output a factor
                lz_state.emit_factor(node.id(), c);
            }

            // traverse further
            lz_state.traverse_to_child_node(child);

            return child.is_new();
        };

        // set up pointer jumping
        pointer_jumping_t pjm(m_jump_width);
        pjm.reset_buffer(node.id());

        // main loop
        char c;
        continue_while: while(is.get(c)) {
            auto action = pjm.on_insert_char(static_cast<uliteral_t>(c));
            if (action.buffer_full_and_found()) {
                // we can jump ahead
                lz_state.set_traverse_state(action.traverse_state());

                pjm.reset_buffer(node.id());
            } else if (action.buffer_full_and_not_found()) {
                // we need to manually add to the trie,
                // and create a new jump entry
                for(size_t i = 0; i < pjm.jump_buffer_size() - 1; i++) {
                    uliteral_t const bc = pjm.jump_buffer(i);
                    bool is_new_node = find_or_insert(bc);
                    if (is_new_node) {
                        // we got a new trie node in the middle of the
                        // jump buffer, restart the jump buffer search
                        lz_state.reset_traverse_state(bc);
                        pjm.shift_buffer(i + 1, node.id());
                        goto continue_while;
                    }
                }
                {
                    // the child node for the last char in the buffer
                    // is also the node the new jump pointer jumps to.
                    size_t i = pjm.jump_buffer_size() - 1;
                    uliteral_t const bc = pjm.jump_buffer(i);
                    bool is_new_node = find_or_insert(bc);

                    // the next time we will skip over this through the jump pointer
                    DCHECK(is_new_node);

                    pjm.insert_jump_buffer(lz_state.get_traverse_state());

                    lz_state.reset_traverse_state(bc);
                    pjm.reset_buffer(node.id());
                }
            } else {
                // read next char...
            }
        }

        // process chars from last incomplete jump buffer
        for(size_t i = 0; i < pjm.jump_buffer_size(); i++) {
            uliteral_t const bc = pjm.jump_buffer(i);
            bool is_new_node = find_or_insert(bc);
            if (is_new_node) {
                lz_state.reset_traverse_state(bc);
            }
        }

        // take care of left-overs. We do not assume that the stream has a sentinel
        lz_state.end_of_input(static_cast<uliteral_t>(c));

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
        return Algorithm::instance<lz_algo_t::Decompressor<coder_t>>(cfg.str());
    }
};

}//ns
