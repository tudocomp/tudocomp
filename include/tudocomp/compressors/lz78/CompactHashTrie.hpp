#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>
#include <tudocomp/util/compact_hash/map/typedefs.hpp>
#include <tudocomp/util.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lz78 {

constexpr TypeDesc compact_hash_strategy_type() {
    return TypeDesc("compact_hash_strategy");
}

namespace ch {
using namespace compact_hash;
using namespace compact_hash::map;

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
    using config_args = typename table_t::config_args;
    inline static void set_config_from_tdc(Config&& tdc_config, config_args& config) {
        // nothing to be done per default
    }

    inline Common() = default;
    inline Common(size_t table_size,
                  config_args config,
                  uint64_t key_width = table_t::DEFAULT_KEY_WIDTH,
                  uint64_t value_width = table_t::DEFAULT_VALUE_WIDTH):
        m_table(table_size, key_width, value_width, config)
    {
    }

    inline size_t size() const {
        return m_table.size();
    }

    inline size_t table_size() {
        return m_table.table_size();
    }

    inline double max_load_factor() {
        return m_table.current_config().size_manager_config.load_factor;
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
        if (r == 0u) {
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
            DCHECK_NE(v, 0U);
            return v;
        } else {
            return 0;
        }
    }
};

struct Sparse:
    Common<sparse_cv_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m(compact_hash_strategy_type(), "sparse_cv", "Sparse Table with CV structure");
        return m;
    }

    using Common::Common;
};

struct Plain:
    Common<plain_cv_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m(compact_hash_strategy_type(), "plain_cv", "Plain Table with CV structure");
        return m;
    }

    using Common::Common;
};

struct SparseDisplacement:
    Common<sparse_layered_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m(compact_hash_strategy_type(), "sparse_disp", "Sparse Table with displacement structure");
        m.param("layered_bit_width",
            "The number of bits aviable per displacement entry until they get spilled"
            " to a hashamp"
        ).primitive(4);
        return m;
    }

    inline static void set_config_from_tdc(Config&& tdc_config, config_args& config) {
        config.displacement_config.table_config.bit_width_config.width
            = tdc_config.param("layered_bit_width").as_int();
    }
    using Common::Common;
};

struct PlainDisplacement:
    Common<plain_layered_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m(compact_hash_strategy_type(), "plain_disp", "Plain Table with displacement structure");
        m.param("layered_bit_width",
            "The number of bits aviable per displacement entry until they get spilled"
            " to a hashamp"
        ).primitive(4);
        return m;
    }

    inline static void set_config_from_tdc(Config&& tdc_config, config_args& config) {
        config.displacement_config.table_config.bit_width_config.width
            = tdc_config.param("layered_bit_width").as_int();
    }
    using Common::Common;
};

struct SparseEliasDisplacement:
    Common<sparse_elias_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m(compact_hash_strategy_type(), "sparse_elias_disp", "Sparse Table with elias gamma coded displacement structure");
        m.param("elias_bucket_size",
            "Size of the buckets used to store the elias coded displacement values"
        ).primitive(1024);
        return m;
    }

    inline static void set_config_from_tdc(Config&& tdc_config, config_args& config) {
        config.displacement_config.table_config.bucket_size_config.bucket_size
            = tdc_config.param("elias_bucket_size").as_int();
    }
    using Common::Common;
};

struct PlainEliasDisplacement:
    Common<plain_elias_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m(compact_hash_strategy_type(), "plain_elias_disp", "Plain Table with elias gamma coded displacement structure");
        m.param("elias_bucket_size",
            "Size of the buckets used to store the elias coded displacement values"
        ).primitive(1024);
        return m;
    }

    inline static void set_config_from_tdc(Config&& tdc_config, config_args& config) {
        config.displacement_config.table_config.bucket_size_config.bucket_size
            = tdc_config.param("elias_bucket_size").as_int();
    }
    using Common::Common;
};

template<typename val_t, typename hash_t = poplar_xorshift_t>
using plain_elias_growing_hashmap_t
    = hashmap_t<
        val_t, hash_t, plain_sentinel_t,
        displacement_t<elias_gamma_displacement_table_t<
            growing_elias_gamma_bucket_size_t>>>;

template<typename val_t, typename hash_t = poplar_xorshift_t>
using sparse_elias_growing_hashmap_t
    = hashmap_t<
        val_t, hash_t, buckets_bv_t,
        displacement_t<elias_gamma_displacement_table_t<
            growing_elias_gamma_bucket_size_t>>>;

struct SparseEliasGrowingDisplacement:
    Common<sparse_elias_growing_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m(compact_hash_strategy_type(), "sparse_elias_growing_disp", "Sparse Table with elias gamma coded displacement structure");
        m.param("elias_bucket_growth_factor",
            "Bucket sizes grow according to the table size via the formular "
            "`std::pow(std::log2(table_size), F)`, where `F` is the factor `3.0/2.0` per default"
        ).primitive(3.0/2.0);
        return m;
    }

    inline static void set_config_from_tdc(Config&& tdc_config, config_args& config) {
        config.displacement_config.table_config.bucket_size_config.factor
            = tdc_config.param("elias_bucket_growth_factor").as_float();
    }
    using Common::Common;
};

struct PlainEliasGrowingDisplacement:
    Common<plain_elias_growing_hashmap_t<dynamic_t>>
{
    inline static Meta meta() {
        Meta m(compact_hash_strategy_type(), "plain_elias_growing_disp", "Plain Table with elias gamma coded displacement structure");
        m.param("elias_bucket_growth_factor",
            "Bucket sizes grow according to the table size via the formular "
            "`std::pow(std::log2(table_size), F)`, where `F` is the factor `3.0/2.0` per default"
        ).primitive(3.0/2.0);
        return m;
    }

    inline static void set_config_from_tdc(Config&& tdc_config, config_args& config) {
        config.displacement_config.table_config.bucket_size_config.factor
            = tdc_config.param("elias_bucket_growth_factor").as_float();
    }
    using Common::Common;
};

template<typename compact_hash_strategy_t>
class NoKVGrow {
public:
    using config_args = typename compact_hash_strategy_t::config_args;
private:

    size_t m_initial_table_size;
    config_args m_config;

    // We map (key_width, value_width) to hashmap
    static constexpr size_t MIN_BITS = 1;
    template<typename T>
    struct width_bucket_t {
        std::vector<T> m_elements;

        template<typename grow_t>
        T& access(size_t i, grow_t f) {
            DCHECK_GE(i, MIN_BITS);
            while((i - MIN_BITS) >= m_elements.size()) {
                m_elements.push_back(f(m_elements.size() + MIN_BITS));
            }
            return m_elements[i - MIN_BITS];
        }

        T& operator[](size_t i) {
            DCHECK_GE(i, MIN_BITS);
            return m_elements.at(i - MIN_BITS);
        }

        inline size_t min_index() const {
            return MIN_BITS;
        }

        inline size_t size() const {
            return m_elements.size() + MIN_BITS;
        }

        width_bucket_t() = default;
    };

    using key_tables_t = width_bucket_t<compact_hash_strategy_t>;
    key_tables_t m_key_tables;
    size_t m_overall_size = 0;
    size_t m_overall_table_size = 0;

public:
    inline static void set_config_from_tdc(Config&& tdc_config, config_args& config) {
        compact_hash_strategy_t::set_config_from_tdc(tdc_config.sub_config("compact_hash_strategy"), config);
    }

    inline static Meta meta() {
        Meta m(compact_hash_strategy_type(), "no_k_grow", "Adapter that does not grow the bit widths of keys, but rather creates additional hash tables as needed.");
        m.param("compact_hash_strategy")
            .strategy<compact_hash_strategy_t>(TypeDesc("compact_hash_strategy"));
        return m;
    }

    inline NoKVGrow(size_t table_size, config_args config):
        m_initial_table_size(table_size),
        m_config(config)
    {
    }

    inline size_t size() const {
        return m_overall_size;
    }

    inline size_t table_size() {
        return m_overall_table_size;
    }

    inline double max_load_factor() {
        return m_config.size_manager_config.load_factor;
    }

    inline static uint64_t my_bits_for(uint64_t v) {
        if (v == 0) return 0;
        return bits_for(v);
    }

    inline uint64_t insert(uint64_t key, uint64_t value) {
        constexpr bool reduce_key_range = true;

        // Grow by-key bit index as needed
        uint16_t key_bits = bits_for(key);
        auto&& table = m_key_tables.access(key_bits, [&](uint64_t bits) {
            compact_hash_strategy_t x {
                m_initial_table_size,
                m_config,
                key_bits - uint64_t(key_bits > 0 && reduce_key_range),
            };
            m_overall_size += x.size();
            m_overall_table_size += x.table_size();
            return x;
        });

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

        if (reduce_key_range) {
            key -= key_min;
        }

        m_overall_size -= table.size();
        m_overall_table_size -= table.table_size();
        auto r = table.insert(key, value);
        m_overall_size += table.size();
        m_overall_table_size += table.table_size();

        return r;
    }
/*
    inline NoKVGrow(NoKVGrow&&) = default;
    inline NoKVGrow& operator=(NoKVGrow&&) = default;
    inline ~NoKVGrow() {
        if (m_guard) {
            std::cout << "Tables:\n";
            for (size_t i = m_key_tables.min_index(); i < m_key_tables.size(); i++) {
                std::cout << "  KeyBits(" << (i + 0) << "):\n";
                auto& val_tables = m_key_tables[i];
                for (size_t j = val_tables.min_index(); j < val_tables.size(); j++) {
                    std::cout << "    ValBits(" << (j + 0) << "): size/tsize(";
                    auto& table = val_tables[j];
                    std::cout
                    << table.size()
                    << "/"
                    << table.table_size()
                    << ")"
                    << ", kvsize("
                    << table.key_width()
                    << ","
                    << table.value_width()
                    << ")"
                    << "\n";
                }
            }

            size_t mx = 45;
            DebugTableFormatter fmt;

            fmt.put(0, 0, 0);
            for(size_t i = 1; i < mx; i++) {
                fmt.put(i, 0, i);
                fmt.put(space_value(i), 1, i);
                fmt.put(unspace_value(space_value(i)), 2, i);
            }
            std::cout << fmt.print(mx, 4) << "\n";
        }
    }
private:
    MoveGuard m_guard;
*/
};

template<typename compact_hash_strategy_t>
class NoKGrow {
public:
    using config_args = typename compact_hash_strategy_t::config_args;
private:

    size_t m_initial_table_size;
    config_args m_config;

    // We map (key_width, value_width) to hashmap
    static constexpr size_t MIN_BITS = 1;
    template<typename T>
    struct width_bucket_t {
        std::vector<T> m_elements;

        template<typename grow_t>
        T& access(size_t i, grow_t f) {
            DCHECK_GE(i, MIN_BITS);
            while((i - MIN_BITS) >= m_elements.size()) {
                m_elements.push_back(f(m_elements.size() + MIN_BITS));
            }
            return m_elements[i - MIN_BITS];
        }

        T& operator[](size_t i) {
            DCHECK_GE(i, MIN_BITS);
            return m_elements.at(i - MIN_BITS);
        }

        inline size_t min_index() const {
            return MIN_BITS;
        }

        inline size_t size() const {
            return m_elements.size() + MIN_BITS;
        }

        width_bucket_t() = default;
    };

    using val_tables_t = width_bucket_t<compact_hash_strategy_t>;
    using key_tables_t = width_bucket_t<val_tables_t>;
    key_tables_t m_key_tables;
    size_t m_overall_size = 0;
    size_t m_overall_table_size = 0;

public:
    inline static void set_config_from_tdc(Config&& tdc_config, config_args& config) {
        compact_hash_strategy_t::set_config_from_tdc(tdc_config.sub_config("compact_hash_strategy"), config);
    }

    inline static Meta meta() {
        Meta m(compact_hash_strategy_type(), "no_kv_grow", "Adapter that does not grow the bit widths of keys and values, but rather creates additional hash tables as needed.");
        m.param("compact_hash_strategy")
            .strategy<compact_hash_strategy_t>(TypeDesc("compact_hash_strategy"));
        return m;
    }

    inline NoKGrow(size_t table_size, config_args config):
        m_initial_table_size(table_size),
        m_config(config)
    {
    }

    inline size_t size() const {
        return m_overall_size;
    }

    inline size_t table_size() {
        return m_overall_table_size;
    }

    inline double max_load_factor() {
        return m_config.size_manager_config.load_factor;
    }

    inline static uint64_t my_bits_for(uint64_t v) {
        if (v == 0) return 0;
        return bits_for(v);
    }

    // skip every power of two, except 0
    inline uint64_t space_value(uint64_t value) {
        uint64_t extra_bits = 0;
        uint64_t bits = 0;

        do {
            extra_bits = bits_for(value) - bits - 1;
            value += extra_bits;
            bits += extra_bits;
        } while (extra_bits != 0);

        return value;
    }

    // unskip every power of two, except 0
    inline uint64_t unspace_value(uint64_t value) {
        return value + 1 - bits_for(value);
    }

    inline uint64_t insert(uint64_t key, uint64_t value) {
        value = space_value(value);

        constexpr bool reduce_key_range = true;
        constexpr bool reduce_value_range = true;

        // Grow by-key bit index as needed
        uint16_t key_bits = bits_for(key);
        auto&& val_tables = m_key_tables.access(key_bits, [](uint64_t bits) {
            return val_tables_t();
        });

        // Grow by-val bit index as needed
        uint16_t value_bits = bits_for(value);
        auto&& table = val_tables.access(value_bits, [&](uint64_t bits) {
            compact_hash_strategy_t x {
                m_initial_table_size,
                m_config,
                key_bits - uint64_t(key_bits > 0 && reduce_key_range),
                bits - uint64_t(bits > 0 && reduce_value_range),
            };
            m_overall_size += x.size();
            m_overall_table_size += x.table_size();
            return x;
        });

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

        /*
        std::cout << "insert: "
            << "key(" << key_bits << ", " << key_min << ".." << key << ".." << key_max << "), "
            << "value(" << value_bits << ", " << value_min << ".." << value << ".." << value_max << ")"
            << "\n";
        */

        if (reduce_key_range) {
            key -= key_min;
        }

        if (reduce_value_range) {
            value -= value_min;
        }

        for (size_t i = val_tables.min_index(); i < value_bits; i++) {
            uint64_t v = val_tables[i].search(key);
            if (v != 0) {
                if (reduce_value_range) {
                    v += min_val(i);
                }
                return unspace_value(v);
            }
        }

        m_overall_size -= table.size();
        m_overall_table_size -= table.table_size();
        auto r = table.insert(key, value);
        m_overall_size += table.size();
        m_overall_table_size += table.table_size();

        if (reduce_value_range) {
            r += value_min;
        }
        return unspace_value(r);
    }
/*
    inline NoKGrow(NoKGrow&&) = default;
    inline NoKGrow& operator=(NoKGrow&&) = default;
    inline ~NoKGrow() {
        if (m_guard) {
            std::cout << "Tables:\n";
            for (size_t i = m_key_tables.min_index(); i < m_key_tables.size(); i++) {
                std::cout << "  KeyBits(" << (i + 0) << "):\n";
                auto& val_tables = m_key_tables[i];
                for (size_t j = val_tables.min_index(); j < val_tables.size(); j++) {
                    std::cout << "    ValBits(" << (j + 0) << "): size/tsize(";
                    auto& table = val_tables[j];
                    std::cout
                    << table.size()
                    << "/"
                    << table.table_size()
                    << ")"
                    << ", kvsize("
                    << table.key_width()
                    << ","
                    << table.value_width()
                    << ")"
                    << "\n";
                }
            }

            size_t mx = 45;
            DebugTableFormatter fmt;

            fmt.put(0, 0, 0);
            for(size_t i = 1; i < mx; i++) {
                fmt.put(i, 0, i);
                fmt.put(space_value(i), 1, i);
                fmt.put(unspace_value(space_value(i)), 2, i);
            }
            std::cout << fmt.print(mx, 4) << "\n";
        }
    }
private:
    MoveGuard m_guard;
*/
};

}

template<typename compact_hash_strategy_t = ch::Sparse>
class CompactHashTrie : public Algorithm, public LZ78Trie<> {
    compact_hash_strategy_t m_table;
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

    inline static typename compact_hash_strategy_t::config_args init_config(
        Config&& tdc_config,
        float load_factor
    ) {
        auto r = typename compact_hash_strategy_t::config_args {};
        r.size_manager_config.load_factor = load_factor;
        compact_hash_strategy_t::set_config_from_tdc(std::move(tdc_config), r);
        return r;
    }

public:
    inline static Meta meta() {
        Meta m(lz78_trie_type(), "compact_sparse_hash", "Compact Sparse Hash Trie");
        m.param("load_factor").primitive(50);
        m.param("compact_hash_strategy").strategy<compact_hash_strategy_t>(
            compact_hash_strategy_type(), Meta::Default<ch::Sparse>());
        return m;
    }

    inline CompactHashTrie(Config&& cfg, size_t n, factorid_t reserve = 0)
        : Algorithm(std::move(cfg))
        , LZ78Trie(n)
        , m_table(zero_or_next_power_of_two(reserve),
                  init_config(this->config().sub_config("compact_hash_strategy"),
                              this->config().param("load_factor").as_float()/100.0f))
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
        uint64_t inserted_val = m_table.insert(key, value);
        //std::cout << "add_root::insert("<<key<<","<<value<<") -> " << inserted_val << "\n";
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

        auto key = create_node(parent+1,c);
        auto val = m_table.insert(key, newleaf_id);
        //std::cout << "find_or_insert::insert("<<key<<","<<newleaf_id<<") -> " << val << "\n";
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
