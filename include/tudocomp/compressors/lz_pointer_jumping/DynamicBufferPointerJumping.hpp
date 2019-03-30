#pragma once

#include <unordered_map>
#include <array>

#include <tudocomp/compressors/lz_trie/LZTrie.hpp>

namespace tdc {namespace lz_pointer_jumping {

template<typename lz_state_type>
class DynamicBufferPointerJumping {
public:
    static const size_t MAX_JUMP_WIDTH = 17;
    using lz_state_t = lz_state_type;
    using traverse_state_t = typename lz_state_t::traverse_state_t;
private:
    using jump_id_t = uint32_t;
    struct CustomEq {
        DynamicBufferPointerJumping* m_self = nullptr;

        inline bool operator()(jump_id_t const& a_id,
                               jump_id_t const& b_id) const {
            size_t jump_width = m_self->m_jump_width;
            auto a = m_self->get_buffer(a_id);
            auto b = m_self->get_buffer(b_id);

            if (m_self->get_parent_node(a_id) != m_self->get_parent_node(b_id)) {
                return false;
            }

            // compare elements word-wise to speed things up
            size_t i = 0;
            for(; i < jump_width; i++) {
                if (a[i] != b[i]) return false;
            }

            return true;
        }
    };
    struct CustomHash {
        DynamicBufferPointerJumping* m_self = nullptr;

        inline size_t operator()(jump_id_t const& a_id) const {
            size_t jump_width = m_self->m_jump_width;
            auto a = m_self->get_buffer(a_id);

            size_t seed = 0;
            auto hash_step = [&](uint64_t v) {
                // hash combine formular stolen from boost
                size_t hsh = std::hash<uint64_t>()(v);
                seed ^= hsh + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };

            hash_step(m_self->get_parent_node(a_id));

            // hash elements word-wise to speed things up
            size_t i = 0;
            for(; i < jump_width; i++) {
                hash_step(a[i]);
            }

            return seed;
        }
    };

    size_t m_jump_width;

    std::unordered_map<
        jump_id_t,
        traverse_state_t,
        CustomHash,
        CustomEq
    > m_jump_pointer_map;
    std::vector<uliteral_t> m_key_strings;
    std::vector<lz_trie::factorid_t> m_key_lz_factors;

    jump_id_t m_working_buffer;

    inline jump_id_t alloc_new_key() {
        size_t ret = m_key_lz_factors.size();
        for (size_t i = 0; i < m_jump_width; i++) {
            m_key_strings.push_back(0);
        }
        m_key_lz_factors.push_back(0);
        return ret;
    }

public:
    using jump_buffer_handle = jump_id_t;

    DynamicBufferPointerJumping(size_t jump_width):
        m_jump_width(jump_width),
        m_jump_pointer_map(1, CustomHash{this}, CustomEq{this}),
        m_working_buffer(alloc_new_key())
    {
    }

    inline void set_parent_node(jump_buffer_handle& handle, lz_trie::factorid_t node) {
        m_key_lz_factors[handle] = node;
    }
    inline lz_trie::factorid_t get_parent_node(jump_buffer_handle const& handle) const {
        return m_key_lz_factors[handle];
    }

    inline uliteral_t* get_buffer(jump_buffer_handle& handle) {
        return &m_key_strings[handle * m_jump_width];
    }
    inline uliteral_t const* get_buffer(jump_buffer_handle const& handle) const {
        return &m_key_strings[handle * m_jump_width];
    }

    struct result_t {
        decltype(m_jump_pointer_map) const* m_map;
        typename decltype(m_jump_pointer_map)::const_iterator m_iter;

        inline bool found() const {
            return m_iter != m_map->end();
        }

        inline traverse_state_t const& get() const {
            DCHECK(found());
            return m_iter->second;
        }

        inline result_t() = default;
        inline result_t(decltype(m_jump_pointer_map) const& map,
                        typename decltype(m_jump_pointer_map)::const_iterator iter):
            m_map(&map), m_iter(iter)
        {}
    };
    inline result_t find(jump_buffer_handle const& key) const {
        return result_t { m_jump_pointer_map, m_jump_pointer_map.find(key) };
    }
    inline void insert(jump_buffer_handle const& key, traverse_state_t const& val) {
        // copy the data from jump pointer key points at into a new allocation,
        // and insert a handle to that

        jump_buffer_handle new_key = alloc_new_key();

        set_parent_node(new_key, get_parent_node(key));
        uliteral_t const* from = get_buffer(key);
        uliteral_t* to = get_buffer(new_key);
        for(size_t i = 0; i < m_jump_width; i++) {
            to[i] = from[i];
        }

        m_jump_pointer_map.insert({new_key, val});
    }
    inline jump_buffer_handle get_working_buffer() const {
        return m_working_buffer;
    }
};

}}
