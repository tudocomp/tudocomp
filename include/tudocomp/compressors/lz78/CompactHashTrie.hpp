#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>
#include <tudocomp/util/compact_sparse_hash.hpp>
#include <tudocomp/util/compact_hash.hpp>
#include <tudocomp/util/compact_displacement_hash.hpp>
#include <tudocomp/util/compact_sparse_displacement_hash.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lz78 {
namespace ch {
using namespace compact_sparse_hashmap;

template<typename table_t>
class Common {
    table_t m_table;
    size_t m_key_width = 0;
    size_t m_value_width = 0;

    inline size_t key_width(uint64_t key) {
        m_key_width = std::max(m_key_width, size_t(bits_for(key)));
        return m_key_width;
    }
    inline size_t value_width(uint64_t val) {
        m_value_width = std::max(m_value_width, size_t(bits_for(val)));
        return m_value_width;
    }
public:
    inline Common(size_t table_size, double max_load_factor):
        m_table(table_size)
    {
        m_table.max_load_factor(max_load_factor);
    }

    inline size_t size() const {
        return m_table.size();
    }

    inline size_t table_size() {
        return m_table.table_size();
    }

    inline double max_load_factor() {
        return m_table.max_load_factor();
    }

    inline uint64_t insert_kv_width(uint64_t key, uint64_t value) {
        DCHECK_NE(value, 0u);
        auto&& r = m_table.access_kv_width(key,
                                           key_width(key),
                                           value_width(value));
        if (r == 0) {
            r = value;
            return r;
        } else {
            return r;
        }
    }
};

struct Sparse:
    Common<compact_sparse_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m("compact_hash_strategy", "sparse_cv", "Sparse Table with CV structure");
        return m;
    }

    using Common::Common;
};

struct Plain:
    Common<compact_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m("compact_hash_strategy", "plain_cv", "Plain Table with CV structure");
        return m;
    }

    using Common::Common;
};

struct SparseDisplacement:
    Common<compact_sparse_displacement_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m("compact_hash_strategy", "sparse_disp", "Sparse Table with displacement structure");
        return m;
    }

    using Common::Common;
};

struct SparseEliasDisplacement:
    Common<compact_sparse_elias_displacement_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m("compact_hash_strategy", "sparse_elias_disp", "Sparse Table with elias gamma coded displacement structure");
        return m;
    }

    using Common::Common;
};

struct PlainDisplacement:
    Common<compact_displacement_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m("compact_hash_strategy", "plain_disp", "Plain Table with displacement structure");
        return m;
    }

    using Common::Common;
};

struct PlainEliasDisplacement:
    Common<compact_elias_displacement_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m("compact_hash_strategy", "plain_elias_disp", "Plain Table with elias gamma coded displacement structure");
        return m;
    }

    using Common::Common;
};

}

template<typename compact_hash_strategy_t = ch::Sparse>
class CompactHashTrie : public Algorithm, public LZ78Trie<> {
    compact_hash_strategy_t m_table;

public:
    inline static Meta meta() {
        Meta m("lz78trie", "compact_sparse_hash", "Compact Sparse Hash Trie");
        m.option("load_factor").dynamic(50);
        m.option("compact_hash_strategy")
            .templated<compact_hash_strategy_t, ch::Sparse>("compact_hash_strategy");
        return m;
    }

    inline CompactHashTrie(Env&& env, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
        : Algorithm(std::move(env))
        , LZ78Trie(n,remaining_characters)
        , m_table(zero_or_next_power_of_two(reserve), this->env().option("load_factor").as_integer()/100.0f)
    {
    }

    IF_STATS(
        MoveGuard m_guard;
        inline ~CompactHashTrie() {
            if (m_guard) {
                //auto stats = m_table.stat_gather();

                // StatPhase::log("collisions", collisions());
                StatPhase::log("table size", m_table.table_size());
                StatPhase::log("load factor", m_table.max_load_factor());
                StatPhase::log("entries", m_table.size());
                // StatPhase::log("resizes", m_resizes);
                // StatPhase::log("special resizes", m_specialresizes);
                StatPhase::log("load ratio", m_table.size() * 100.0 / m_table.table_size());
                /*
                StatPhase::log("buckets", stats.buckets);
                StatPhase::log("allocated_buckets", stats.allocated_buckets);
                StatPhase::log("buckets_real_allocated_capacity_in_bytes", stats.buckets_real_allocated_capacity_in_bytes);
                StatPhase::log("real_allocated_capacity_in_bytes", stats.real_allocated_capacity_in_bytes);
                StatPhase::log("theoretical_minimum_size_in_bits", stats.theoretical_minimum_size_in_bits);
                */
            }
        }
    )
    CompactHashTrie(CompactHashTrie&& other) = default;
    CompactHashTrie& operator=(CompactHashTrie&& other) = default;

    inline node_t add_rootnode(uliteral_t c) {
        auto key = create_node(0, c);
        auto value = size() + 1;
        bool inserted_val = m_table.insert_kv_width(key, value);
        DCHECK_EQ(inserted_val, value);
        return node_t(value - 1, true);
    }

    inline node_t get_rootnode(uliteral_t c) const {
        return node_t(c, false);
    }

    inline void clear() {
        // m_table.clear();
    }

    inline node_t find_or_insert(const node_t& parent_w, uliteral_t c) {
        auto parent = parent_w.id();

        // if we add a new node, its index will be equal to the current size of the dictionary
        const factorid_t newleaf_id = size() + 1;

        // Use 0 as special value for map lookup
        // if val == 0, then it got default constructed, which means
        // it is new
        // could also do it by the handler mechanism if this turns out to be a problem
        DCHECK_NE(newleaf_id, 0u);

        auto key = create_node(parent,c);
        auto val = m_table.insert_kv_width(key, newleaf_id);
        bool is_new = (val == newleaf_id);

        return node_t(val - 1, is_new);
    }

    inline size_t size() const {
        return m_table.size();
    }

    inline void debug_print() {
        // std::cout << m_table.debug_state() << "\n";
    }
};

}} //ns
