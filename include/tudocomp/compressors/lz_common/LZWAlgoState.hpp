#pragma once

#include <tudocomp/compressors/lz_trie/LZTrie.hpp>
#include <tudocomp/compressors/lzw/LZWFactor.hpp>
#include <tudocomp/decompressors/LZWDecompressor.hpp>
#include "BaseLzAlgoState.hpp"

namespace tdc {namespace lz_common {
    template<typename encoder_t, typename dict_t, typename stats_t>
    class LZWAlgoState: public BaseLzAlgoState<encoder_t, dict_t, stats_t> {
    public:
        using BaseLzAlgoState<encoder_t, dict_t, stats_t>::BaseLzAlgoState;
        using BaseLzAlgoState<encoder_t, dict_t, stats_t>::m_factor_count;
        using BaseLzAlgoState<encoder_t, dict_t, stats_t>::m_coder;
        using BaseLzAlgoState<encoder_t, dict_t, stats_t>::m_dict;
        using BaseLzAlgoState<encoder_t, dict_t, stats_t>::m_stats;
        using node_t = typename dict_t::node_t;
        using factorid_t = lz_trie::factorid_t;

        using traverse_state_t = node_t;

        inline bool initialize_traverse_state(std::istream& is) {
            char c;
            if(!is.get(c)) return true;
            node_t node = m_dict.get_rootnode(static_cast<uliteral_t>(c));

            m_traverse_state = node;

            return false;
        }
        inline void reset_traverse_state(uliteral_t last_read_char) {
            m_traverse_state = m_dict.get_rootnode(last_read_char);
            DCHECK_EQ(m_factor_count+ULITERAL_MAX+1, m_dict.size());
        }
        inline void traverse_to_child_node(node_t const& child) {
            m_traverse_state = child;
        }
        inline node_t const& get_node(traverse_state_t const& state) const {
            return state;
        }
        inline void set_traverse_state(traverse_state_t const& state) {
            m_traverse_state = state;
        }
        inline traverse_state_t const& get_traverse_state() {
            return m_traverse_state;
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
        inline void end_of_input(uliteral_t last_read_char) {
            DCHECK_NE(m_traverse_state.id(), lz_trie::undef_id);
            emit_factor(m_traverse_state.id(), '\0');
        }
        inline bool dict_find_or_insert(uliteral_t c) {
            return BaseLzAlgoState<encoder_t, dict_t, stats_t>::dict_find_or_insert(*this, c);
        }
    private:
        traverse_state_t m_traverse_state;
    };
}}
