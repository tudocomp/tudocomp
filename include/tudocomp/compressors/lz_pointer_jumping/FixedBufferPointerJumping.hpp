#pragma once

#include <unordered_map>
#include <array>

#include <tudocomp/compressors/lz_trie/LZTrie.hpp>

namespace tdc {namespace lz_pointer_jumping {

template<typename value_type>
class FixedBufferPointerJumping {
public:
    static const size_t MAX_JUMP_WIDTH = 17;
private:

    struct alignas(uint64_t) SmallFixedString {
        std::array<uliteral_t, MAX_JUMP_WIDTH> m_data;
        lz_trie::factorid_t m_node;

        inline SmallFixedString(uliteral_t const* ptr,
                                size_t size,
                                lz_trie::factorid_t node) {
            DCHECK_LE(size, m_data.size());
            for (size_t i = 0; i < size; i++) {
                m_data[i] = ptr[i];
            }
            for (size_t i = size; i < m_data.size(); i++) {
                m_data[i] = 0;
            }
            m_node = node;
        }
        inline SmallFixedString(): SmallFixedString(nullptr, 0, 0) {}
        inline uliteral_t const& operator[](size_t i) const {
            return m_data[i];
        }
        inline uliteral_t& operator[](size_t i) {
            return m_data[i];
        }
        inline lz_trie::factorid_t node() const { return m_node; }
        inline void node(lz_trie::factorid_t n) { m_node = n; }
    };
    struct SmallFixedStringEq {
        size_t m_actual_size = -1;

        inline bool operator()(SmallFixedString const& a,
                               SmallFixedString const& b) const {
            DCHECK_LE(m_actual_size, MAX_JUMP_WIDTH);
            DCHECK_EQ(sizeof(uint64_t) % sizeof(uliteral_t), 0);
            size_t const elem_in_word = sizeof(uint64_t) / sizeof(uliteral_t);

            if (a.node() != b.node()) return false;

            // compare elements word-wise to speed things up
            size_t i = 0;
            while(i + elem_in_word <= m_actual_size) {
                //std::cout << "word compare at i=" << i << std::endl;
                auto pa = (uint64_t const*) &a[i];
                auto pb = (uint64_t const*) &b[i];
                if (*pa != *pb) return false;
                i += elem_in_word;
            }
            for(; i < m_actual_size; i++) {
                //std::cout << "single compare at i=" << i << std::endl;
                if (a[i] != b[i]) return false;
            }

            return true;
        }
    };
    struct SmallFixedStringHash {
        size_t m_actual_size = -1;

        inline size_t operator()(SmallFixedString const& a) const {
            DCHECK_LE(m_actual_size, MAX_JUMP_WIDTH);
            DCHECK_EQ(sizeof(uint64_t) % sizeof(uliteral_t), 0);
            size_t const elem_in_word = sizeof(uint64_t) / sizeof(uliteral_t);

            size_t seed = 0;
            auto hash_step = [&](uint64_t v) {
                // hash combine formular stolen from boost
                size_t hsh = std::hash<uint64_t>()(v);
                seed ^= hsh + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };

            hash_step(a.node());

            // hash elements word-wise to speed things up
            size_t i = 0;
            while(i + elem_in_word <= m_actual_size) {
                //std::cout << "word hash at i=" << i << std::endl;
                auto pa = (uint64_t const*) &a[i];
                hash_step(*pa);
                i += elem_in_word;
            }
            for(; i < m_actual_size; i++) {
                //std::cout << "single hash at i=" << i << std::endl;
                hash_step(a[i]);
            }

            return seed;
        }
    };
    std::unordered_map<
        SmallFixedString,
        value_type,
        SmallFixedStringHash,
        SmallFixedStringEq
    > m_jump_pointer_map;

public:
    using jump_buffer_handle = SmallFixedString;

    FixedBufferPointerJumping(size_t jump_width):
        m_jump_pointer_map(0, SmallFixedStringHash{jump_width}, SmallFixedStringEq{jump_width}) {}

    inline void set_parent_node(jump_buffer_handle& handle, lz_trie::factorid_t node) {
        handle.node(node);
    }
    inline lz_trie::factorid_t get_parent_node(jump_buffer_handle& handle) const {
        return handle.node();
    }

    inline uliteral_t* get_buffer(jump_buffer_handle& handle) {
        return &handle[0];
    }
    inline uliteral_t const* get_buffer(jump_buffer_handle const& handle) const {
        return &handle[0];
    }

    struct result_t {
        decltype(m_jump_pointer_map) const* m_map;
        typename decltype(m_jump_pointer_map)::const_iterator m_iter;

        inline bool found() const {
            return m_iter != m_map->end();
        }

        inline value_type const& get() {
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
    inline void insert(jump_buffer_handle const& key, value_type const& val) {
        m_jump_pointer_map.insert({key, val});
    }
};

}}
