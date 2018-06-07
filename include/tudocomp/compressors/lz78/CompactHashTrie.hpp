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
    inline Common() = default;
    inline Common(size_t table_size,
                  double max_load_factor,
                  uint64_t key_width = table_t::DEFAULT_KEY_WIDTH,
                  uint64_t value_width = table_t::DEFAULT_VALUE_WIDTH):
        m_table(table_size, key_width, value_width)
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

    inline uint64_t key_width() {
        return m_table.key_width();
    }

    inline uint64_t value_width() {
        return m_table.value_width();
    }

    inline uint64_t insert(uint64_t key, uint64_t value) {
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

    inline uint64_t search(uint64_t key) {
        using ptr_t = typename table_t::pointer_type;

        auto p = m_table.search(key);
        if (p != ptr_t()) {
            uint64_t v = *p;
            DCHECK_NE(v, 0);
            return v;
        } else {
            return 0;
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

template<typename compact_hash_strategy_t>
class NoKVGrow {
    size_t m_initial_table_size;
    double m_max_load_factor;

    // We map (key_width, value_width) to hashmap
    static constexpr size_t min_bits = 1;
    using val_bit_tables_t = std::vector<compact_hash_strategy_t>;
    using key_bit_tables_t = std::vector<val_bit_tables_t>;
    key_bit_tables_t m_tables;
    size_t m_overall_size = 0;
    size_t m_overall_table_size = 0;

public:
    inline static Meta meta() {
        Meta m("compact_hash_strategy", "no_kv_grow", "Adapter that does not grow the bit widths of keys and values, but rather creates additional hash tables as needed.");
        m.option("compact_hash_strategy")
            .templated<compact_hash_strategy_t>("compact_hash_strategy");
        return m;
    }

    inline NoKVGrow(size_t table_size, double max_load_factor):
        m_initial_table_size(table_size),
        m_max_load_factor(max_load_factor)
    {
    }

    inline size_t size() const {
        return m_overall_size;
    }

    inline size_t table_size() {
        return m_overall_table_size;
    }

    inline double max_load_factor() {
        return m_max_load_factor;
    }

    inline static uint64_t my_bits_for(uint64_t v) {
        if (v == 0) return 0;
        return bits_for(v);
    }

    inline uint64_t insert(uint64_t key, uint64_t value) {
        constexpr bool reduce_key_range = true;
        constexpr bool reduce_value_range = false;

        auto min_bit_sat = [min_bits](uint64_t bits) {
            if (bits > min_bits) {
                return bits - min_bits;
            } else {
                return uint64_t(0);
            }
        };

        // Grow by-key bit index as needed
        uint16_t key_bits = bits_for(key);
        while (min_bit_sat(key_bits) >= m_tables.size()) {
            m_tables.push_back(val_bit_tables_t());
        }
        auto& val_bits_tables = m_tables[min_bit_sat(key_bits)];

        // Grow by-val bit index as needed
        uint16_t value_bits = bits_for(value);
        while (min_bit_sat(value_bits) >= val_bits_tables.size()) {
            uint64_t this_val_bits = val_bits_tables.size() + min_bits;

            auto t = compact_hash_strategy_t(m_initial_table_size,
                                             m_max_load_factor,
                                             key_bits - (key_bits > 0 && reduce_key_range),
                                             this_val_bits  - (this_val_bits > 0 && reduce_value_range));
            m_overall_size += t.size();
            m_overall_table_size += t.table_size();

            val_bits_tables.push_back(std::move(t));
        }

        auto min_val = [](uint64_t bits) {
            return bits <= 1 ? 0 : 1ull << (bits - 1);
        };
        auto max_val = [](uint64_t bits) {
            return (1ull << bits) - 1;
        };

        uint64_t key_min = min_val(key_bits);
        uint64_t key_max = max_val(key_bits);
        DCHECK_GE(key, key_min);
        DCHECK_LE(key, key_max);

        uint64_t value_min = min_val(value_bits);
        uint64_t value_max = max_val(value_bits);
        DCHECK_GE(value, value_min);
        DCHECK_LE(value, value_max);

        std::cout << "insert: "
            << "key(" << key_bits << ", " << key_min << ".." << key << ".." << key_max << "), "
            << "value(" << value_bits << ", " << value_min << ".." << value << ".." << value_max << ")"
            << "\n";

        if (reduce_key_range) {
            key -= key_min;
        }

        if (reduce_value_range) {
            value -= value_min;
        }

        for (size_t i = min_bits; i < value_bits; i++) {
            uint64_t v = val_bits_tables[i - min_bits].search(key);
            if (v != 0) {
                if (reduce_value_range) {
                    v += min_val(i);
                }
                return v;
            }
        }

        auto& t = val_bits_tables[value_bits - min_bits];
        m_overall_size -= t.size();
        m_overall_table_size -= t.table_size();
        auto r = t.insert(key, value);
        m_overall_size += t.size();
        m_overall_table_size += t.table_size();

        if (reduce_value_range) {
            r += value_min;
        }
        return r;
    }

    inline NoKVGrow(NoKVGrow&&) = default;
    inline NoKVGrow& operator=(NoKVGrow&&) = default;
    inline ~NoKVGrow() {
        if (m_guard) {
            std::cout << "Tables:\n";
            for (size_t i = 0; i < m_tables.size(); i++) {
                std::cout << "  KeyBits(" << (i + min_bits) << "):\n";
                auto& t = m_tables[i];
                for (size_t j = 0; j < t.size(); j++) {
                    std::cout << "    ValBits(" << (j + min_bits) << "): size/tsize(";
                    auto& t2 = t[j];
                    std::cout
                    << t2.size()
                    << "/"
                    << t2.table_size()
                    << ")"
                    << ", kvsize("
                    << t2.key_width()
                    << ","
                    << t2.value_width()
                    << ")"
                    << "\n";
                }
            }
        }
    }
private:
    MoveGuard m_guard;
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
        bool inserted_val = m_table.insert(key, value);
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
        auto val = m_table.insert(key, newleaf_id);
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
