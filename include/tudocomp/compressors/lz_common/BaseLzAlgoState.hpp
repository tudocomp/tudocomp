#pragma once

namespace tdc {namespace lz_common {
    template<typename encoder_t, typename dict_t, typename stats_t>
    class BaseLzAlgoState {
    public:
        using node_t = typename dict_t::node_t;

        inline BaseLzAlgoState(size_t& factor_count,
                               encoder_t& coder,
                               dict_t& dict,
                               stats_t& stats):
           m_factor_count(factor_count),
           m_coder(coder),
           m_dict(dict),
           m_stats(stats) {}
        BaseLzAlgoState(BaseLzAlgoState const&) = delete;
        BaseLzAlgoState& operator=(BaseLzAlgoState const&) = delete;
    protected:
        size_t& m_factor_count;
        encoder_t& m_coder;
        dict_t& m_dict;
        stats_t& m_stats;

        template<typename self_t>
        inline bool dict_find_or_insert(self_t& self, uliteral_t c) {
            node_t const& node = self.get_node(self.get_traverse_state());

            // advance trie state with the next read character
            m_dict.signal_character_read();
            node_t child = m_dict.find_or_insert(node, c);

            if(child.is_new()) {
                // we found a leaf, output a factor
                self.emit_factor(node.id(), c);
            }

            // traverse further
            self.traverse_to_child_node(child);

            return child.is_new();
        };
    };
}}
