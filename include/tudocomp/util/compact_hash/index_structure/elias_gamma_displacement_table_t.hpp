#pragma once

#include <limits>
#include <unordered_map>
#include <type_traits>
#include <cmath>

#include <tudocomp/util/bit_packed_layout_t.hpp>
#include <tudocomp/util/int_coder.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/IntPtr.hpp>

#include <tudocomp/util/serialization.hpp>

namespace tdc {namespace compact_hash {

template<typename base_type>
struct alloc_callback_ret_t {
    base_type* data;
    uint64_t bit_offset;
};

template<typename alloc_callback_t>
class sink_t {
    alloc_callback_t m_alloc_callback;

    using base_type = std::remove_pointer_t<decltype(m_alloc_callback(0).data)>;

    static constexpr size_t base_bitsize() {
        return sizeof(base_type) * CHAR_BIT;
    }

    static constexpr base_type base_bit_mask(size_t offset) {
        return (1ull << offset);
    }
public:
    inline sink_t(alloc_callback_t alloc_callback):
        m_alloc_callback(alloc_callback) {}

    inline void write_bit(bool set) {
        auto res = m_alloc_callback(1);

        auto word_offset = res.bit_offset >> 6;
        auto bit_offset = res.bit_offset & 0b111111ull;

        res.data[word_offset] &= ~base_bit_mask(bit_offset);
        res.data[word_offset] |= base_bit_mask(bit_offset) * base_type(set);

        // std::cout << "wrote bit " << set << "\n";

        if (set) {
            DCHECK_NE(res.data[word_offset], 0U);
        }
    }
    inline uint8_t read_bit() {
        auto res = m_alloc_callback(1);

        auto word_offset = res.bit_offset >> 6;
        auto bit_offset = res.bit_offset & 0b111111ull;

        bool r = (res.data[word_offset] & base_bit_mask(bit_offset)) != 0;
        // std::cout << "read bit " << r << "\n";
        return r;
    }

    template<typename T>
    inline void write_int(T value, size_t bits = sizeof(T) * CHAR_BIT) {
        // TODO: better impl, that writes more than 1 bit at a time
        tdc::write_int<T>(std::move(*this), value, bits);
    }
    template<class T>
    inline T read_int(size_t bits = sizeof(T) * CHAR_BIT) {
        // TODO: better impl, that writes more than 1 bit at a time
        return tdc::read_int<T>(std::move(*this), bits);
    }
};

struct elias_gamma_bucket_t {
    struct context_t {
    std::unique_ptr<uint64_t[]>& m_data;
    uint64_t& m_bits;
    uint64_t& m_elem_cursor;
    uint64_t& m_bit_cursor;


    /*
    inline void dump_all() {
        auto ptr = cbp::cbp_repr_t<uint_t<1>>::construct_relative_to(m_data.get(), 0, 1);
        auto s = m_bits;
        // std::cout << "[";
        while(s--) {
            // std::cout << *ptr;
        }
        // std::cout << "]";
    }
    */

    template<typename sink_t>
    inline size_t read(sink_t&& sink) {
        // std::cout << "read()\n";
        auto r = read_elias_gamma<size_t>(sink) - 1;
        // std::cout << "increase m_elem_cursor=" << m_elem_cursor << " by 1\n";
        m_elem_cursor++;
        return r;
    }

    template<typename sink_t>
    inline void write(sink_t&& sink, size_t v) {
        // std::cout << "write(" << v << ")\n";
        write_elias_gamma<size_t>(sink, v + 1);
        // std::cout << "increase m_elem_cursor=" << m_elem_cursor << " by 1\n";
        // std::cout << "write change: "; dump_all(); // std::cout << "\n";
        m_elem_cursor++;
    }

    inline size_t encoded_bit_size(size_t v) {
        uint64_t bit_size = 0;
        uint64_t buffer = 0;

        auto f = [&](size_t bits) {
            uint64_t bit_cursor = bit_size % 64;
            DCHECK_LE(bit_cursor + bits, 64U);
            auto r = alloc_callback_ret_t<uint64_t> { &buffer, bit_cursor };
            bit_size += bits;
            return r;
        };
        auto sink = sink_t<decltype(f)> { f };
        write_elias_gamma<size_t>(sink, v + 1);

        // std::cout << "encoding a " << v << " takes " << bit_size << " bits\n";
        return bit_size;
    }

    inline auto fixed_sink() {
        auto f = [&](size_t bits) {
            DCHECK_LE(m_bit_cursor + bits, m_bits);
            auto r = alloc_callback_ret_t<uint64_t> { m_data.get(), m_bit_cursor };
            // std::cout << "increase m_bit_cursor=" << m_bit_cursor << " by " << bits << "\n";
            m_bit_cursor += bits;
            return r;
        };
        return sink_t<decltype(f)> { f };
    }

    inline void realloc(size_t old_size, size_t new_size) {
        auto n = std::make_unique<uint64_t[]>(new_size);
        size_t common_size = std::min(old_size, new_size);
        for (size_t i = 0; i < common_size; i++) {
            n[i] = m_data[i];
        }
        m_data = std::move(n);
    }

    inline void seek(size_t pos) {
        if (pos < m_elem_cursor) {
            m_elem_cursor = 0;
            m_bit_cursor = 0;
        }
        while(m_elem_cursor < pos) {
            read(fixed_sink());
        }
    }

    inline void realloc_bits(uint64_t bits) {
        if (bits2alloc(bits) != bits2alloc(m_bits)) {
            realloc(bits2alloc(m_bits), bits2alloc(bits));
        }
        m_bits = bits;
        // std::cout << "realloc " << m_bits << " bits, @" << bits2alloc(m_bits) << "\n";
    }

    inline size_t get(size_t pos) {
        seek(pos);
        return read(fixed_sink());
    }

    inline void shift_bits(uint64_t from, uint64_t to, uint64_t size) {
        /* TODO: Efficient shift-based moving
        if from > to {
            uint64_t word_start = from % 64 + 1;
            uint64_t word_end = (from + size) % 64;
        }
        if from < to {

        }
        */

        // std::cout << "shift from " << from << ", to " << to << ", size " << size << "\n";
        DCHECK_LE(from, m_bits);
        DCHECK_LE(to, m_bits);

        DCHECK_LE(from + size, m_bits);
        DCHECK_LE(to + size, m_bits);

        auto from_ptr = cbp::cbp_repr_t<uint_t<1>>::construct_relative_to(m_data.get(), from, 1);
        auto to_ptr = cbp::cbp_repr_t<uint_t<1>>::construct_relative_to(m_data.get(), to, 1);
        auto from_end_ptr = from_ptr + size;

        if (to < from) {
            while(from_ptr != from_end_ptr) {
                *to_ptr = *from_ptr;
                to_ptr++;
                from_ptr++;
                size--;
            }

            DCHECK_EQ(size, 0U);
        }
        if (to > from) {
            from_ptr += size;
            to_ptr += size;
            from_end_ptr -= size;

            while(from_ptr != from_end_ptr) {
                to_ptr--;
                from_ptr--;
                *to_ptr = *from_ptr;
                size--;
            }

            DCHECK_EQ(size, 0U);
        }
    }

    inline void set(size_t pos, size_t val) {
        // std::cout << "set " << pos << " to " << val << "\n";
        // std::cout << "vector: ";
        // dump_all();
        // std::cout << "\n";

        seek(pos);

        auto const backup_bit_cursor = m_bit_cursor;
        auto const backup_elem_cursor = m_elem_cursor;
        // std::cout << "cursor: elem=" << m_elem_cursor << ", bits=" << m_bit_cursor << "\n";
        auto const existing_val = get(pos);
        // std::cout << "cursor: elem=" << m_elem_cursor << ", bits=" << m_bit_cursor << "\n";
        // std::cout << "existing_val: " << existing_val << "\n";

        if (existing_val != val) {
            m_bit_cursor = backup_bit_cursor;
            m_elem_cursor = backup_elem_cursor;
            // std::cout << "cursor: elem=" << m_elem_cursor << ", bits=" << m_bit_cursor << "\n";

            auto existing_val_bit_size = encoded_bit_size(existing_val);
            // std::cout << "existing_val_bit_size: " << existing_val_bit_size << "\n";

            auto new_val_bit_size = encoded_bit_size(val);
            // std::cout << "new_val_bit_size: " << new_val_bit_size << "\n";

            auto new_bit_size = m_bits + new_val_bit_size - existing_val_bit_size;
            auto existing_bit_size = m_bits;

            // std::cout << "existing_bit_size: " << existing_bit_size << "\n";
            // std::cout << "new_bit_size:      " << new_bit_size << "\n";
            // std::cout << std::endl;

            if (new_bit_size < existing_bit_size) {
                // Shift left, then shrink

                shift_bits(m_bit_cursor + existing_val_bit_size,
                           m_bit_cursor + new_val_bit_size,
                           existing_bit_size - (m_bit_cursor + existing_val_bit_size));
                realloc_bits(new_bit_size);
            } else {
                // Grow first, the shift right

                realloc_bits(new_bit_size);
                shift_bits(m_bit_cursor + existing_val_bit_size,
                           m_bit_cursor + new_val_bit_size,
                           existing_bit_size - (m_bit_cursor + existing_val_bit_size));
            }

            write(fixed_sink(), val);
        }

        {
            m_bit_cursor = backup_bit_cursor;
            m_elem_cursor = backup_elem_cursor;
            auto const post_val_check = get(pos);
            DCHECK_EQ(val, post_val_check);
        }
        // std::cout << "\n";
    }
    };

    auto context(uint64_t& element_cursor, uint64_t& bit_cursor) {
        return context_t {
            m_data,
            m_bits,
            element_cursor,
            bit_cursor,
        };
    }
    auto context(uint64_t& element_cursor, uint64_t& bit_cursor) const {
        return context_t {
            m_data,
            m_bits,
            element_cursor,
            bit_cursor,
        };
    }

    mutable std::unique_ptr<uint64_t[]> m_data;
    mutable uint64_t m_bits = 0;

    inline elias_gamma_bucket_t(size_t size)
    {
        uint64_t elem_cursor = 0;
        uint64_t bit_cursor = 0;
        auto ctx = this->context(elem_cursor, bit_cursor);

        // Allocate memory for all encoded 0s
        auto all_bits = ctx.encoded_bit_size(0) * size;
        // std::cout << "-------\n";
        // std::cout << "size: " << size << "\n";
        ctx.realloc_bits(all_bits);

        // Encode all 0s.
        // TODO: Just copy the encoding of the first one
        for(size_t i = 0; i < size; i++) {
            ctx.write(ctx.fixed_sink(), 0);
        }
    }

    inline elias_gamma_bucket_t() {}

    inline static size_t bits2alloc(uint64_t bits) {
        return (bits + 63ull) >> 6ull;
    }
};

/// Buckets take up exactly `N` elements.
template<size_t N>
struct fixed_elias_gamma_bucket_size_t {
    /// runtime initilization arguments, if any
    struct config_args {};

    /// get the config of this instance
    inline config_args current_config() const { return config_args{}; }

    fixed_elias_gamma_bucket_size_t(config_args) {};

    inline size_t bucket_size(size_t /*table_size*/) const { return N; }
};

/// Buckets take up exactly `fixed_size` elements.
struct dynamic_fixed_elias_gamma_bucket_size_t {
    size_t m_fixed_size;

    /// runtime initilization arguments, if any
    struct config_args { size_t bucket_size = 1024; };

    /// get the config of this instance
    inline config_args current_config() const {
        return config_args{ m_fixed_size };
    }

    dynamic_fixed_elias_gamma_bucket_size_t(config_args config) {
        m_fixed_size = config.bucket_size;
    };

    inline size_t bucket_size(size_t /*table_size*/) const {
        return m_fixed_size;
    }
};

/// Bucket sizes grow according to the table size via the formular
/// `std::pow(std::log2(table_size), F)`, where `F` is the factor `3.0/2.0` per default.
struct growing_elias_gamma_bucket_size_t {
    double m_factor;

    /// runtime initilization arguments, if any
    struct config_args { double factor = 3.0 / 2.0; };

    /// get the config of this instance
    inline config_args current_config() const {
        return config_args{ m_factor };
    }

    growing_elias_gamma_bucket_size_t(config_args config) {
        m_factor = config.factor;
    };

    inline size_t bucket_size(size_t table_size) const {
        // TODO: Add reference for the growth formula.
        return std::pow(std::log2(table_size), m_factor);
    }
};

/// Stores displacement entries as elias-gamma encoded integers.
///
/// To prevent large scanning costs, the entries are split up into buckets.
///
/// The size of each buckets is determined by `elias_gamma_bucket_size_t`.
///
/// It expects a type with a member
/// `static size_t bucket_size(size_t table_size);` for calculating the
/// desired bucket size.
template<typename elias_gamma_bucket_size_t>
class elias_gamma_displacement_table_t {
public:
    using bucket_size_t = elias_gamma_bucket_size_t;
private:
    mutable uint64_t m_elem_cursor = 0;
    mutable uint64_t m_bit_cursor = 0;
    mutable size_t m_bucket_cursor = 0;

    bucket_size_t m_bucket_size;
    size_t m_bucket_size_cache;

    std::unique_ptr<elias_gamma_bucket_t[]> m_buckets;

    struct BucketSizes {
        size_t full_buckets;
        size_t remainder_bucket_size;
        size_t buckets;
        size_t bucket_size;
    };
    inline BucketSizes calc_buckets(size_t table_size) const {
        auto bucket_size = m_bucket_size.bucket_size(table_size);

        size_t full_buckets = table_size / bucket_size;
        size_t remainder_bucket_size = table_size % bucket_size;
        size_t buckets = full_buckets + (remainder_bucket_size != 0);

        return {
            full_buckets,
            remainder_bucket_size,
            buckets,
            bucket_size,
        };
    }

    template<typename T>
    friend struct ::tdc::serialize;

    template<typename T>
    friend struct ::tdc::heap_size;
public:
    /// runtime initilization arguments, if any
    struct config_args {
        typename bucket_size_t::config_args bucket_size_config;
    };

    /// get the config of this instance
    inline config_args current_config() const {
        return config_args{
            m_bucket_size.current_config()
        };
    }

    inline elias_gamma_displacement_table_t(size_t table_size,
                                            config_args config):
        m_bucket_size(config.bucket_size_config)
    {
        auto r = calc_buckets(table_size);
        m_bucket_size_cache = r.bucket_size;

        m_buckets = std::make_unique<elias_gamma_bucket_t[]>(r.buckets);

        for (size_t i = 0; i < r.full_buckets; i++) {
            m_buckets[i] = elias_gamma_bucket_t(m_bucket_size_cache);
        }
        if (r.remainder_bucket_size != 0) {
            m_buckets[r.buckets - 1] = elias_gamma_bucket_t(r.remainder_bucket_size);
        }
    }

    inline size_t get(size_t pos) const {
        size_t bucket = pos / m_bucket_size_cache;
        size_t offset = pos % m_bucket_size_cache;
        if (bucket != m_bucket_cursor) {
            m_bucket_cursor = bucket;
            m_elem_cursor = 0;
            m_bit_cursor = 0;
        }

        return m_buckets[m_bucket_cursor]
            .context(m_elem_cursor, m_bit_cursor)
            .get(offset);
    }
    inline void set(size_t pos, size_t val) {
        size_t bucket = pos / m_bucket_size_cache;
        size_t offset = pos % m_bucket_size_cache;
        if (bucket != m_bucket_cursor) {
            m_bucket_cursor = bucket;
            m_elem_cursor = 0;
            m_bit_cursor = 0;
        }

        m_buckets[m_bucket_cursor]
            .context(m_elem_cursor, m_bit_cursor)
            .set(offset, val);
    }
};

}

template<typename elias_gamma_bucket_size_t>
struct heap_size<compact_hash::elias_gamma_displacement_table_t<elias_gamma_bucket_size_t>> {
    using T = compact_hash::elias_gamma_displacement_table_t<elias_gamma_bucket_size_t>;

    static object_size_t compute(T const& val, size_t table_size) {
        auto bytes = object_size_t::empty();

        bytes += heap_size<uint64_t>::compute(val.m_elem_cursor);
        bytes += heap_size<uint64_t>::compute(val.m_bit_cursor);
        bytes += heap_size<size_t>::compute(val.m_bucket_cursor);
        bytes += heap_size<size_t>::compute(val.m_bucket_size_cache);
        bytes += heap_size<elias_gamma_bucket_size_t>::compute(val.m_bucket_size);
        bytes += object_size_t::exact(sizeof(decltype(val.m_buckets)));

        using bucket_t = compact_hash::elias_gamma_bucket_t;
        using table_t =
            compact_hash::elias_gamma_displacement_table_t<elias_gamma_bucket_size_t>;

        table_t const& table = val;

        auto& buckets = table.m_buckets;
        auto s = table.calc_buckets(table_size);
        for (size_t i = 0; i < s.buckets; i++) {
            bucket_t const& b = buckets[i];

            bytes += heap_size<uint64_t>::compute(b.m_bits);

            size_t words = bucket_t::bits2alloc(b.m_bits);
            bytes += heap_size<decltype(b.m_data)>::compute(b.m_data, words);
        }

        return bytes;
    }
};

template<typename elias_gamma_bucket_size_t>
struct serialize<compact_hash::elias_gamma_displacement_table_t<elias_gamma_bucket_size_t>> {
    using T = compact_hash::elias_gamma_displacement_table_t<elias_gamma_bucket_size_t>;

    static object_size_t write(std::ostream& out, T const& val, size_t table_size) {
        auto bytes = object_size_t::empty();

        using bucket_t = compact_hash::elias_gamma_bucket_t;
        using table_t =
            compact_hash::elias_gamma_displacement_table_t<elias_gamma_bucket_size_t>;

        table_t const& table = val;
        bytes += serialize<size_t>::write(out, table.m_bucket_size_cache);
        bytes += serialize<elias_gamma_bucket_size_t>::write(out, table.m_bucket_size);

        auto& buckets = table.m_buckets;
        auto s = table.calc_buckets(table_size);
        for (size_t i = 0; i < s.buckets; i++) {
            bucket_t const& b = buckets[i];

            bytes += serialize<uint64_t>::write(out, b.m_bits);

            size_t words = bucket_t::bits2alloc(b.m_bits);
            for (size_t j = 0; j < words; j++) {
                bytes += serialize<uint64_t>::write(out, b.m_data[j]);
            }
        }

        return bytes;
    }

    static T read(std::istream& in, size_t table_size) {
        using bucket_t = compact_hash::elias_gamma_bucket_t;
        using table_t =
            compact_hash::elias_gamma_displacement_table_t<elias_gamma_bucket_size_t>;

        table_t table = table_t(table_size, {});
        table.m_bucket_size_cache = serialize<size_t>::read(in);
        table.m_bucket_size = serialize<elias_gamma_bucket_size_t>::read(in);

        auto& buckets = table.m_buckets;
        auto s = table.calc_buckets(table_size);

        for (size_t i = 0; i < s.buckets; i++) {
            bucket_t& b = buckets[i];

            b.m_bits = serialize<uint64_t>::read(in);

            size_t words = bucket_t::bits2alloc(b.m_bits);
            b.m_data = std::make_unique<uint64_t[]>(words);
            for (size_t j = 0; j < words; j++) {
                b.m_data[j] = serialize<uint64_t>::read(in);
            }
        }

        return table;
    }

    static bool equal_check(T const& lhs, T const& rhs, size_t table_size) {
        for (size_t i = 0; i < table_size; i++) {
            if (!gen_equal_diagnostic(lhs.get(i) == rhs.get(i))) {
                return false;
            }
        }
        return gen_equal_check(m_bucket_size_cache)
            && gen_equal_check(m_bucket_size);
    }
};

template<size_t N>
struct heap_size<compact_hash::fixed_elias_gamma_bucket_size_t<N>> {
    static object_size_t compute(compact_hash::fixed_elias_gamma_bucket_size_t<N> const& val) {
        return object_size_t::exact(sizeof(compact_hash::fixed_elias_gamma_bucket_size_t<N>));
    }
};
gen_heap_size_without_indirection(compact_hash::dynamic_fixed_elias_gamma_bucket_size_t)
gen_heap_size_without_indirection(compact_hash::growing_elias_gamma_bucket_size_t)

template<size_t N>
struct serialize<compact_hash::fixed_elias_gamma_bucket_size_t<N>> {
    using T = compact_hash::fixed_elias_gamma_bucket_size_t<N>;

    static object_size_t write(std::ostream& out, T const& val) {
        auto bytes = object_size_t::empty();
        return bytes;
    }

    static T read(std::istream& in) {
        return T({});
    }

    static bool equal_check(T const& lhs, T const& rhs) {
        return true;
    }
};

template<>
struct serialize<compact_hash::dynamic_fixed_elias_gamma_bucket_size_t> {
    using T = compact_hash::dynamic_fixed_elias_gamma_bucket_size_t;

    static object_size_t write(std::ostream& out, T const& val) {
        auto bytes = object_size_t::empty();
        bytes += serialize_write(out, val.m_fixed_size);
        return bytes;
    }

    static T read(std::istream& in) {
        auto val = T({});
        serialize_read_into(in, val.m_fixed_size);
        return val;
    }

    static bool equal_check(T const& lhs, T const& rhs) {
        return gen_equal_check(m_fixed_size);
    }
};

template<>
struct serialize<compact_hash::growing_elias_gamma_bucket_size_t> {
    using T = compact_hash::growing_elias_gamma_bucket_size_t;

    static object_size_t write(std::ostream& out, T const& val) {
        auto bytes = object_size_t::empty();
        bytes += serialize_write(out, val.m_factor);
        return bytes;
    }

    static T read(std::istream& in) {
        auto val = T({});
        serialize_read_into(in, val.m_factor);
        return val;
    }

    static bool equal_check(T const& lhs, T const& rhs) {
        return gen_equal_check(m_factor);
    }
};

}
