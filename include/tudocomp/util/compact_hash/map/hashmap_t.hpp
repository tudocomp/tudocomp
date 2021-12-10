#pragma once

#include <tudocomp/util/compact_hash/util.hpp>
#include <tudocomp/util/compact_hash/size_manager_t.hpp>
#include <tudocomp/util/serialization.hpp>

#include <tudocomp/util/compact_hash/map/satellite_data_t.hpp>

#include <tudocomp/util/serialization.hpp>

namespace tdc {namespace compact_hash{namespace map {

template<typename val_t, typename hash_t, template<typename> typename storage_t, typename placement_t>
class hashmap_t {
    using storage_app_t = storage_t<satellite_data_t<val_t>>;
    using satellite_t = typename storage_app_t::satellite_t_export;
public:
    /// runtime initilization arguments for the template config parameters
    struct config_args {
        typename size_manager_t::config_args size_manager_config;
        typename hash_t::config_args hash_config;
        typename storage_app_t::config_args storage_config;
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

    /// By-value representation of a value
    using value_type = typename cbp::cbp_repr_t<val_t>::value_type;
    /// Reference to a value
    using reference_type = ValRef<val_t>;
    /// Pointer to a value
    using pointer_type = ValPtr<val_t>;

    /// Default value of the `key_width` parameter of the constructor.
    static constexpr size_t DEFAULT_KEY_WIDTH = 1;
    static constexpr size_t DEFAULT_VALUE_WIDTH = 1;
    static constexpr size_t DEFAULT_TABLE_SIZE = 0;

    inline hashmap_t(hashmap_t&& other):
        m_sizing(std::move(other.m_sizing)),
        m_key_width(std::move(other.m_key_width)),
        m_val_width(std::move(other.m_val_width)),
        m_storage(std::move(other.m_storage)),
        m_placement(std::move(other.m_placement)),
        m_hash(std::move(other.m_hash)),
        m_is_empty(std::move(other.m_is_empty))
    {
        other.m_is_empty = true;
    }
    inline hashmap_t& operator=(hashmap_t&& other) {
        // NB: overwriting the storage does not automatically destroy the values in them.
        destroy_vals();

        m_sizing = std::move(other.m_sizing);
        m_key_width = std::move(other.m_key_width);
        m_val_width = std::move(other.m_val_width);
        m_storage = std::move(other.m_storage);
        m_placement = std::move(other.m_placement);
        m_hash = std::move(other.m_hash);
        m_is_empty = std::move(other.m_is_empty);

        other.m_is_empty = true;

        return *this;
    }
    // NB: These just exist to catch bugs, and could be removed
    inline hashmap_t(hashmap_t const& other) = delete;
    inline hashmap_t& operator=(hashmap_t  const& other) = delete;

    /// Constructs a hashtable with a initial table size `size`,
    /// and a initial key bit-width `key_width`.
    inline hashmap_t(size_t size = DEFAULT_TABLE_SIZE,
                     size_t key_width = DEFAULT_KEY_WIDTH,
                     size_t value_width = DEFAULT_VALUE_WIDTH,
                     config_args config = config_args{}):
        m_sizing(size, config.size_manager_config),
        m_key_width(key_width),
        m_val_width(value_width),
        m_storage(table_size(), storage_widths(), config.storage_config),
        m_placement(table_size(), config.displacement_config),
        m_hash(real_width(), config.hash_config)
    {
    }

    inline ~hashmap_t() {
        if (!m_is_empty) {
            // NB: overwriting the storage does not automatically destroy the values in them.
            destroy_vals();
        }
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

    /// Current width of the values stored in this datastructure.
    inline size_t value_width() const {
        return m_val_width;
    }

    /// Amount of bits of the key, that are stored implicitly
    /// by its position in the table.
    inline size_t initial_address_width() {
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

    /// Inserts a key-value pair into the hashtable.
    inline void insert(uint64_t key, value_type&& value) {
        insert_kv_width(key, std::move(value), key_width(), value_width());
    }

    /// Inserts a key-value pair into the hashtable,
    /// and grow the key width as needed.
    inline void insert_key_width(uint64_t key, value_type&& value, uint8_t key_width) {
        insert_kv_width(key, std::move(value), key_width, value_width());
    }

    /// Inserts a key-value pair into the hashtable,
    /// and grow the key and value width as needed.
    inline void insert_kv_width(uint64_t key, value_type&& value, uint8_t key_width, uint8_t value_width) {
        auto raw_key_width = std::max<size_t>(key_width, this->key_width());
        auto raw_val_width = std::max<size_t>(value_width, this->value_width());

        auto result = grow_and_insert(key, raw_key_width, raw_val_width);

        if (!result.key_already_exist()) {
            result.ptr().set_val_no_drop(std::move(value));
        } else {
            result.ptr().set_val(std::move(value));
        }
    }

    /// Returns a reference to the element with key `key`.
    ///
    /// If the value does not already exist in the table, it will be
    /// default-constructed.
    inline reference_type access(uint64_t key) {
        return access_kv_width(key, key_width(), value_width());
    }

    /// Returns a reference to the element with key `key`,
    /// and grow the key width as needed.
    ///
    /// If the value does not already exist in the table, it will be
    /// default-constructed.
    inline reference_type access_key_width(uint64_t key, uint8_t key_width) {
        return access_kv_width(key, key_width, value_width());
    }

    /// Returns a reference to the element with key `key`,
    /// and grow the key and value width as needed.
    ///
    /// If the value does not already exist in the table, it will be
    /// default-constructed.
    inline reference_type access_kv_width(uint64_t key, uint8_t key_width, uint8_t value_width) {
        auto raw_key_width = std::max<size_t>(key_width, this->key_width());
        auto raw_val_width = std::max<size_t>(value_width, this->value_width());

        auto result = grow_and_insert(key, raw_key_width, raw_val_width);

        if (!result.key_already_exist()) {
            result.ptr().set_val_no_drop(value_type());
        }

        pointer_type addr = result.ptr().val_ptr();
        DCHECK(addr != pointer_type());
        return *addr;
    }

    /// Returns a reference to the element with key `key`.
    ///
    /// This has the same semantic is `access(key)`.
    inline reference_type operator[](uint64_t key) {
        return access(key);
    }

    /// Grow the key width as needed.
    ///
    /// Note that it is more efficient to change the width directly during
    /// insertion of a new value.
    inline void grow_key_width(size_t key_width) {
        auto raw_key_width = std::max<size_t>(key_width, this->key_width());
        grow_if_needed(size(), raw_key_width, value_width());
    }

    /// Grow the key and value width as needed.
    ///
    /// Note that it is more efficient to change the width directly during
    /// insertion of a new value.
    inline void grow_kv_width(size_t key_width, size_t value_width) {
        auto raw_key_width = std::max<size_t>(key_width, this->key_width());
        auto raw_val_width = std::max<size_t>(value_width, this->value_width());
        grow_if_needed(size(), raw_key_width, raw_val_width);
    }

    /// Search for a key inside the hashtable.
    ///
    /// This returns a pointer to the value if its found, or null
    /// otherwise.
    inline pointer_type search(uint64_t key) {
        auto dkey = decompose_key(key);
        auto pctx = m_placement.context(m_storage, table_size(), storage_widths(), m_sizing);
        auto r = pctx.search(dkey.initial_address, dkey.stored_quotient);
        if (r.found()) {
            return r.ptr().val_ptr();
        } else {
            return pointer_type();
        }
    }


    /// Moves the contents of this hashtable
    /// into another table.
    ///
    /// This method tries to eagerly free memory in
    /// order to keep the total consumption low, if possible.
    ///
    /// The target hashtable will grow as needed. To prevent that, ensure its
    /// capacity and bit widths are already large enough.
    inline void move_into(hashmap_t& other) {
        auto pctx = m_placement.context(m_storage, table_size(), storage_widths(), m_sizing);
        pctx.drain_all([&](auto initial_address, auto kv) {
            auto stored_quotient = kv.get_quotient();
            auto key = this->compose_key(initial_address, stored_quotient);
            other.insert(key, std::move(*kv.val_ptr()));
        });
    }

    /// Check wether for the `new_size` this hashtable would need
    /// to perform a grow of the capacity
    inline bool needs_to_grow_capacity(size_t new_size) const {
        return m_sizing.needs_to_grow_capacity(m_sizing.capacity(), new_size);
    }

    /// Check wether for the `new_size`, `new_key_width` and
    /// `new_value_width` this hashtable would need to reallocate.
    inline bool needs_to_realloc(size_t new_size,
                                 size_t new_key_width,
                                 size_t new_value_width) const {
        return needs_to_grow_capacity(new_size)
            || (new_key_width != key_width())
            || (new_value_width != value_width());
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

    /// Search for a key inside the hashtable.
    ///
    /// This returns a pointer to the value if its found, or null
    /// otherwise. The behavior of this method is the same as `search()`, and
    /// exists for STL-compability.
    inline pointer_type find(uint64_t key) {
        return search(key);
    }

    /// exists for STL-compability.
    pointer_type end() const { return pointer_type(); }

    /// Count the number of occurrences of `key`, as defined on STL containers.
    ///
    /// It will return either 0 or 1.
    inline size_t count(uint64_t key) {
        return search(key) != pointer_type();
    }

    inline std::string debug_print_storage() {
        std::stringstream ss;

        auto sctx = m_storage.context(table_size(), storage_widths());
        for (size_t i = 0; i < table_size(); i++) {
            auto p = sctx.table_pos(i);
            std::stringstream ss2;
            ss2 << i << ": \t";
            if (sctx.pos_is_empty(p)) {
                ss2 << "-";
            } else {
                auto ptr = sctx.at(p);
                ss2 << "(q=" << ptr.get_quotient()
                   << ", v=" << (*ptr.val_ptr()) << ")";
            }
            ss2 << "\n";
            auto x = ss2.str();
            ss << x;
            //std::cout << x;
        }
        return ss.str();
    }

private:
    using widths_t = typename satellite_t::entry_bit_width_t;

    /// Size of table, and width of the stored keys and values
    size_manager_t m_sizing;
    uint8_t m_key_width;
    uint8_t m_val_width;

    /// Storage of the table elements
    storage_app_t m_storage;

    /// Placement management structures
    placement_t m_placement;

    /// Hash function
    hash_t m_hash {1};

    /// Marker for correctly handling moving-out
    bool m_is_empty = false;

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

    inline widths_t storage_widths() const {
        return { uint8_t(quotient_width()), uint8_t(value_width()) };
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

    /// Run the destructors of the bucket elements,
    /// but don't drop them from the table.
    ///
    /// WARNING: This should only be called
    /// before an operation that would call the destructors of the buckets
    /// themselves, like in the destructor of the hashtable.
    ///
    /// The reason this exists is that a bucket_t does not
    /// initialize or destroy the elements in it automatically,
    /// to prevent unneeded empty-constructions of its elements.
    /// TODO: Is this still a useful semantic? A bucket_t can manage its own data.
    inline void destroy_vals() {
        auto sctx = m_storage.context(table_size(), storage_widths());
        sctx.destroy_vals();
    }

    /// Access the element represented by `handler` under
    /// the key `key` with the, possibly new, width of `key_width` bits.
    ///
    /// `handler` is a type that allows reacting correctly to different ways
    /// to access or create a new or existing value in the hashtable.
    /// See `InsertHandler` and `AddressDefaultHandler` below.
    inline auto grow_and_insert(uint64_t key, size_t key_width, size_t value_width) {
        grow_if_needed(this->size() + 1, key_width, value_width);
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
    inline void grow_if_needed(size_t const new_size,
                               size_t const new_key_width,
                               size_t const new_value_width) {
        /*
        std::cout
                << "buckets size/cap: " << m_buckets.size()
                << ", size: " << m_sizing.size()
                << "\n";
        */

        // TODO: Could reuse the existing table if only m_key_width changes
        // TODO: The iterators is inefficient since it does redundant
        // memory lookups and address calculations.

        if (needs_to_realloc(new_size, new_key_width, new_value_width)) {
            size_t new_capacity = grown_capacity(new_size);
            auto config = this->current_config();
            auto new_table = hashmap_t<val_t, hash_t, storage_t, placement_t>(
                new_capacity, new_key_width, new_value_width, config);

            /*
            std::cout
                << "grow to cap " << new_table.table_size()
                << ", key_width: " << new_table.key_width()
                << ", val_width: " << new_table.value_width()
                << ", real_width: " << new_table.real_width()
                << ", quot width: " << new_table.quotient_width()
                << "\n";
            */

            move_into(new_table);

            *this = std::move(new_table);
        }

        DCHECK(!needs_to_realloc(new_size, new_key_width, new_value_width));
    }
};

}}

template<typename val_t, typename hash_t, template<typename> typename storage_t, typename placement_t>
struct heap_size<compact_hash::map::hashmap_t<val_t, hash_t, storage_t, placement_t>> {
    using T = compact_hash::map::hashmap_t<val_t, hash_t, storage_t, placement_t>;

    static object_size_t compute(T const& val) {
        using namespace compact_hash::map;
        using namespace compact_hash;

        auto bytes = object_size_t::empty();

        bytes += heap_size<size_manager_t>::compute(val.m_sizing);
        bytes += heap_size<uint8_t>::compute(val.m_key_width);
        bytes += heap_size<uint8_t>::compute(val.m_val_width);
        bytes += heap_size<hash_t>::compute(val.m_hash);
        bytes += heap_size<typename T::storage_app_t>::compute(
            val.m_storage, val.table_size(), val.storage_widths());
        bytes += heap_size<placement_t>::compute(val.m_placement, val.table_size());
        bytes += heap_size<uint8_t>::compute(val.m_is_empty);

        return bytes;
    }
};

template<typename val_t, typename hash_t, template<typename> typename storage_t, typename placement_t>
struct serialize<compact_hash::map::hashmap_t<val_t, hash_t, storage_t, placement_t>> {
    using T = compact_hash::map::hashmap_t<val_t, hash_t, storage_t, placement_t>;

    static object_size_t write(std::ostream& out, T const& val) {
        using namespace compact_hash::map;
        using namespace compact_hash;

        auto bytes = object_size_t::empty();

        bytes += serialize<size_manager_t>::write(out, val.m_sizing);
        bytes += serialize<uint8_t>::write(out, val.m_key_width);
        bytes += serialize<uint8_t>::write(out, val.m_val_width);
        bytes += serialize<hash_t>::write(out, val.m_hash);
        bytes += serialize<typename T::storage_app_t>::write(
            out, val.m_storage, val.table_size(), val.storage_widths());
        bytes += serialize<placement_t>::write(out, val.m_placement, val.table_size());
        bytes += serialize<uint8_t>::write(out, val.m_is_empty);

        return bytes;
    }
    static T read(std::istream& in) {
        using namespace compact_hash::map;
        using namespace compact_hash;

        T ret;

        auto sizing = serialize<size_manager_t>::read(in);
        auto key_width = serialize<uint8_t>::read(in);
        auto val_width = serialize<uint8_t>::read(in);
        auto hash = serialize<hash_t>::read(in);
        ret.m_sizing = std::move(sizing);
        ret.m_key_width = std::move(key_width);
        ret.m_val_width = std::move(val_width);
        ret.m_hash = std::move(hash);

        auto storage = serialize<typename T::storage_app_t>::read(in, ret.table_size(), ret.storage_widths());
        auto placement = serialize<placement_t>::read(in, ret.table_size());

        ret.m_storage = std::move(storage);
        ret.m_placement = std::move(placement);

        auto is_empty = serialize<uint8_t>::read(in);
        ret.m_is_empty = std::move(is_empty);

        return ret;
    }
    static bool equal_check(T const& lhs, T const& rhs) {
        if (!(gen_equal_check(table_size()) && gen_equal_check(storage_widths().quot_width)&& gen_equal_check(storage_widths().val_width))) {
            return false;
        }

        auto table_size = lhs.table_size();
        auto storage_widths = lhs.storage_widths();

        bool deep_eq = gen_equal_check(m_sizing)
        && gen_equal_check(m_key_width)
        && gen_equal_check(m_val_width)
        && gen_equal_check(m_hash)
        && gen_equal_check(m_storage, table_size, storage_widths)
        && gen_equal_check(m_placement, table_size)
        && gen_equal_check(m_is_empty);

        return deep_eq;
    }
};

}
