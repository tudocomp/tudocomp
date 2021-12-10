#pragma once

#include <limits>

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/IntPtr.hpp>
#include "../entry_t.hpp"

#include <tudocomp/util/serialization.hpp>

namespace tdc {namespace compact_hash {

class cv_bvs_t {
    template<typename T>
    friend struct ::tdc::serialize;

    template<typename T>
    friend struct ::tdc::heap_size;

    IntVector<uint_t<2>> m_cv;
    inline cv_bvs_t(IntVector<uint_t<2>>&& cv): m_cv(std::move(cv)) {}

public:
    /// runtime initilization arguments, if any
    struct config_args {};

    /// get the config of this instance
    inline config_args current_config() const { return config_args{}; }

    inline cv_bvs_t(size_t table_size, config_args config) {
        m_cv.reserve(table_size);
        m_cv.resize(table_size);
    }

    /// A Group is a half-open range [group_start, group_end)
    /// that corresponds to a group of elements in the hashtable that
    /// belong to the same initial_address.
    ///
    /// This means that `c[group_start] == 1`, and
    /// `c[group_start < x < group_end] == 0`.
    ///
    /// `groups_terminator` points to the next free location
    /// inside the hashtable.
    struct Group {
        size_t group_start;       // Group that belongs to the key.
        size_t group_end;         // It's a half-open range: [start .. end).
        size_t groups_terminator; // Next free location.
    };

    template<typename storage_t, typename size_mgr_t>
    struct context_t {
        using satellite_t = typename storage_t::satellite_t_export;
        using entry_width_t = typename satellite_t::entry_bit_width_t;
        using entry_ptr_t = typename satellite_t::entry_ptr_t;
        using entry_t = generic_entry_t<entry_ptr_t>;
        using table_pos_t = typename storage_t::table_pos_t;

        IntVector<uint_t<2>>& m_cv;
        size_t const table_size;
        entry_width_t widths;
        size_mgr_t const& size_mgr;
        storage_t& storage;

        /// Getter for the v bit at table position `pos`.
        inline bool get_v(size_t pos) {
            return (m_cv[pos] & 0b01) != 0;
        }

        /// Getter for the c bit at table position `pos`.
        inline bool get_c(size_t pos) {
            return (m_cv[pos] & 0b10) != 0;
        }

        /// Setter for the v bit at table position `pos`.
        inline void set_v(size_t pos, bool v) {
            auto x = m_cv[pos] & 0b10;
            m_cv[pos] = x | (0b01 * v);
        }

        /// Setter for the c bit at table position `pos`.
        inline void set_c(size_t pos, bool c) {
            auto x = m_cv[pos] & 0b01;
            m_cv[pos] = x | (0b10 * c);
        }

        /// Setter for the c and v bit at table position `pos`.
        inline void set_cv(size_t pos, uint8_t v) {
            m_cv[pos] = v;
        }

        // Assumption: There exists a group at the initial address of `key`.
        // This group is either the group belonging to key,
        // or the one after it in the case that no group for `key` exists yet.
        inline Group search_existing_group(uint64_t initial_address) {
            auto sctx = storage.context(table_size, widths);
            auto ret = Group();
            size_t cursor = initial_address;

            // Walk forward from the initial address until we find a empty location.
            // TODO: This search could maybe be accelerated by:
            // - checking whole blocks in the bucket bitvector for == or != 0
            size_t v_counter = 0;
            DCHECK_EQ(get_v(cursor), true);
            for(;
                !sctx.pos_is_empty(sctx.table_pos(cursor));
                cursor = size_mgr.mod_add(cursor))
            {
                v_counter += get_v(cursor);
            }
            DCHECK_GE(v_counter, 1U);
            ret.groups_terminator = cursor;

            // Walk back again to find the end of the group
            // belonging to the initial address.
            size_t c_counter = v_counter;
            for(; c_counter != 1; cursor = size_mgr.mod_sub(cursor)) {
                c_counter -= get_c(size_mgr.mod_sub(cursor));
            }
            ret.group_end = cursor;

            // Walk further back to find the start of the group
            // belonging to the initial address
            for(; c_counter != 0; cursor = size_mgr.mod_sub(cursor)) {
                c_counter -= get_c(size_mgr.mod_sub(cursor));
            }
            ret.group_start = cursor;

            return ret;
        }

        /// Search a quotient inside an existing Group.
        ///
        /// This returns a pointer to the value if its found, or null
        /// otherwise.
        inline entry_t search_in_group(Group const& group,
                                       uint64_t stored_quotient) {
            auto sctx = storage.context(table_size, widths);
            for(size_t i = group.group_start; i != group.group_end; i = size_mgr.mod_add(i)) {
                auto sparse_entry = sctx.at(sctx.table_pos(i));

                if (sparse_entry.get_quotient() == stored_quotient) {
                    uint64_t in_group_offset = size_mgr.mod_sub(i, group.group_start);
                    return entry_t::found_exist(in_group_offset, sparse_entry);
                }
            }
            return entry_t::not_found();
        }

        /// Inserts a new key-value pair after an existing
        /// group, shifting all following entries one to the right as needed.
        inline entry_ptr_t insert_value_after_group(
            Group const& group, uint64_t stored_quotient)
        {
            auto sctx = storage.context(table_size, widths);
            auto end_pos = sctx.table_pos(group.group_end);
            if (sctx.pos_is_empty(end_pos)) {
                // if there is no following group, just append the new entry
                return sctx.allocate_pos(end_pos);
            } else {
                // else, shift all following elements one to the right
                return shift_groups_and_insert(group.group_end,
                                               group.groups_terminator,
                                               stored_quotient);
            }
        }

        /// Shifts all values and `c` bits of the half-open range [from, to)
        /// inside the table one to the right, and inserts the new value
        /// at the now-empty location `from`.
        ///
        /// The position `to` needs to be empty.
        inline entry_ptr_t shift_groups_and_insert(
            size_t from, size_t to, uint64_t stored_quotient)
        {
            DCHECK_NE(from, to);

            for(size_t i = to; i != from;) {
                size_t next_i = size_mgr.mod_sub(i, size_t(1));

                set_c(i, get_c(next_i));

                i = next_i;
            }
            set_c(from, false);

            return shift_elements_and_insert(from, to);
        }

        /// Shifts all values of the half-open range [from, to)
        /// inside the table one to the right, and inserts the new value
        /// at the now-empty location `from`.
        ///
        /// The position `to` needs to be empty.
        inline entry_ptr_t shift_elements_and_insert(
            size_t from, size_t to)
        {
            auto sctx = storage.context(table_size, widths);
            // move from...to one to the right, then insert at from

            DCHECK(from != to);

            table_pos_t from_pos;

            if (to < from) {
                // if the range wraps around, we decompose into two ranges:
                // [   |      |      ]
                // | to^      ^from  |
                // ^start         end^
                // [ 2 ]      [  1   ]
                //
                // NB: because we require from != to, and insert 1 additional element,
                // we are always dealing with a minimum 2 element range,
                // and thus can not end up with a split range with length == 0.

                from_pos = sparse_shift(from,  table_size);
                if (to > 0) {
                    auto start_pos = sparse_shift(0, to);
                    sctx.at(from_pos).swap_with(sctx.at(start_pos));
                }
            } else {
                // [     |      |      ]
                //   from^      ^to

                from_pos = sparse_shift(from, to);
            }

            // insert the element from the end of the range at the free
            // position to the right of it.
            auto new_loc = sctx.allocate_pos(sctx.table_pos(to));

            auto from_ptrs = sctx.at(from_pos);
            new_loc.init_from(from_ptrs);
            from_ptrs.uninitialize();

            return from_ptrs;
        }

        /// Shifts all elements one to the right,
        /// moving the last element to the front position,
        /// and returns a ptr pair to it.
        inline table_pos_t sparse_shift(size_t from, size_t to) {
            DCHECK_LT(from, to);
            auto sctx = storage.context(table_size, widths);

            // initialize iterators like this:
            // [         ]
            // ^from   to^
            //          ||
            //    <- src^|
            //    <- dest^

            auto from_loc = sctx.table_pos(from);
            auto from_iter = sctx.make_iter(from_loc);

            auto last = sctx.table_pos(to - 1);
            auto src = sctx.make_iter(last);
            auto dst = sctx.make_iter(sctx.table_pos(to));

            // move the element at the last position to a temporary position
            auto tmp_p = sctx.at(last);
            auto tmp = tmp_p.move_out();

            // move all elements one to the right
            // TODO: Could be optimized
            // to memcpies for different underlying layouts
            while(src != from_iter) {
                // Decrement first for backward iteration
                src.decrement();
                dst.decrement();

                // Get access to the value/quotient at src and dst
                auto src_be = src.get();
                auto dst_be = dst.get();

                // Copy value/quotient over
                dst_be.move_from(src_be);
            }

            // move last element to the front
            auto from_p = sctx.at(from_loc);
            from_p.set(std::move(tmp));
            return from_loc;
        }

        inline uint64_t local_id_to_global_id(uint64_t initial_address, uint64_t local_id) {
            local_id <<= size_mgr.capacity_log2();
            local_id |= initial_address;
            return local_id;
        }

        entry_t lookup_insert(uint64_t initial_address,
                              uint64_t stored_quotient)
        {
            auto sctx = storage.context(table_size, widths);
            auto ia_pos = sctx.table_pos(initial_address);

            // cases:
            // - initial address empty.
            // - initial address occupied, there is an element for this key
            //   (v[initial address] = 1).
            // - initial address occupied, there is no element for this key
            //   (v[initial address] = 0).

            if (sctx.pos_is_empty(ia_pos)) {
                // check if we can insert directly

                auto location = sctx.allocate_pos(ia_pos);
                location.set_quotient(stored_quotient);

                // we created a new group, so update the bitflags
                set_cv(initial_address, 0b11);

                uint64_t global_id = local_id_to_global_id(initial_address, 0);
                return entry_t::found_new(global_id, location);
            } else {
                // check if there already is a group for this key
                bool const group_exists = get_v(initial_address);

                if (group_exists) {
                    auto const group = search_existing_group(initial_address);

                    // check if element already exists
                    auto r = search_in_group(group, stored_quotient);

                    if (r.found()) {
                        // There is a value for this key already.
                        DCHECK_EQ(r.ptr().get_quotient(), stored_quotient);

                        uint64_t global_id = local_id_to_global_id(
                            initial_address, r.id());
                        return entry_t::found_exist(global_id, r.ptr());
                    } else {
                        // Insert a new value
                        auto p = insert_value_after_group(group, stored_quotient);
                        p.set_quotient(stored_quotient);

                        uint64_t in_group_offset = size_mgr.mod_sub(
                            group.group_end, group.group_start);
                        uint64_t global_id = local_id_to_global_id(
                            initial_address, in_group_offset);
                        return entry_t::found_new(global_id, p);
                    }
                } else {
                    // insert a new group

                    // pretend we already inserted the new group
                    // this makes table_insert_value_after_group() find the group
                    // at the location _before_ the new group
                    set_v(initial_address, true);
                    auto const group = search_existing_group(initial_address);

                    // insert the element after the found group
                    auto p = insert_value_after_group(group, stored_quotient);
                    p.set_quotient(stored_quotient);

                    // mark the inserted element as the start of a new group,
                    // thus fixing-up the v <-> c mapping
                    set_c(group.group_end, true);

                    uint64_t global_id = local_id_to_global_id(
                        initial_address, 0);
                    return entry_t::found_new(global_id, p);
                }
            }
        }

        template<typename F>
        inline void for_all_allocated(F f) {
            auto sctx = storage.context(table_size, widths);

            // first, skip forward to the first empty location
            // so that iteration can start at the beginning of the first complete group

            size_t i = 0;
            for(;;i++) {
                if (sctx.pos_is_empty(sctx.table_pos(i))) {
                    break;
                }
            }

            // Remember our startpoint so that we can recognize it when
            // we wrapped around back to it
            size_t const original_start = i;

            // We proceed to the next position so that we can iterate until
            // we reach `original_start` again.
            uint64_t initial_address = i;
            i = size_mgr.mod_add(i);

            while(true) {
                auto sctx = storage.context(table_size, widths);
                while (sctx.pos_is_empty(sctx.table_pos(i))) {
                    if (i == original_start) {
                        return;
                    }

                    initial_address = i;
                    i = size_mgr.mod_add(i);
                }

                // If start of group, find next v bit to find initial address
                if (get_c(i)) {
                    initial_address = size_mgr.mod_add(initial_address);
                    while(!get_v(initial_address)) {
                        initial_address = size_mgr.mod_add(initial_address);
                    }
                }

                f(initial_address, i);

                i = size_mgr.mod_add(i);
            }
        }

        void print_all() {
            auto sctx = storage.context(table_size, widths);
            std::cout << "/////////////////\n";
            for(size_t i = 0; i < table_size; i++) {
                auto p = sctx.table_pos(i);
                if(sctx.pos_is_empty(p)) {
                    std::cout << "-- -\n";
                } else {
                    std::cout << int(get_c(i)) << int(get_v(i)) << " #\n";
                }
            }
            std::cout << "/////////////////\n";
        }

        template<typename F>
        inline void drain_all(F f) {
            table_pos_t drain_start;
            bool first = true;

            for_all_allocated([&](auto initial_address, auto i) {
                auto sctx = storage.context(table_size, widths);
                auto p = sctx.table_pos(i);

                if (first) {
                    first = false;
                    drain_start = p;
                }

                sctx.trim_storage(&drain_start, p);
                f(initial_address, sctx.at(p));
            });
        }

        inline entry_t search(uint64_t initial_address, uint64_t stored_quotient) {
            //std::cout << "search on cv(ia="<<initial_address<<", sq="<<stored_quotient<<")\n";
            if (get_v(initial_address)) {
                auto grp = search_existing_group(initial_address);
                auto r = search_in_group(grp, stored_quotient);
                if (!r.found()) {
                    return r;
                } else {
                    uint64_t global_id = local_id_to_global_id(
                        initial_address, r.id());
                    return entry_t::found_exist(global_id, r.ptr());
                }
            }
            return entry_t::not_found();
        }
    };
    template<typename storage_t, typename size_mgr_t>
    inline auto context(storage_t& storage,
                        size_t table_size,
                        typename storage_t::satellite_t_export::entry_bit_width_t const& widths,
                        size_mgr_t const& size_mgr) {
        return context_t<storage_t, size_mgr_t> {
            m_cv, table_size, widths, size_mgr, storage
        };
    }
};

}

template<>
struct heap_size<compact_hash::cv_bvs_t> {
    using T = compact_hash::cv_bvs_t;

    static object_size_t compute(T const& val, size_t table_size) {
        DCHECK_EQ(val.m_cv.size(), table_size);
        auto size = val.m_cv.stat_allocation_size_in_bytes();

        return object_size_t::exact(size);
    }
};

template<>
struct serialize<compact_hash::cv_bvs_t> {
    using T = compact_hash::cv_bvs_t;

    static object_size_t write(std::ostream& out, T const& val,
                               size_t table_size) {
        DCHECK_EQ(val.m_cv.size(), table_size);
        auto data = (char const*) val.m_cv.data();
        auto size = val.m_cv.stat_allocation_size_in_bytes();

        out.write(data, size);

        return object_size_t::exact(size);
    }

    static T read(std::istream& in,
                  size_t table_size) {
        auto cv = IntVector<uint_t<2>>();
        cv.reserve(table_size);
        cv.resize(table_size);
        auto data = (char*) cv.data();
        auto size = cv.stat_allocation_size_in_bytes();

        in.read(data, size);

        return T {
            std::move(cv)
        };
    }

    static bool equal_check(T const& lhs, T const& rhs, size_t table_size) {
        return gen_equal_diagnostic(lhs.m_cv == rhs.m_cv);
    }
};

}
