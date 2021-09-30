#pragma once

#include <tudocomp/util/compact_hash/util.hpp>
#include <tudocomp/util/compact_hash/size_manager_t.hpp>
#include <tudocomp/util/compact_hash/storage/buckets_bv_t.hpp>
#include <tudocomp/util/compact_hash/entry_t.hpp>

#include <tudocomp/util/compact_hash/set/no_satellite_data_t.hpp>

#include <tudocomp/util/serialization.hpp>

namespace tdc {namespace compact_hash {namespace set {

template<typename hash_t, typename placement_t>
class hashset_t {
    using storage_t = buckets_bv_t<no_satellite_data_t>;
    using satellite_t = typename storage_t::satellite_t_export;
public:
    /// runtime initilization arguments for the template config parameters
    struct config_args {
        typename size_manager_t::config_args size_manager_config;
        typename hash_t::config_args hash_config;
        typename storage_t::config_args storage_config;
        typename placement_t::config_args displacement_config;
    };

    /// this is called during a resize to copy over internal config values
    inline config_args current_config() const {
        auto r = config_args{};
        r.size_manager_config = m_sizing.current_config();
        r.hash_config = m_hash.current_config();
        r.storage_config = m_storage.current_config();
        r.displacement_config = m_placement.current_config();
        return r;
    }

    /// Default value of the `key_width` parameter of the constructor.
    static constexpr size_t DEFAULT_KEY_WIDTH = 1;
    static constexpr size_t DEFAULT_TABLE_SIZE = 0;

    inline hashset_t(hashset_t&& other):
        m_sizing(std::move(other.m_sizing)),
        m_key_width(std::move(other.m_key_width)),
        m_storage(std::move(other.m_storage)),
        m_placement(std::move(other.m_placement)),
        m_hash(std::move(other.m_hash))
    {
    }
    inline hashset_t& operator=(hashset_t&& other) {
        m_sizing = std::move(other.m_sizing);
        m_key_width = std::move(other.m_key_width);
        m_storage = std::move(other.m_storage);
        m_placement = std::move(other.m_placement);
        m_hash = std::move(other.m_hash);

        return *this;
    }
    // NB: These just exist to catch bugs, and could be removed
    inline hashset_t(hashset_t const& other) = delete;
    inline hashset_t& operator=(hashset_t  const& other) = delete;

    /// Constructs a hashtable with a initial table size `size`,
    /// and a initial key bit-width `key_width`.
    inline hashset_t(size_t size = DEFAULT_TABLE_SIZE,
                     size_t key_width = DEFAULT_KEY_WIDTH,
                     config_args config = config_args{}):
        m_sizing(size, config.size_manager_config),
        m_key_width(key_width),
        m_storage(table_size(), storage_widths(), config.storage_config),
        m_placement(table_size(), config.displacement_config),
        m_hash(real_width(), config.hash_config)
    {
    }

    /// Returns the amount of elements inside the datastructure.
    inline size_t size() const {
        return m_sizing.size();
    }

    /// Returns the current size of the hashtable.
    /// This value is greater-or-equal the amount of the elements
    /// currently contained in it, which is represented by `size()`.
    inline size_t table_size() const {
        return m_sizing.capacity();
    }

    /// Current width of the keys stored in this datastructure.
    inline size_t key_width() const {
        return m_key_width;
    }

    /// Amount of bits of the key, that are stored implicitly
    /// by its position in the table.
    inline size_t initial_address_width() const {
        return m_sizing.capacity_log2();
    }

    /// Amount of bits of the key, that are stored explicitly
    /// in the buckets.
    inline size_t quotient_width() const {
        return real_width() - m_sizing.capacity_log2();
    }

    /// Sets the maximum load factor
    /// (how full the table can get before re-allocating).
    ///
    /// Expects a value `0.0 < z < 1.0`.
    inline void max_load_factor(float z) {
        m_sizing.max_load_factor(z);
    }

    /// Returns the maximum load factor.
    inline float max_load_factor() const noexcept {
        return m_sizing.max_load_factor();
    }

    struct default_on_resize_t {
        /// Will be called in case of an resize.
        inline void on_resize(size_t table_size) {}
        /// Will be called after `on_resize()` for each element
        /// that gets re-inserted into the new set.
        inline void on_reinsert(uint64_t key, uint64_t id) {}
    };

    using entry_t = generic_entry_t<typename satellite_t::entry_ptr_t>;

    /// Looks up the key `key` in the set, inserting it if
    /// it doesn't already exist.
    ///
    /// The returned `entry_t` contains both an _id_ that is unique for each
    /// element in the set for a given table size,
    /// and a boolean indicating if the key already exists.
    ///
    /// If the set needs to be resized, the observer `on_resize` will be
    /// used to notify the code about the changed size and new key-id mappings.
    template<typename on_resize_t = default_on_resize_t>
    inline entry_t lookup_insert(uint64_t key,
                                 on_resize_t&& on_resize = on_resize_t()) {
        return lookup_insert_key_width(key, key_width(), on_resize);
    }

    /// Looks up the key `key` in the set, inserting it if
    /// it doesn't already exist, and grows the key width to `key_width`
    /// bits.
    ///
    /// The returned `entry_t` contains both an _id_ that is unique for each
    /// element in the set for a given table size,
    /// and a boolean indicating if the key already exists.
    ///
    /// If the set needs to be resized, the observer `on_resize` will be
    /// used to notify the code about the changed size and new key-id mappings.
    template<typename on_resize_t = default_on_resize_t>
    inline entry_t lookup_insert_key_width(uint64_t key,
                                           uint8_t key_width,
                                           on_resize_t&& on_resize = on_resize_t()) {
        auto raw_key_width = std::max<size_t>(key_width, this->key_width());
        return grow_and_insert(key, raw_key_width, on_resize);
    }

    /// Grow the key width as needed.
    ///
    /// Note that it is more efficient to change the width directly during
    /// insertion of a new value.
    template<typename on_resize_t = default_on_resize_t>
    inline void grow_key_width(size_t key_width,
                               on_resize_t&& on_resize = on_resize_t()) {
        auto raw_key_width = std::max<size_t>(key_width, this->key_width());
        grow_if_needed(size(), raw_key_width, on_resize);
    }

    /// Search for a key inside the hashset.
    ///
    /// The returned `entry_t` contains a boolean indicating if the key was found.
    /// If it is, then it contains the corresponding _id_ of the entry.
    inline entry_t lookup(uint64_t key) {
        auto dkey = decompose_key(key);
        auto pctx = m_placement.context(m_storage, table_size(), storage_widths(), m_sizing);
        return pctx.search(dkey.initial_address, dkey.stored_quotient);
    }

    /// Swap this instance of the data structure with another one.
    inline void swap(hashset_t& other) {
        std::swap(*this, other);
    }

    /// Moves the contents of this hashtable
    /// into another table.
    ///
    /// This method tries to eagerly free memory in
    /// order to keep the total consumption low, if possible.
    ///
    /// The target hashtable will grow as needed. To prevent that, ensure its
    /// capacity and bit widths are already large enough.
    ///
    /// The `on_resize` handler will call `on_reinsert()` for
    /// each moved element. It will not be called for growth operations
    /// of the target hashtable.
    template<typename on_resize_t = default_on_resize_t>
    inline void move_into(hashset_t& other,
                          on_resize_t&& on_resize = on_resize_t()) {
        auto pctx = m_placement.context(m_storage, table_size(), storage_widths(), m_sizing);
        pctx.drain_all([&](auto initial_address, auto kv) {
            auto stored_quotient = kv.get_quotient();
            auto key = this->compose_key(initial_address, stored_quotient);
            auto r = other.lookup_insert(key);
            DCHECK(r.found());
            DCHECK(!r.key_already_exist());
            on_resize.on_reinsert(key, r.id());
        });
    }

    /// Check wether for the `new_size` this hashtable would need
    /// to perform a grow of the capacity.
    inline bool needs_to_grow_capacity(size_t new_size) const {
        return m_sizing.needs_to_grow_capacity(m_sizing.capacity(), new_size);
    }

    /// Check wether for the `new_size` and `new_key_width` this
    /// hashtable would need to reallocate.
    inline bool needs_to_realloc(size_t new_size,
                                 size_t new_key_width) const {
        return needs_to_grow_capacity(new_size)
            || (new_key_width != key_width());
    }

    /// Compute the new capacity the hashmap would have after a grow
    /// operation for `new_size`.
    inline size_t grown_capacity(size_t new_size) const {
        size_t new_capacity = m_sizing.capacity();
        while (m_sizing.needs_to_grow_capacity(new_capacity, new_size)) {
            new_capacity = m_sizing.grown_capacity(new_capacity);
        }
        return new_capacity;
    }

    /// Pseudo-Pointer to a key.
    ///
    /// Does not actually point at a memory location, and defines equality
    ///  based on the key value and wether this is in its `null` state.
    class pointer_type {
        uint64_t m_key;
        bool m_empty;
    public:
        pointer_type(uint64_t key): m_key(key), m_empty(false) {}
        pointer_type(): m_key(-1), m_empty(true) {}

        inline uint64_t& operator*() {
            return m_key;
        }
        inline uint64_t* operator->() {
            return &m_key;
        }
        inline bool operator==(pointer_type const& other) const {
            return (m_empty == other.m_empty) && (m_key == other.m_key);
        }
        inline bool operator!=(pointer_type const& other) const {
            return !(*this == other);
        }
        inline bool operator==(uint64_t const* const& other) const {
            return (other != nullptr) && (m_key == *other);
        }
        inline bool operator!=(uint64_t const* const& other) const {
            return !(*this == other);
        }
    };

    /// Search for a key inside the hashtable.
    ///
    /// This returns a pseudo-pointer to the key if its found, or null
    /// otherwise. This method exists for STL-compability.
    inline pointer_type find(uint64_t key) {
        if  (count(key)) {
            return pointer_type(key);
        } else {
            return pointer_type();
        }
    }

    /// Count the number of occurrences of `key`, as defined on STL containers.
    ///
    /// It will return either 0 or 1.
    inline size_t count(uint64_t key) {
        return lookup(key).found();
    }

private:
    using quot_width_t = typename satellite_t::entry_bit_width_t;

    /// Size of table, and width of the stored keys and values
    size_manager_t m_sizing;
    uint8_t m_key_width;

    /// Storage of the table elements
    storage_t m_storage;

    /// Placement management structures
    placement_t m_placement;

    /// Hash function
    hash_t m_hash {1};

    template<typename T>
    friend struct ::tdc::serialize;

    template<typename T>
    friend struct ::tdc::heap_size;

    /// The actual amount of bits currently usable for
    /// storing a key in the hashtable.
    ///
    /// Due to implementation details, this can be
    /// larger than `key_width()`.
    ///
    /// Specifically, there are currently two cases:
    /// - If all bits of the the key fit into the initial-address space,
    ///   then the quotient bitvector inside the buckets would
    ///   have to store integers of width 0. This is undefined behavior
    ///   with the current code, so we add a padding bit.
    /// - Otherwise the current maximum key width `m_key_width`
    ///   determines the real width.
    inline size_t real_width() const {
        return std::max<size_t>(m_sizing.capacity_log2() + 1, m_key_width);
    }

    inline quot_width_t storage_widths() const {
        return uint8_t(quotient_width());
    }

    /// Debug check that a key does not occupy more bits than the
    /// hashtable currently allows.
    inline bool dcheck_key_width(uint64_t key) {
        uint64_t key_mask = (1ull << (key_width() - 1ull) << 1ull) - 1ull;
        bool key_is_too_large = key & ~key_mask;
        return !key_is_too_large;
    }

    /// Decompose a key into its initial address and quotient.
    inline decomposed_key_t decompose_key(uint64_t key) {
        DCHECK(dcheck_key_width(key)) << "Attempt to decompose key " << key << ", which requires more than the current set maximum of " << key_width() << " bits, but should not.";

        uint64_t hres = m_hash.hash(key);

        DCHECK_EQ(m_hash.hash_inv(hres), key);

        return m_sizing.decompose_hashed_value(hres);
    }

    /// Compose a key from its initial address and quotient.
    inline uint64_t compose_key(uint64_t initial_address, uint64_t quotient) {
        uint64_t harg = m_sizing.compose_hashed_value(initial_address, quotient);
        uint64_t key = m_hash.hash_inv(harg);

        DCHECK(dcheck_key_width(key)) << "Composed key " << key << ", which requires more than the current set maximum of " << key_width() << " bits, but should not.";
        return key;
    }

    /// Access the element represented by `handler` under
    /// the key `key` with the, possibly new, width of `key_width` bits.
    ///
    /// `handler` is a type that allows reacting correctly to different ways
    /// to access or create a new or existing value in the hashtable.
    /// See `InsertHandler` and `AddressDefaultHandler` below.
    template<typename on_resize_t>
    inline auto grow_and_insert(uint64_t key, size_t key_width, on_resize_t& onr) {
        grow_if_needed(this->size() + 1, key_width, onr);
        auto const dkey = this->decompose_key(key);

        DCHECK_EQ(key, this->compose_key(dkey.initial_address, dkey.stored_quotient));

        auto pctx = m_placement.context(m_storage, table_size(), storage_widths(), m_sizing);

        auto result = pctx.lookup_insert(dkey.initial_address, dkey.stored_quotient);

        if (!result.key_already_exist()) {
            m_sizing.set_size(m_sizing.size() + 1);
        }

        return result;
    }

    /// Check the current key width and table site against the arguments,
    /// and grows the table or quotient bitvectors as needed.
    template<typename on_resize_t>
    inline void grow_if_needed(size_t const new_size,
                               size_t const new_key_width,
                               on_resize_t& onr) {
        /*
        std::cout
                << "buckets size/cap: " << m_buckets.size()
                << ", size: " << m_sizing.size()
                << "\n";
        */

        // TODO: Could reuse the existing table if only m_key_width changes
        // TODO: The iterators is inefficient since it does redundant
        // memory lookups and address calculations.

        if (needs_to_realloc(new_size, new_key_width)) {
            size_t new_capacity = grown_capacity(new_size);
            auto config = this->current_config();
            auto new_table = hashset_t<hash_t, placement_t>(
                new_capacity, new_key_width, config);

            /*
            std::cout
                << "grow to cap " << new_table.table_size()
                << ", key_width: " << new_table.key_width()
                << ", val_width: " << new_table.value_width()
                << ", real_width: " << new_table.real_width()
                << ", quot width: " << new_table.quotient_width()
                << "\n";
            */

            onr.on_resize(new_capacity);

            move_into(new_table, onr);

            *this = std::move(new_table);
        }

        DCHECK(!needs_to_realloc(new_size, new_key_width));
    }
};

}}

template<typename hash_t, typename placement_t>
struct heap_size<compact_hash::set::hashset_t<hash_t, placement_t>> {
    using T = compact_hash::set::hashset_t<hash_t, placement_t>;
    using storage_t = typename T::storage_t;

    static object_size_t compute(T const& val) {
        using namespace compact_hash::set;
        using namespace compact_hash;

        auto bytes = object_size_t::empty();

        bytes += heap_size<size_manager_t>::compute(val.m_sizing);
        bytes += heap_size<uint8_t>::compute(val.m_key_width);
        bytes += heap_size<hash_t>::compute(val.m_hash);
        bytes += heap_size<storage_t>::compute(
            val.m_storage, val.table_size(), val.storage_widths());
        bytes += heap_size<placement_t>::compute(
            val.m_placement, val.table_size());

        return bytes;
    }
};

template<typename hash_t, typename placement_t>
struct serialize<compact_hash::set::hashset_t<hash_t, placement_t>> {
    using T = compact_hash::set::hashset_t<hash_t, placement_t>;
    using storage_t = typename T::storage_t;

    static object_size_t write(std::ostream& out, T const& val) {
        using namespace compact_hash::set;
        using namespace compact_hash;

        auto bytes = object_size_t::empty();

        bytes += serialize<size_manager_t>::write(out, val.m_sizing);
        bytes += serialize<uint8_t>::write(out, val.m_key_width);
        bytes += serialize<hash_t>::write(out, val.m_hash);
        bytes += serialize<storage_t>::write(
            out, val.m_storage, val.table_size(), val.storage_widths());
        bytes += serialize<placement_t>::write(
            out, val.m_placement, val.table_size());

        return bytes;
    }
    static T read(std::istream& in) {
        using namespace compact_hash::set;
        using namespace compact_hash;

        T ret;

        auto sizing = serialize<size_manager_t>::read(in);
        auto key_width = serialize<uint8_t>::read(in);
        auto hash = serialize<hash_t>::read(in);
        ret.m_sizing = std::move(sizing);
        ret.m_key_width = std::move(key_width);
        ret.m_hash = std::move(hash);

        auto storage = serialize<storage_t>::read(in, ret.table_size(), ret.storage_widths());
        auto placement = serialize<placement_t>::read(in, ret.table_size());

        ret.m_storage = std::move(storage);
        ret.m_placement = std::move(placement);

        return ret;
    }
    static bool equal_check(T const& lhs, T const& rhs) {
        if (!(gen_equal_check(table_size()) && gen_equal_check(storage_widths()))) {
            return false;
        }

        auto table_size = lhs.table_size();
        auto storage_widths = lhs.storage_widths();

        bool deep_eq = gen_equal_check(m_sizing)
        && gen_equal_check(m_key_width)
        && gen_equal_check(m_hash)
        && gen_equal_check(m_storage, table_size, storage_widths)
        && gen_equal_check(m_placement, table_size);

        return deep_eq;
    }
};

}
