#pragma once

#include <memory>

#include <tudocomp/util/compact_hash/util.hpp>
#include <tudocomp/util/compact_hash/entry_t.hpp>

#include <tudocomp/util/serialization.hpp>

// Table for uninitalized elements

namespace tdc {namespace compact_hash {
    template<typename satellite_t>
    struct plain_sentinel_t {
        using satellite_t_export = satellite_t;
        using entry_ptr_t = typename satellite_t::entry_ptr_t;
        using entry_bit_width_t = typename satellite_t::entry_bit_width_t;
        using qvd_t = typename satellite_t::bucket_data_layout_t;
        using value_type = typename satellite_t::sentinel_value_type;

        template<typename T>
        friend struct ::tdc::serialize;

        std::unique_ptr<uint64_t[]> m_alloc;
        value_type m_empty_value;

        /// runtime initilization arguments, if any
        struct config_args {
            value_type empty_value = value_type();
        };

        /// get the config of this instance
        inline config_args current_config() const {
            return config_args{
                m_empty_value,
            };
        }

        inline plain_sentinel_t() {}
        inline plain_sentinel_t(size_t table_size,
                                entry_bit_width_t widths,
                                config_args config):
            m_empty_value(config.empty_value)
        {
            size_t alloc_size = qvd_t::calc_sizes(table_size, widths).overall_qword_size;
            m_alloc = std::make_unique<uint64_t[]>(alloc_size);

            auto ctx = context(table_size, widths);

            for(size_t i = 0; i < table_size; i++) {
                // NB: Using at because allocate_pos()
                // destroys the location first.
                auto elem = ctx.at(ctx.table_pos(i));
                elem.set_no_drop(value_type(m_empty_value), 0);
            }
        }
        struct table_pos_t {
            size_t offset;
            inline table_pos_t(): offset(-1) {}
            inline table_pos_t(size_t o): offset(o) {}
            inline table_pos_t& operator=(table_pos_t const& other) = default;
            inline table_pos_t(table_pos_t const& other) = default;
        };
        // pseudo-iterator for iterating over bucket elements
        // NB: does not wrap around!
        struct iter_t {
            entry_ptr_t       m_end;
            value_type const& m_empty_value;

            inline iter_t(entry_ptr_t endpos,
                          value_type const& empty_value):
                m_end(endpos),
                m_empty_value(empty_value)
            {
            }

            inline entry_ptr_t get() {
                return m_end;
            }

            inline void decrement() {
                do {
                    m_end.decrement_ptr();
                } while(*m_end.val_ptr() == m_empty_value);
            }

            inline bool operator!=(iter_t& other) {
                return m_end != other.m_end;
            }
        };

        template<typename alloc_type>
        struct context_t {
            alloc_type& m_alloc;
            value_type const& m_empty_value;
            size_t const table_size;
            entry_bit_width_t widths;

            inline void destroy_vals() {
                qvd_t::destroy_vals(m_alloc.get(), table_size, widths);
            }

            inline table_pos_t table_pos(size_t pos) {
                return table_pos_t { pos };
            }
            inline entry_ptr_t allocate_pos(table_pos_t pos) {
                DCHECK_LT(pos.offset, table_size);
                auto tmp = at(pos);

                // NB: allocate_pos returns a unitialized location,
                // but all locations are per default initialized with a empty_value.
                // Therefore we destroy the existing value first.
                tmp.uninitialize();

                return tmp;
            }
            inline entry_ptr_t at(table_pos_t pos) {
                DCHECK_LT(pos.offset, table_size);
                return qvd_t::at(m_alloc.get(), table_size, pos.offset, widths);
            }
            inline bool pos_is_empty(table_pos_t pos) {
                DCHECK_LT(pos.offset, table_size);
                return *at(pos).val_ptr() == m_empty_value;
            }
            inline iter_t make_iter(table_pos_t const& pos) {
                // NB: One-pass-the-end is acceptable for a end iterator
                DCHECK_LE(pos.offset, table_size);
                return iter_t {
                    qvd_t::at(m_alloc.get(), table_size, pos.offset, widths),
                    m_empty_value,
                };
            }
            inline void trim_storage(table_pos_t* last_start, table_pos_t const& end) {
                // Nothing to be done
            }
        };
        inline auto context(size_t table_size, entry_bit_width_t const& widths) {
            return context_t<std::unique_ptr<uint64_t[]>> {
                m_alloc, m_empty_value, table_size, widths,
            };
        }
        inline auto context(size_t table_size, entry_bit_width_t const& widths) const {
            return context_t<std::unique_ptr<uint64_t[]> const> {
                m_alloc, m_empty_value, table_size, widths,
            };
        }
    };
}

template<typename satellite_t>
struct heap_size<compact_hash::plain_sentinel_t<satellite_t>> {
    using T = compact_hash::plain_sentinel_t<satellite_t>;
    using entry_bit_width_t = typename T::entry_bit_width_t;
    using value_type = typename T::value_type;
    using qvd_t = typename T::qvd_t;

    static object_size_t compute(T const& val, size_t table_size, entry_bit_width_t const& widths) {
        using namespace compact_hash;

        auto bytes = object_size_t::empty();

        auto alloc_size = qvd_t::calc_sizes(table_size, widths).overall_qword_size;

        bytes += heap_size<value_type>::compute(val.m_empty_value);
        bytes += heap_size<std::unique_ptr<uint64_t[]>>::compute(val.m_alloc, alloc_size);

        return bytes;
    }
};

template<typename satellite_t>
struct serialize<compact_hash::plain_sentinel_t<satellite_t>> {
    using T = compact_hash::plain_sentinel_t<satellite_t>;
    using entry_bit_width_t = typename T::entry_bit_width_t;
    using value_type = typename T::value_type;
    using qvd_t = typename T::qvd_t;

    static object_size_t write(std::ostream& out, T const& val, size_t table_size, entry_bit_width_t const& widths) {
        using namespace compact_hash;

        auto bytes = object_size_t::empty();

        auto alloc_size = qvd_t::calc_sizes(table_size, widths).overall_qword_size;

        bytes += serialize<value_type>::write(out, val.m_empty_value);
        for (size_t i = 0; i < alloc_size; i++) {
            bytes += serialize<uint64_t>::write(out, val.m_alloc[i]);
        }

        return bytes;
    }
    static T read(std::istream& in, size_t table_size, entry_bit_width_t const& widths) {
        using namespace compact_hash;

        auto alloc_size = qvd_t::calc_sizes(table_size, widths).overall_qword_size;

        T ret;
        ret.m_empty_value = serialize<value_type>::read(in);
        ret.m_alloc = std::make_unique<uint64_t[]>(alloc_size);

        for (size_t i = 0; i < alloc_size; i++) {
            ret.m_alloc[i] = serialize<uint64_t>::read(in);
        }

        return ret;
    }
    static bool equal_check(T const& lhs, T const& rhs, size_t table_size, entry_bit_width_t const& widths) {
        auto lhsc = lhs.context(table_size, widths);
        auto rhsc = rhs.context(table_size, widths);

        for (size_t i = 0; i < table_size; i++) {
            auto lhspos = lhsc.table_pos(i);
            auto rhspos = rhsc.table_pos(i);
            if (!gen_equal_diagnostic(lhsc.pos_is_empty(lhspos) == rhsc.pos_is_empty(rhspos))) {
                return false;
            }
            if (!lhsc.pos_is_empty(lhspos)) {
                auto lhsptrs = lhsc.at(lhspos);
                auto rhsptrs = rhsc.at(rhspos);

                if (!gen_equal_diagnostic(lhsptrs.get_quotient() == rhsptrs.get_quotient())) {
                    return false;
                }
                if (!gen_equal_diagnostic(*lhsptrs.val_ptr() == *rhsptrs.val_ptr())) {
                    return false;
                }
            }
        }

        return true;
    }
};

}
