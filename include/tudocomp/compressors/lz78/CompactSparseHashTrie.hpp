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

struct Sparse {
    inline static Meta meta() {
        Meta m("compact_hash_strategy", "sparse_cv", "Sparse Table with CV structure");
        return m;
    }

    using table_t = compact_sparse_hashtable::compact_sparse_hashtable_t<dynamic_t>;
};

struct Plain {
    inline static Meta meta() {
        Meta m("compact_hash_strategy", "plain_cv", "Plain Table with CV structure");
        return m;
    }

    using table_t = compact_sparse_hashtable::compact_hashtable_t<dynamic_t>;
};

struct SparseDisplacement {
    inline static Meta meta() {
        Meta m("compact_hash_strategy", "sparse_disp", "Sparse Table with displacement structure");
        return m;
    }

    using table_t = compact_sparse_hashtable::compact_sparse_displacement_hashtable_t<dynamic_t>;
};

struct SparseEliasDisplacement {
    inline static Meta meta() {
        Meta m("compact_hash_strategy", "sparse_elias_disp", "Sparse Table with elias gamma coded displacement structure");
        return m;
    }

    using table_t = compact_sparse_hashtable::compact_sparse_elias_displacement_hashtable_t<dynamic_t>;
};

struct PlainDisplacement {
    inline static Meta meta() {
        Meta m("compact_hash_strategy", "plain_disp", "Plain Table with displacement structure");
        return m;
    }

    using table_t = compact_sparse_hashtable::compact_displacement_hashtable_t<dynamic_t>;
};

struct PlainEliasDisplacement {
    inline static Meta meta() {
        Meta m("compact_hash_strategy", "plain_elias_disp", "Plain Table with elias gamma coded displacement structure");
        return m;
    }

    using table_t = compact_sparse_hashtable::compact_elias_displacement_hashtable_t<dynamic_t>;
};

template<typename compact_hash_strategy_t = Sparse>
class CompactSparseHashTrie : public Algorithm, public LZ78Trie<> {
    using table_t = typename compact_hash_strategy_t::table_t;
    using ref_t = typename table_t::reference_type;

    table_t m_table;
    //std::unordered_map<uint64_t, factorid_t> m_table;
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
    inline static Meta meta() {
        Meta m("lz78trie", "compact_sparse_hash", "Compact Sparse Hash Trie");
        m.option("load_factor").dynamic(50);
        m.option("compact_hash_strategy")
            .templated<compact_hash_strategy_t, Sparse>("compact_hash_strategy");
        return m;
    }

    inline CompactSparseHashTrie(Env&& env, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
        : Algorithm(std::move(env))
        , LZ78Trie(n,remaining_characters)
        , m_table(zero_or_next_power_of_two(reserve))
    {
        m_table.max_load_factor(this->env().option("load_factor").as_integer()/100.0f );
    }

    IF_STATS(
        MoveGuard m_guard;
        inline ~CompactSparseHashTrie() {
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
    CompactSparseHashTrie(CompactSparseHashTrie&& other) = default;
    CompactSparseHashTrie& operator=(CompactSparseHashTrie&& other) = default;

    inline node_t add_rootnode(uliteral_t c) {
        auto key = create_node(0, c);
        auto value = size() + 1;

        ref_t entry = m_table.access_kv_width(key, key_width(key), value_width(value));

        //std::cout << "find_or_insert(" << key << ", " << entry << ", " << value << ");\n";

        DCHECK_NE(value, 0u);

        entry = value;
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
        ref_t val = m_table.access_kv_width(key, key_width(key), value_width(newleaf_id));
        if (val == 0u) {
            val = newleaf_id;
            DCHECK_EQ(val, newleaf_id);
            return node_t(val - 1, true);
        } else {
            return node_t(val - 1, false);
        }
    }

    inline size_t size() const {
        return m_table.size();
    }

    inline void debug_print() {
        // std::cout << m_table.debug_state() << "\n";
    }
};

}} //ns
