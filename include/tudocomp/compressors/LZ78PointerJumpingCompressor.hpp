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
    struct NodePair {
        node_t parent;
        node_t node;
    };
    using pointer_jumping_t =
        lz_pointer_jumping::PointerJumping<NodePair,
                                           lz_pointer_jumping::FixedBufferPointerJumping<NodePair>>;

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

        IF_STATS(size_t stat_dictionary_resets = 0);
        IF_STATS(size_t stat_dict_counter_at_last_reset = 0);
        IF_STATS(size_t stat_factor_count = 0);
        size_t factor_count = 0;
        char c; // input char used by the main loop

        // set up coder
        typename coder_t::Encoder coder(config().sub_config("coder"), out, NoLiterals());
        auto new_factor = [&](uint64_t node_id, uliteral_t c) {
            lz78::encode_factor(coder, node_id, c, factor_count);
            factor_count++;
            IF_STATS(stat_factor_count++);
            std::cout << "FACTOR (" << node_id << ", " << c << ")" << std::endl;
        };

        // set up dictionary (the lz trie)
        dict_t dict(config().sub_config("lz_trie"), n, reserved_size);
        auto reset_dict = [&dict] {
            dict.clear();
            node_t node = dict.add_rootnode(0);
            DCHECK_EQ(node.id(), dict.size() - 1);
            DCHECK_EQ(node.id(), 0U);
        };
        reset_dict();

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
            auto debug_print_traverse_mapping = [](auto node, auto c, auto child, char m) {
                std::cout << "(" << node << ") -" << c << "-> (" << child << ")" << m << std::endl;
            };
            std::cout << "add char '" << c << "' to trie" << std::endl;
            // advance trie state with the next read character
            dict.signal_character_read();
            node_t child = dict.find_or_insert(node, static_cast<uliteral_t>(c));

            if(child.is_new()) {
                // we found a leaf, output a factor
                new_factor(node.id(), static_cast<uliteral_t>(c));
                debug_print_traverse_mapping(node.id(), c, child.id(), '*');

                // reset search node
                parent = node = dict.get_rootnode(0);
                DCHECK_EQ(node.id(), 0U);
                DCHECK_EQ(parent.id(), 0U);
                DCHECK_EQ(factor_count+1, dict.size());

                return child;
            } else {
                debug_print_traverse_mapping(node.id(), c, child.id(), ' ');
                // traverse further
                parent = node;
                node = child;

                return child;
            }
        };

        // set up pointer jumping
        pointer_jumping_t pjm(m_jump_width);
        pjm.reset_buffer(node.id());
        auto debug_print_current_jump_buffer_mapping = [&pjm, &node] (char m) {
            std::cout << "(" << pjm.jump_buffer_parent_node() << ") -";
            std::cout << "[";
            pjm.debug_print_buffer(std::cout);
            std::cout << "]";
            std::cout << "-> (" << node.id() << ")" << m << std::endl;
        };

        // begin of main loop
        continue_while: while(is.get(c)) {
            auto action = pjm.on_insert_char(static_cast<uliteral_t>(c));
            if (action.buffer_full_and_found()) {
                // we can jump ahead
                auto entry = action.entry();
                node = entry.get().node;
                parent = entry.get().parent;
                debug_print_current_jump_buffer_mapping(' ');

                pjm.reset_buffer(node.id());
            } else if (action.buffer_full_and_not_found()) {
                // we need to manually add to the trie,
                // and create a new jump entry
                for(size_t i = 0; i < pjm.jump_buffer_size(); i++) {
                    auto child = add_char_to_trie(pjm.jump_buffer(i));
                    if (child.is_new()) {
                        // we got a new trie node in the middle of the jump buffer,
                        // restart the jump buffer search
                        pjm.shift_buffer(i + 1, node.id());
                        goto continue_while;
                    }
                }
                // auto child = add_char_to_trie(pjm.jump_buffer(pjm.jump_buffer_size() - 1));

                pjm.insert_jump_buffer(NodePair{ parent, node });
                debug_print_current_jump_buffer_mapping('*');
                pjm.reset_buffer(node.id());
            } else {
                // read next char...
            }
        }

        std::cout << "finished main loop" << std::endl;

        // process chars from last incomplete jump buffer
        for(size_t i = 0; i < pjm.jump_buffer_size(); i++) {
            add_char_to_trie(pjm.jump_buffer(i));
        }

        // take care of left-overs. We do not assume that the stream has a sentinel
        if(node.id() != 0) {
            new_factor(parent.id(), static_cast<uliteral_t>(c));
        }

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
        return Algorithm::instance<LZ78Decompressor<coder_t>>(cfg.str());
    }
};

}//ns
