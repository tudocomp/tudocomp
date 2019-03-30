#pragma once

#include <unordered_map>
#include <array>

#include <tudocomp/compressors/lz_trie/LZTrie.hpp>

namespace tdc {namespace lz_pointer_jumping {

static constexpr bool PRINT_DEBUG_TRANSITIONS = false;

template<typename pj_trie_t>
class PointerJumping: public pj_trie_t {
    using jump_buffer_handle = typename pj_trie_t::jump_buffer_handle;
public:
    using lz_state_t = typename pj_trie_t::lz_state_t;
    using traverse_state_t = typename pj_trie_t::traverse_state_t;
    using factorid_t = lz_trie::factorid_t;

    inline PointerJumping(lz_state_t& lz_state, size_t jump_width):
        pj_trie_t(jump_width),
        m_lz_state(lz_state),
        m_jump_width(jump_width),
        m_jump_buffer_handle(this->get_working_buffer())
    {}

    inline uliteral_t& jump_buffer(size_t i) {
        DCHECK_LT(i, m_jump_width);
        return this->get_buffer(m_jump_buffer_handle)[i];
    }
    inline uliteral_t const& jump_buffer(size_t i) const {
        DCHECK_LT(i, m_jump_width);
        return this->get_buffer(m_jump_buffer_handle)[i];
    }
    inline factorid_t jump_buffer_parent_node() const {
        return this->get_parent_node(m_jump_buffer_handle);
    }
    inline size_t jump_buffer_size() const {
        return m_jump_buffer_size;
    }

    struct action_t {
        bool m_was_full;
        typename pj_trie_t::result_t m_result;

        inline bool buffer_full_and_found() const {
            return m_was_full && m_result.found();
        }
        inline bool buffer_full_and_not_found() const {
            return m_was_full && (!m_result.found());
        }
        inline traverse_state_t traverse_state() const {
            DCHECK(buffer_full_and_found());
            return m_result.get();
        }
    };
    inline action_t on_insert_char(uliteral_t c) {
        DCHECK_LT(m_jump_buffer_size, m_jump_width);
        jump_buffer(m_jump_buffer_size) = c;
        m_jump_buffer_size++;

        if(jump_buffer_full()) {
            auto entry = find_jump_buffer();
            if (entry.found()) {
                debug_transition(std::cout, entry.get(), false);
            }
            return action_t { true, entry };
        } else {
            debug_open_transition(std::cout);
            return action_t { false, typename pj_trie_t::result_t() };
        }
    }

    inline void shift_buffer(size_t elements) {
        factorid_t parent_node = current_lz_node_id();
        size_t remaining = m_jump_width - elements;
        for(size_t i = 0; i < remaining; i++) {
            jump_buffer(i) = jump_buffer(i + elements);
        }
        m_jump_buffer_size -= elements;
        this->set_parent_node(m_jump_buffer_handle, parent_node);
        debug_open_transition(std::cout);
    }

    inline void reset_buffer() {
        factorid_t parent_node = current_lz_node_id();
        m_jump_buffer_size = 0;
        this->set_parent_node(m_jump_buffer_handle, parent_node);
        debug_open_transition(std::cout);
    }

    inline bool jump_buffer_full() const {
        return m_jump_width == m_jump_buffer_size;
    }

    inline auto find_jump_buffer() const {
        return this->find(m_jump_buffer_handle);
    }

    inline void insert_jump_buffer(traverse_state_t const& val) {
        debug_transition(std::cout, val, true);
        this->insert(m_jump_buffer_handle, val);
    }

    // DEBUG printing function

    inline void debug_print_buffer(std::ostream& out, jump_buffer_handle const& handle, size_t size) const {
        if (!PRINT_DEBUG_TRANSITIONS) return;

        for(size_t i = 0; i < size; i++) {
            out << this->get_buffer(handle)[i];
        }
    }

    inline void debug_print_buffer(std::ostream& out) const {
        if (!PRINT_DEBUG_TRANSITIONS) return;

        debug_print_buffer(out, m_jump_buffer_handle, m_jump_buffer_size);
    }

    inline void debug_open_transition(std::ostream& out) const {
        if (!PRINT_DEBUG_TRANSITIONS) return;

        factorid_t parent_node = jump_buffer_parent_node();

        out << "(" << parent_node << ") -[";
        debug_print_buffer(out);
        out << "..." << std::endl;
    }
    inline void debug_transition(std::ostream& out, traverse_state_t const& to, bool is_new) const {
        if (!PRINT_DEBUG_TRANSITIONS) return;

        factorid_t parent_node = jump_buffer_parent_node();

        out << "(" << parent_node << ") -[";
        debug_print_buffer(out);
        out << "]-> (" << m_lz_state.get_node(to).id() << ")" ;
        if (is_new) {
            out << "*";
        }
        out << std::endl;
    }
private:
    lz_state_t&        m_lz_state;
    size_t             m_jump_width;
    jump_buffer_handle m_jump_buffer_handle;

    size_t             m_jump_buffer_size = 0;

    inline factorid_t current_lz_node_id() const {
        return m_lz_state.get_node(m_lz_state.get_traverse_state()).id();
    }
};

}}
