#pragma once

#include <unordered_map>
#include <array>

#include <tudocomp/compressors/lz_trie/LZTrie.hpp>

namespace tdc {namespace lz_pointer_jumping {

template<typename value_type, typename pj_trie_t>
class PointerJumping: public pj_trie_t {
    using jump_buffer_handle = typename pj_trie_t::jump_buffer_handle;

    size_t             m_jump_width = 0;
    jump_buffer_handle m_jump_buffer_handle;
    size_t             m_jump_buffer_size = 0;
public:

    inline PointerJumping(size_t jump_width):
        pj_trie_t(jump_width),
        m_jump_width(jump_width)
    {}

    inline uliteral_t& jump_buffer(size_t i) {
        DCHECK_LT(i, m_jump_width);
        return this->get_buffer(m_jump_buffer_handle)[i];
    }
    inline uliteral_t const& jump_buffer(size_t i) const {
        DCHECK_LT(i, m_jump_width);
        return this->get_buffer(m_jump_buffer_handle)[i];
    }
    inline lz_trie::factorid_t jump_buffer_parent_node() {
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
        inline typename pj_trie_t::result_t entry() const {
            return m_result;
        }
    };
    inline action_t on_insert_char(uliteral_t c) {
        DCHECK_LT(m_jump_buffer_size, m_jump_width);
        jump_buffer(m_jump_buffer_size) = c;
        m_jump_buffer_size++;
        std::cout << "process char '" << c << "', buffer: \"";
        debug_print_buffer(std::cout);
        std::cout << "\"" << std::endl;

        if(jump_buffer_full()) {
            auto entry = find_jump_buffer();
            return action_t { true, entry };
        } else {
            return action_t { false, typename pj_trie_t::result_t() };
        }
    }

    inline void reset_buffer(lz_trie::factorid_t parent_node) {
        m_jump_buffer_size = 0;
        this->set_parent_node(m_jump_buffer_handle, parent_node);
        std::cout << "reset buffersize to 0, buffer node to " << parent_node << std::endl;
    }

    inline void shift_buffer(size_t elements, lz_trie::factorid_t parent_node) {
        std::cout << "shift buffer by "<<elements<<" elements. before: \"";
        debug_print_buffer(std::cout);
        std::cout << "\", node: " << parent_node;

        size_t remaining = m_jump_width - elements;
        for(size_t i = 0; i < remaining; i++) {
            jump_buffer(i) = jump_buffer(i + elements);
        }
        m_jump_buffer_size -= elements;
        this->set_parent_node(m_jump_buffer_handle, parent_node);

        std::cout << "; after: \"";
        debug_print_buffer(std::cout);
        std::cout << "\", node: " << parent_node;
        std::cout << std::endl;
    }

    inline bool jump_buffer_full() const {
        return m_jump_width == m_jump_buffer_size;
    }

    inline auto find_jump_buffer() const {
        return this->find(m_jump_buffer_handle);
    }

    inline void insert_jump_buffer(value_type const& val) {
        this->insert(m_jump_buffer_handle, val);
    }

    // DEBUG printing function

    inline void debug_print_buffer(std::ostream& out, jump_buffer_handle const& handle, size_t size) const {
        for(size_t i = 0; i < size; i++) {
            out << this->get_buffer(handle)[i];
        }
    }

    inline void debug_print_buffer(std::ostream& out) const {
        debug_print_buffer(out, m_jump_buffer_handle, m_jump_buffer_size);
    }
};

}}
