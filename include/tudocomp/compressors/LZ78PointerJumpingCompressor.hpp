#pragma once

#include <unordered_map>
#include <array>

#include <tudocomp/util.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Range.hpp>

#include <tudocomp/compressors/lz78/LZ78Coding.hpp>
#include <tudocomp/decompressors/LZ78Decompressor.hpp>

#include <tudocomp_stat/StatPhase.hpp>

// For default params
#include <tudocomp/compressors/lz_trie/TernaryTrie.hpp>
#include <tudocomp/coders/BinaryCoder.hpp>

#include <tudocomp/compressors/lz_pointer_jumping/FixedBufferPointerJumping.hpp>
#include <tudocomp/compressors/lz_pointer_jumping/PointerJumping.hpp>

namespace tdc {

template <typename coder_t, typename dict_t>
class LZ78PointerJumpingCompressor: public Compressor {
private:
    using node_t = typename dict_t::node_t;
    using encoder_t = typename coder_t::Encoder;
    struct stats_t {
        IF_STATS(size_t dictionary_resets = 0);
        IF_STATS(size_t dict_counter_at_last_reset = 0);
        IF_STATS(size_t total_factor_count = 0);
    };

    struct lz_algo_t {
        struct step_state_t {
            node_t parent;
            node_t node;
        };

        size_t& m_factor_count;
        encoder_t& m_coder;
        dict_t& m_dict;
        stats_t& m_stats;

        inline lz_algo_t(size_t& factor_count,
                         encoder_t& coder,
                         dict_t& dict,
                         stats_t& stats):
           m_factor_count(factor_count),
           m_coder(coder),
           m_dict(dict),
           m_stats(stats) {}
        lz_algo_t(lz_algo_t const&) = delete;
        lz_algo_t& operator=(lz_algo_t const&) = delete;

        inline void emit_factor(lz_trie::factorid_t node, uliteral_t c) {
            lz78::encode_factor(m_coder, node, c, m_factor_count);
            m_factor_count++;
            IF_STATS(m_stats.total_factor_count++);
            // std::cout << "FACTOR (" << node_id << ", " << c << ")" << std::endl;
        }
    };

    using step_state_t = typename lz_algo_t::step_state_t;

    using pointer_jumping_t =
        lz_pointer_jumping::PointerJumping<step_state_t,
                                           lz_pointer_jumping::FixedBufferPointerJumping<step_state_t>>;

    /// Max dictionary size before reset, 0 == unlimited
    const lz_trie::factorid_t m_dict_max_size {0};

    /// Pointer Jumping jump width.
    const size_t m_jump_width {1};

public:
    inline LZ78PointerJumpingCompressor(Config&& cfg):
        Compressor(std::move(cfg)),
        m_dict_max_size(this->config().param("dict_size").as_uint()),
        m_jump_width(this->config().param("jump_width").as_uint())
    {
        CHECK_GE(m_jump_width, 1);
        CHECK_LE(m_jump_width, pointer_jumping_t::MAX_JUMP_WIDTH);
        CHECK_EQ(m_dict_max_size, 0) << "dictionary resets are currently not supported";
    }

    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "lz78_pj",
            "Computes the Lempel-Ziv 78 factorization of the input.");
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
        StatPhase phase("Lz78 compression");
        stats_t stats;

        size_t factor_count = 0;
        char c; // input char used by the main loop

        // set up coder
        encoder_t coder(config().sub_config("coder"), out, NoLiterals());

        // set up dictionary (the lz trie)
        dict_t dict(config().sub_config("lz_trie"), n, reserved_size + 1);
        auto reset_dict = [&dict] {
            dict.clear();
            node_t const node = dict.add_rootnode(0);
            DCHECK_EQ(node.id(), dict.size() - 1);
            DCHECK_EQ(node.id(), 0U);
        };
        reset_dict();

        // set up lz algorithm state
        lz_algo_t lz_state { factor_count, coder, dict, stats };
        auto new_factor = [&](uint64_t node_id, uliteral_t c) {
            lz_state.emit_factor(node_id, c);
        };

        // set up initial search nodes
        node_t node = dict.get_rootnode(0);
        node_t parent = node; // parent of node, needed for the last factor
        DCHECK_EQ(node.id(), 0U);
        DCHECK_EQ(parent.id(), 0U);
        auto add_char_to_trie = [&dict,
                                 &new_factor,
                                 &factor_count,
                                 &node,
                                 &parent](uliteral_t c)
        {
            // advance trie state with the next read character
            dict.signal_character_read();
            node_t child = dict.find_or_insert(node, static_cast<uliteral_t>(c));

            if(child.is_new()) {
                // we found a leaf, output a factor
                new_factor(node.id(), static_cast<uliteral_t>(c));
            } else {
                // traverse further
                parent = node;
                node = child;
            }

            return child;
        };
        auto reset_search_nodes = [&dict, &factor_count, &node, &parent] (uliteral_t c) {
            // reset search node
            parent = node = dict.get_rootnode(0);
            DCHECK_EQ(node.id(), 0U);
            DCHECK_EQ(parent.id(), 0U);
            DCHECK_EQ(factor_count+1, dict.size());
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
                node = entry.get().node;
                parent = entry.get().parent;

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

                    pjm.insert_jump_buffer({node, child});

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
        if(node.id() != 0) {
            new_factor(parent.id(), static_cast<uliteral_t>(c));
        }

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
        return Algorithm::instance<LZ78Decompressor<coder_t>>(cfg.str());
    }
};

}//ns
