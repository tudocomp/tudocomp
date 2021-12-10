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

template<size_t N>
struct static_layered_bit_width_t {
    using elem_t = uint_t<N>;

    /// runtime initilization arguments, if any
    struct config_args {};

    /// get the config of this instance
    inline config_args current_config() const { return config_args{}; }

    static_layered_bit_width_t() = default;
    static_layered_bit_width_t(config_args config) {}

    inline void set_width(IntVector<elem_t>& iv) const {}
    inline uint64_t max() const { return std::numeric_limits<elem_t>::max(); }
};

struct dynamic_layered_bit_width_t {
    using elem_t = dynamic_t;

    size_t m_width;

    /// runtime initilization arguments, if any
    struct config_args { size_t width = 4; };

    /// get the config of this instance
    inline config_args current_config() const { return config_args{ m_width }; }

    dynamic_layered_bit_width_t() = default;
    dynamic_layered_bit_width_t(config_args config): m_width(config.width) {}

    inline void set_width(IntVector<elem_t>& iv) const {
        iv.width(m_width);
    }
    inline uint64_t max() const { return (1ull << m_width) - 1; }
};

/// Stores displacement entries as integers with a bit width given by
/// `bit_width_t`. Displacement value larger than that
/// will be spilled into a `std::unordered_map<size_t, size_t>`.
template<typename bit_width_t>
class layered_displacement_table_t {
    template<typename T>
    friend struct ::tdc::serialize;

    template<typename T>
    friend struct ::tdc::heap_size;

    using elem_t = typename bit_width_t::elem_t;
    using elem_val_t = typename IntVector<elem_t>::value_type;

    IntVector<elem_t> m_displace;
    std::unordered_map<size_t, size_t> m_spill;
    bit_width_t m_bit_width;

    layered_displacement_table_t() = default;
public:
    /// runtime initilization arguments, if any
    struct config_args {
        typename bit_width_t::config_args bit_width_config;
    };

    /// get the config of this instance
    inline config_args current_config() const {
        return config_args{ m_bit_width.current_config() };
    }

    inline layered_displacement_table_t(size_t table_size,
                                        config_args config):
        m_bit_width(config.bit_width_config)
    {
        m_bit_width.set_width(m_displace);
        m_displace.reserve(table_size);
        m_displace.resize(table_size);
    }
    inline size_t get(size_t pos) {
        size_t max = m_bit_width.max();
        size_t tmp = elem_val_t(m_displace[pos]);
        if (tmp == max) {
            return m_spill[pos];
        } else {
            return tmp;
        }
    }
    inline void set(size_t pos, size_t val) {
        size_t max = m_bit_width.max();
        if (val >= max) {
            m_displace[pos] = max;
            m_spill[pos] = val;
        } else {
            m_displace[pos] = val;
        }
    }
};

}

template<typename bit_width_t>
struct heap_size<compact_hash::layered_displacement_table_t<bit_width_t>> {
    using T = compact_hash::layered_displacement_table_t<bit_width_t>;

    static object_size_t compute(T const& val, size_t table_size) {
        auto bytes = object_size_t::empty();

        DCHECK_EQ(val.m_displace.size(), table_size);
        auto size = val.m_displace.stat_allocation_size_in_bytes();
        bytes += object_size_t::exact(size);
        bytes += heap_size_compute(val.m_bit_width);

        size_t unordered_map_size_guess
            = sizeof(decltype(val.m_spill))
            + val.m_spill.size() * sizeof(size_t) * 2;

        bytes += object_size_t::unknown_extra_data(unordered_map_size_guess);

        return bytes;
    }
};

template<typename bit_width_t>
struct serialize<compact_hash::layered_displacement_table_t<bit_width_t>> {
    using T = compact_hash::layered_displacement_table_t<bit_width_t>;

    static object_size_t write(std::ostream& out, T const& val, size_t table_size) {
        auto bytes = object_size_t::empty();

        DCHECK_EQ(val.m_displace.size(), table_size);

        bytes += serialize_write(out, val.m_bit_width);

        auto data = (char const*) val.m_displace.data();
        auto size = val.m_displace.stat_allocation_size_in_bytes();
        out.write(data, size);
        bytes += object_size_t::exact(size);

        size_t spill_size = val.m_spill.size();
        out.write((char*) &spill_size, sizeof(size_t));
        bytes += object_size_t::exact(sizeof(size_t));

        for (auto pair : val.m_spill) {
            size_t k = pair.first;
            size_t v = pair.second;
            out.write((char*) &k, sizeof(size_t));
            out.write((char*) &v, sizeof(size_t));
            bytes += object_size_t::exact(sizeof(size_t) * 2);
            spill_size--;
        }

        DCHECK_EQ(spill_size, 0U);

        return bytes;
    }

    static T read(std::istream& in, size_t table_size) {
        T ret;
        serialize_read_into(in, ret.m_bit_width);
        ret.m_bit_width.set_width(ret.m_displace);
        ret.m_displace.reserve(table_size);
        ret.m_displace.resize(table_size);
        auto data = (char*) ret.m_displace.data();
        auto size = ret.m_displace.stat_allocation_size_in_bytes();
        in.read(data, size);

        auto& spill = ret.m_spill;
        size_t spill_size;
        in.read((char*) &spill_size, sizeof(size_t));

        for (size_t i = 0; i < spill_size; i++) {
            size_t k;
            size_t v;
            in.read((char*) &k, sizeof(size_t));
            in.read((char*) &v, sizeof(size_t));

            spill[k] = v;
        }

        return ret;
    }

    static bool equal_check(T const& lhs, T const& rhs, size_t table_size) {
        return gen_equal_diagnostic(lhs.m_displace == rhs.m_displace)
        && gen_equal_diagnostic(lhs.m_spill == rhs.m_spill)
        && gen_equal_check(m_bit_width);
    }
};

template<size_t bit_width_t>
struct heap_size<compact_hash::static_layered_bit_width_t<bit_width_t>> {
    using T = compact_hash::static_layered_bit_width_t<bit_width_t>;

    static object_size_t compute(T const& val) {
        return object_size_t::empty();
    }
};

template<size_t bit_width_t>
struct serialize<compact_hash::static_layered_bit_width_t<bit_width_t>> {
    using T = compact_hash::static_layered_bit_width_t<bit_width_t>;

    static object_size_t write(std::ostream& out, T const& val) {
        return object_size_t::empty();
    }

    static T read(std::istream& in) {
        return T();
    }

    static bool equal_check(T const& lhs, T const& rhs) {
        return true;
    }
};

template<>
struct heap_size<compact_hash::dynamic_layered_bit_width_t> {
    using T = compact_hash::dynamic_layered_bit_width_t;

    static object_size_t compute(T const& val) {
        return object_size_t::exact(sizeof(T));
    }
};

template<>
struct serialize<compact_hash::dynamic_layered_bit_width_t> {
    using T = compact_hash::dynamic_layered_bit_width_t;

    static object_size_t write(std::ostream& out, T const& val) {
        auto bytes = object_size_t::empty();
        bytes += serialize_write(out, val.m_width);
        return bytes;
    }

    static T read(std::istream& in) {
        T ret;
        serialize_read_into(in, ret.m_width);
        return ret;
    }

    static bool equal_check(T const& lhs, T const& rhs) {
        return gen_equal_check(m_width);
    }
};


}
