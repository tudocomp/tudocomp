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

namespace tdc {

template<typename node_t, typename value_type>
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
                std::cout << "word compare at i=" << i << std::endl;
                auto pa = (uint64_t const*) &a[i];
                auto pb = (uint64_t const*) &b[i];
                if (*pa != *pb) return false;
                i += elem_in_word;
            }
            for(; i < m_actual_size; i++) {
                std::cout << "single compare at i=" << i << std::endl;
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
                std::cout << "word hash at i=" << i << std::endl;
                auto pa = (uint64_t const*) &a[i];
                hash_step(*pa);
                i += elem_in_word;
            }
            for(; i < m_actual_size; i++) {
                std::cout << "single hash at i=" << i << std::endl;
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

    inline void set_parent_node(jump_buffer_handle& handle, node_t const& node) {
        handle.node(node.id());
    }

    inline uliteral_t* get_buffer(jump_buffer_handle& handle) {
        return &handle[0];
    }

    struct result_t {
        decltype(m_jump_pointer_map)& m_map;
        typename decltype(m_jump_pointer_map)::const_iterator m_iter;

        inline bool found() const {
            return m_iter != m_map.end();
        }

        inline value_type const& get() {
            DCHECK(found());
            return m_iter->second;
        }
    };
    inline result_t find(jump_buffer_handle const& key) {
        return result_t { m_jump_pointer_map, m_jump_pointer_map.find(key) };
    }
    inline void insert(jump_buffer_handle const& key, value_type const& val) {
        m_jump_pointer_map.insert({key, val});
    }
};

template <typename coder_t, typename dict_t>
class LZ78PointerJumpingCompressor: public Compressor {
private:
    using node_t = typename dict_t::node_t;
    struct NodePair {
        node_t parent;
        node_t node;
    };
    using pointer_jumping_t = FixedBufferPointerJumping<node_t, NodePair>;

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

        dict_t dict(config().sub_config("lz_trie"), n, reserved_size);

        auto reset_dict = [&dict] {
            dict.clear();
            node_t node = dict.add_rootnode(0);
            DCHECK_EQ(node.id(), dict.size() - 1);
            DCHECK_EQ(node.id(), 0U);
        };
        reset_dict();

        typename coder_t::Encoder coder(config().sub_config("coder"), out, NoLiterals());
        auto new_factor = [&](uint64_t node_id, uliteral_t c) {
            lz78::encode_factor(coder, node_id, c, factor_count);
            factor_count++;
            IF_STATS(stat_factor_count++);
        };

        pointer_jumping_t jump_pointer_map(m_jump_width);

        // setup initial state for the node search
        char c;
        typename pointer_jumping_t::jump_buffer_handle c_buf_handle;
        size_t c_buf_size = 0;
         // TODO: checked span?
        auto c_buf = jump_pointer_map.get_buffer(c_buf_handle);

        // set up initial search node
        node_t node = dict.get_rootnode(0);
        node_t parent = node; // parent of node, needed for the last factor
        DCHECK_EQ(node.id(), 0U);
        DCHECK_EQ(parent.id(), 0U);
        jump_pointer_map.set_parent_node(c_buf_handle, node);

        enum next_trie_char_result {
            TRAVERSE_FURTHER,
            NEW_CHILD,
            NEW_CHILD_RESET_DICT
        };

        auto add_char_to_trie = [&](uliteral_t c) {
            // advance trie state with the next read character
            dict.signal_character_read();
            node_t child = dict.find_or_insert(node, static_cast<uliteral_t>(c));

            if(child.is_new()) {
                // we found a leaf, output a factor
                new_factor(node.id(), static_cast<uliteral_t>(c));

                // reset search node
                parent = node = dict.get_rootnode(0);
                DCHECK_EQ(node.id(), 0U);
                DCHECK_EQ(parent.id(), 0U);
                DCHECK_EQ(factor_count+1, dict.size());

                // check if dictionary's maximum size was reached
                // (this will never happen if m_dict_max_size == 0)
                if(tdc_unlikely(dict.size() == m_dict_max_size)) {
                    DCHECK_GT(dict.size(),0U);
                    reset_dict();
                    factor_count = 0;
                    IF_STATS(stat_dictionary_resets++);
                    IF_STATS(stat_dict_counter_at_last_reset = m_dict_max_size);
                }
            } else {
                // traverse further
                parent = node;
                node = child;
            }
        };

        while(is.get(c)) {
            c_buf[c_buf_size] = static_cast<uliteral_t>(c);
            c_buf_size++;
            if (c_buf_size == m_jump_width) {
                auto entry = jump_pointer_map.find(c_buf_handle);
                if (entry.found()) {
                    // we can jump ahead
                    node = entry.get().node;
                    parent = entry.get().parent;
                    c_buf_size = 0;
                } else {
                    // we need to manually add to the trie
                    for(size_t i = 0; i < c_buf_size; i++) {
                        add_char_to_trie(c_buf[i]); // TODO: this does skip over restarts!
                    }
                    jump_pointer_map.insert(c_buf_handle, NodePair{ parent, node });
                    jump_pointer_map.set_parent_node(c_buf_handle, node);
                    c_buf_size = 0;
                }
            }
        }

        // process chars from last incomplete jump buffer
        for(size_t i = 0; i < c_buf_size; i++) {
            add_char_to_trie(c_buf[i]);
        }
        c_buf_size = 0;

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
