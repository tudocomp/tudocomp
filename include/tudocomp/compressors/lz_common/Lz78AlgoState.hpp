#pragma once

#include <tudocomp/compressors/lz_common/factorid_t.hpp>
#include <tudocomp/compressors/lz78/LZ78Coding.hpp>
#include <tudocomp/decompressors/LZ78Decompressor.hpp>
#include "BaseLzAlgoState.hpp"

namespace tdc {namespace lz_common {
    template<typename encoder_t, typename dict_t, typename stats_t>
    class Lz78AlgoState: public BaseLzAlgoState<encoder_t, dict_t, stats_t> {
    public:
        using BaseLzAlgoState<encoder_t, dict_t, stats_t>::BaseLzAlgoState;
        using BaseLzAlgoState<encoder_t, dict_t, stats_t>::m_factor_count;
        using BaseLzAlgoState<encoder_t, dict_t, stats_t>::m_coder;
        using BaseLzAlgoState<encoder_t, dict_t, stats_t>::m_dict;
        using BaseLzAlgoState<encoder_t, dict_t, stats_t>::m_stats;
        using node_t = typename dict_t::node_t;
        using factorid_t = lz_common::factorid_t;

        struct traverse_state_t {
            node_t parent;
            node_t node;
        };

        inline bool initialize_traverse_state(std::istream& is) {
            node_t node = m_dict.get_rootnode(0);
            node_t parent = node; // parent of node, needed for the last factor
            DCHECK_EQ(node.id(), 0U);
            DCHECK_EQ(parent.id(), 0U);

            m_traverse_state = { parent, node };

            return false;
        }
        inline void reset_traverse_state(uliteral_t last_read_char) {
            m_traverse_state.parent = m_dict.get_rootnode(0);
            m_traverse_state.node = m_traverse_state.parent;
            DCHECK_EQ(m_traverse_state.node.id(), 0U);
            DCHECK_EQ(m_traverse_state.parent.id(), 0U);
            DCHECK_EQ(m_factor_count+1, m_dict.size());
        }
        inline void traverse_to_child_node(node_t const& child) {
            m_traverse_state.parent = m_traverse_state.node;
            m_traverse_state.node = child;
        }
        inline node_t const& get_node(traverse_state_t const& state) const {
            return state.node;
        }
        inline void set_traverse_state(traverse_state_t const& state) {
            m_traverse_state = state;
        }
        inline traverse_state_t const& get_traverse_state() {
            return m_traverse_state;
        }
        inline static constexpr size_t initial_dict_size() {
            return 1;
        }
        inline void emit_factor(factorid_t node, uliteral_t c) {
            lz78::encode_factor(m_coder, node, c, m_factor_count);
            m_factor_count++;
            IF_STATS(m_stats.total_factor_count++);
            // std::cout << "FACTOR (" << node_id << ", " << c << ")" << std::endl;
        }
        inline void reset_dict() {
            m_dict.clear();
            node_t const node = m_dict.add_rootnode(0);
            DCHECK_EQ(node.id(), m_dict.size() - 1);
            DCHECK_EQ(node.id(), 0U);
        };
        inline void end_of_input(uliteral_t last_read_char) {
            if(m_traverse_state.node.id() != 0) {
                emit_factor(m_traverse_state.parent.id(), last_read_char);
            }
        }
        inline bool dict_find_or_insert( uliteral_t c) {
            return BaseLzAlgoState<encoder_t, dict_t, stats_t>::dict_find_or_insert(*this, c);
        }
    private:
        traverse_state_t m_traverse_state;
    };
}}
