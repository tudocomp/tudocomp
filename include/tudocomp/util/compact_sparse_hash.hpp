#pragma once

#include <memory>
#include <cstdint>
#include <utility>
#include <algorithm>

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/IntPtr.hpp>

namespace tdc {

/*
    uint64_t x;

uint64_t xorshift64star(uint64_t state[static 1]) {
    uint64_t x = state[0];
    x ^= x >> 12; // a
    x ^= x << 25; // b
    x ^= x >> 27; // c
    state[0] = x;
    return x * 0x2545F4914F6CDD1D;
}

unsigned long long xor64(){
static unsigned long long x=88172645463325252LL;
xˆ=(x<<13); xˆ=(x>>7); return (xˆ=(x<<17));
*/

inline uint64_t compact_hashfn(uint64_t x, uint64_t w)  {
    uint64_t j = (w / 2ull) + 1;
    DCHECK_LT((w / 2ull), j);
    DCHECK_NE(w, 0);

    // NB: Two shifts because a single shift with w == 64 is undefined
    // behavior
    uint64_t w_mask = (1ull << (w - 1ull) << 1ull) - 1ull;

    return (x xor ((x << j) & w_mask)) & w_mask;
}

inline uint64_t compact_reverse_hashfn(uint64_t x, uint64_t w)  {
    return compact_hashfn(x, w);
}

inline uint8_t log2_upper(uint64_t v) {
    uint8_t m = 0;
    uint64_t n = v;
    while(n) {
        n >>= 1;
        m++;
    }
    m--;
    return m;
}

inline bool is_pot(size_t n) {
    return (n > 0ull && ((n & (n - 1ull)) == 0ull));
}

class SizeManager {
    uint8_t m_capacity_log2;
    size_t m_size;

public:

    inline SizeManager(size_t capacity) {
        m_size = 0;
        CHECK(is_pot(capacity));
        m_capacity_log2 = log2_upper(capacity);
    }

    inline size_t& size() {
        return m_size;
    }

    inline size_t const& size() const {
        return m_size;
    }

    inline size_t capacity() const {
        return 1ull << m_capacity_log2;
    }

    inline uint8_t capacity_log2() const {
        return m_capacity_log2;
    }
};

using QuotPtr = IntPtr<dynamic_t>;
inline QuotPtr make_quot_ptr(uint64_t* ptr, size_t quot_width) {
    using namespace int_vector;

    return IntPtrBase<QuotPtr>(ptr, 0, quot_width);
}

template<typename val_t>
class BucketElem;

template<typename val_t>
class Bucket {
    std::unique_ptr<uint64_t[]> m_data;

public:
    inline bool is_empty() const {
        return m_data.get() == nullptr;
    }

private:

    static inline bool is_aligned(void const* const pointer, size_t byte_count) {
        return (uintptr_t)pointer % byte_count == 0;
    }

    static inline size_t size(uint64_t bv) {
        return __builtin_popcountll(bv);
    }

    struct Layout {
        size_t vals_qword_offset;
        size_t quots_qword_offset;
        size_t overall_qword_size;
    };
    inline static Layout calc_sizes(size_t size, size_t quot_width) {
        DCHECK_NE(size, 0);
        DCHECK_LE(alignof(val_t), alignof(uint64_t));

        size_t vals_size_in_bytes = sizeof(val_t) * size;
        size_t vals_size_in_qwords = (vals_size_in_bytes + 7) / 8;

        uint64_t quots_size_in_bits = quot_width * size;
        size_t quots_size_in_qwords = (quots_size_in_bits + 63ull) / 64ull;

        Layout r;

        r.vals_qword_offset = 1;
        r.quots_qword_offset = r.vals_qword_offset + vals_size_in_qwords;

        r.overall_qword_size = 1 + vals_size_in_qwords + quots_size_in_qwords;

        /*
        std::cout
            << "size=" << size
            << ", quot_width=" << quot_width
            << ", sizeof(val_t)=" << sizeof(val_t)
            << ", alignof(val_t)=" << alignof(val_t)
            << ", sizeof(uint64_t)=" << sizeof(uint64_t)
            << ", alignof(uint64_t)=" << alignof(uint64_t)
            << ", vals_qword_offset=" << r.vals_qword_offset
            << ", quots_qword_offset=" << r.quots_qword_offset
            << ", overall_qword_size=" << r.overall_qword_size
            << "\n";
        */

        return r;
    }

    struct Ptrs {
        val_t* vals_ptr;
        QuotPtr quots_ptr;
    };
    inline Ptrs ptrs(size_t quot_width) const {
        DCHECK(!is_empty());
        size_t size = this->size();

        auto layout = calc_sizes(size, quot_width);

        uint64_t* const vals_start = &m_data[layout.vals_qword_offset];
        uint64_t* const quots_ptr = &m_data[layout.quots_qword_offset];

        val_t* const vals_ptr = reinterpret_cast<val_t*>(vals_start);

        DCHECK(is_aligned(vals_ptr, alignof(val_t))) << (void*)vals_ptr << ", " << alignof(val_t);
        DCHECK(is_aligned(quots_ptr, alignof(uint64_t)));

        return Ptrs {
            vals_ptr,
            make_quot_ptr(quots_ptr, quot_width),
        };
    }

public:
    inline Bucket(): m_data() {}
    explicit inline Bucket(uint64_t bv, size_t quot_width) {
        if (bv != 0) {
            auto layout = calc_sizes(size(bv), quot_width);

            m_data = std::make_unique<uint64_t[]>(layout.overall_qword_size);
            m_data[0] = bv;

            // NB: We call this for its alignment asserts
            ptrs(quot_width);
        } else {
            m_data.reset();
        }
    }

    inline uint64_t bv() const {
        if (!is_empty()) {
            return m_data[0];
        } else {
            return 0;
        }
    }

    inline size_t size() const {
        return size(bv());
    }

    inline void destroy_vals(size_t quot_width) {
        if (!is_empty()) {
            size_t size = this->size();

            val_t* start = ptrs(quot_width).vals_ptr;
            val_t* end = start + size;

            for(; start != end; start++) {
                start->~val_t();
            }
        }
    }

    inline Bucket(Bucket&& other) = default;
    inline Bucket& operator=(Bucket&& other) = default;

    inline BucketElem<val_t> at(size_t pos, size_t quot_width) const {
        if(!is_empty()) {
            auto ps = ptrs(quot_width);
            return BucketElem<val_t>(ps.vals_ptr + pos, ps.quots_ptr + pos);
        } else {
            DCHECK_EQ(pos, 0);
            return BucketElem<val_t>(nullptr, make_quot_ptr(nullptr, quot_width));
        }
    }
};


template<typename val_t>
inline void insert_in_bucket(Bucket<val_t>& bucket,
                                      size_t new_elem_bucket_pos,
                                      uint64_t new_elem_bv_bit,
                                      size_t qw,
                                      val_t&& val,
                                      uint64_t quot)
{
    static_assert(sizeof(Bucket<val_t>) == sizeof(void*), "unique_ptr is more than 1 ptr large!");

    size_t const size = bucket.size();

    // TODO: check out different sizing strategies
    // eg, the known sparse_hash repo uses overallocation for small buckets

    // create a new bucket with enough size for the new element
    auto new_bucket = Bucket<val_t>(bucket.bv() | new_elem_bv_bit, qw);

    // move all elements before the new element's location from old bucket into new bucket
    for(size_t i = 0; i < new_elem_bucket_pos; i++) {
        // TODO: Iterate pointers instead?
        auto new_elem = new_bucket.at(i, qw);
        auto old_elem = bucket.at(i, qw);

        new_elem.set_quotient(old_elem.get_quotient());
        new(&new_elem.val()) val_t(std::move(old_elem.val()));
    }

    // move new element into its location in the new bucket
    {
        auto new_elem = new_bucket.at(new_elem_bucket_pos, qw);
        new_elem.set_quotient(quot);
        new(&new_elem.val()) val_t(std::move(val));
    }

    // move all elements after the new element's location from old bucket into new bucket
    for(size_t i = new_elem_bucket_pos; i < size; i++) {
        // TODO: Iterate pointers instead?
        auto new_elem = new_bucket.at(i + 1, qw);
        auto old_elem = bucket.at(i, qw);

        new_elem.set_quotient(old_elem.get_quotient());
        new(&new_elem.val()) val_t(std::move(old_elem.val()));
    }

    // destroy old empty elements
    bucket.destroy_vals(qw);

    bucket = std::move(new_bucket);
}

template<typename val_t>
class BucketElem {
    val_t* m_val_ptr;
    QuotPtr m_quot_ptr;

    friend class Bucket<val_t>;

    inline BucketElem(val_t* val_ptr,
                      QuotPtr quot_ptr):
        m_val_ptr(val_ptr),
        m_quot_ptr(quot_ptr)
    {
    }

public:
    inline BucketElem():
        m_val_ptr(nullptr), m_quot_ptr() {}

    inline uint64_t get_quotient() {
        return uint64_t(*m_quot_ptr);
    }

    inline void set_quotient(uint64_t v) {
        *m_quot_ptr = v;
    }

    inline void swap_quotient(uint64_t& other) {
        uint64_t tmp = uint64_t(*m_quot_ptr);
        std::swap(other, tmp);
        *m_quot_ptr = tmp;
    }

    inline val_t& val() {
        return *m_val_ptr;
    }

    inline val_t const& val() const {
        return *m_val_ptr;
    }

    inline void increment_ptr() {
        m_quot_ptr++;
        m_val_ptr++;
    }
    inline void decrement_ptr() {
        m_quot_ptr--;
        m_val_ptr--;
    }

    inline bool ptr_eq(BucketElem const& other) {
        return m_val_ptr == other.m_val_ptr;
    }
};

template<typename val_t>
class compact_hash {
    using key_t = uint64_t;
    using buckets_t = std::vector<Bucket<val_t>>;

    static constexpr size_t BVS_WIDTH_SHIFT = 6;
    static constexpr size_t BVS_WIDTH_MASK = 0b111111;
    static constexpr size_t DEFAULT_KEY_WIDTH = 16;
    static constexpr bool HIGH_BITS_RANDOM = false;

    SizeManager m_sizing;
    uint8_t m_width;

    // Compact table data
    IntVector<uint_t<2>> m_cv;

    // Sparse table data
    buckets_t m_buckets;

    inline static constexpr size_t min_size(size_t size) {
        return (size < 2) ? 2 : size;
    }

public:

    inline compact_hash(size_t size, size_t key_width = DEFAULT_KEY_WIDTH):
        m_sizing(min_size(size)),
        m_width(key_width)
    {
        size_t cv_size = table_size();
        size_t buckets_size = ((table_size() + BVS_WIDTH_MASK) >> BVS_WIDTH_SHIFT);

        // std::cout << "cv_size: " << cv_size << ", buckets_size: " << buckets_size << "\n";

        m_cv.reserve(cv_size);
        m_cv.resize(cv_size);
        m_buckets.reserve(buckets_size);
        m_buckets.resize(buckets_size);
    }

    inline ~compact_hash() {
        destroy_buckets();
    }

    inline compact_hash(compact_hash&& other):
        m_sizing(std::move(other.m_sizing)),
        m_width(std::move(other.m_width)),
        m_cv(std::move(other.m_cv)),
        m_buckets(std::move(other.m_buckets))
    {
    }

    inline compact_hash& operator=(compact_hash&& other) {
        destroy_buckets();

        m_sizing = std::move(other.m_sizing);
        m_width = std::move(other.m_width);
        m_cv = std::move(other.m_cv);
        m_buckets = std::move(other.m_buckets);

        return *this;
    }

private:
    inline uint8_t table_size_log2() {
        return m_sizing.capacity_log2();
    }

    inline size_t table_size() {
        return m_sizing.capacity();
    }

    inline bool get_v(size_t pos) {
        return (m_cv.at(pos) & 0b01) != 0;
    }

    inline bool get_c(size_t pos) {
        return (m_cv.at(pos) & 0b10) != 0;
    }

    inline void set_v(size_t pos, bool v) {
        auto x = m_cv.at(pos) & 0b10;
        m_cv.at(pos) = x | (0b01 * v);
    }

    inline void set_c(size_t pos, bool c) {
        auto x = m_cv.at(pos) & 0b01;
        m_cv.at(pos) = x | (0b10 * c);
    }

    inline void sparse_drop_bucket(size_t i) {
        size_t qw = quotient_width();
        m_buckets.at(i).destroy_vals(qw);
        m_buckets.at(i) = Bucket<val_t>();
    }

    inline void destroy_buckets() {
        size_t qw = quotient_width();
        for(size_t i = 0; i < m_buckets.size(); i++) {
            if (!m_buckets[i].is_empty()) {
                m_buckets[i].destroy_vals(qw);
            }
        }
    }

    inline void dcheck_key_width(uint64_t key) {
        IF_DEBUG({
            uint64_t key_mask = (1ull << (m_width - 1ull) << 1ull) - 1ull;
            bool key_is_too_large = key & ~key_mask;
            DCHECK(!key_is_too_large) << "Attempt to insert key " << key << ", which requires more bits than the current set maximum of " << m_width << " Bits";
        });
    }

    // he actual amount of bits usable for storing a key
    // is always >= the set key width stored in m_width
    inline uint8_t real_width() {
        return std::max(uint8_t(table_size_log2() + 1), m_width);
    }

    struct DecomposedKey {
        size_t initial_address;   // initial address of key in table
        size_t stored_quotient;   // quotient value stored in table
    };

    inline DecomposedKey decompose_key(uint64_t key) {
        dcheck_key_width(key);
        uint64_t hres = compact_hashfn(key, real_width());

        IF_DEBUG({
            auto reverted = compact_reverse_hashfn(hres, real_width());
            DCHECK_EQ(reverted, key);
        });

        uint64_t shift = table_size_log2();

        if (HIGH_BITS_RANDOM) {
            shift = 64 - shift;
            return DecomposedKey {
                hres >> shift,
                hres & ((1ull << shift) - 1ull),
            };
        } else {
            return DecomposedKey {
                hres & ((1ull << shift) - 1ull),
                hres >> shift,
            };
        }
    }

    inline uint64_t compose_key(uint64_t initial_address, uint64_t quotient) {
        uint64_t shift = table_size_log2();

        uint64_t harg;
        if (HIGH_BITS_RANDOM) {
            shift = 64 - shift;
            harg = (initial_address << shift) | quotient;
        } else {
            harg = (quotient << shift) | initial_address;
        }

        uint64_t key = compact_reverse_hashfn(harg, real_width());
        dcheck_key_width(key);
        return key;
    }

    template<typename handler_t>
    inline void shift_insert_handler(size_t from,
                                     size_t to,
                                     uint64_t quot,
                                     handler_t&& handler) {
        DCHECK_NE(from, to);

        for(size_t i = to; i != from;) {
            size_t next_i = mod_sub(i, size_t(1));

            set_c(i, get_c(next_i));

            i = next_i;
        }
        shift_insert_sparse_handler(from, to, quot, std::move(handler));

        set_c(from, false);
    }

    struct SearchedGroup {
        size_t group_start;       // group that belongs to the key
        size_t group_end;         // it's a half-open range: [start .. end)
        size_t groups_terminator; // next free location
    };

    // assumption: there exists a group at the location indicated by key
    // this group is either the group belonging to key,
    // or the one after it in the case that no group for key exists yet
    inline SearchedGroup search_existing_group(DecomposedKey const& key) {
        auto ret = SearchedGroup();

        // walk forward from the initial address until we find a empty location
        // TODO: This search could maybe be accelerated by:
        // - checking whole blocks in the bucket bitvector for == or != 0
        size_t cursor = key.initial_address;
        size_t v_counter = 0;
        DCHECK_EQ(get_v(cursor), true);

        for(; sparse_exists(cursor); cursor = mod_add(cursor)) {
            v_counter += get_v(cursor);
        }
        DCHECK_GE(v_counter, 1);
        ret.groups_terminator = cursor;

        // walk back again to find start of group belong to the initial address
        size_t c_counter = v_counter;
        for(; c_counter != 1; cursor = mod_sub(cursor)) {
            c_counter -= get_c(mod_sub(cursor));
        }

        ret.group_end = cursor;

        for(; c_counter != 0; cursor = mod_sub(cursor)) {
            c_counter -= get_c(mod_sub(cursor));
        }

        ret.group_start = cursor;

        return ret;
    }

    inline val_t* search(SearchedGroup const& res, uint64_t stored_quotient) {
        for(size_t i = res.group_start; i != res.group_end; i = mod_add(i)) {
            auto sparse_entry = sparse_get_at(i);

            if (sparse_entry.get_quotient() == stored_quotient) {
                return &sparse_entry.val();
            }
        }
        return nullptr;
    }

    inline val_t* search(uint64_t key) {
        auto dkey = decompose_key(key);
        if (get_v(dkey.initial_address)) {
            return search(search_existing_group(dkey), dkey.stored_quotient);
        }
        return nullptr;
    }

public:

    // TODO: This is not a std interface
    inline void insert(uint64_t key, val_t&& value) {
        insert(key, std::move(value), m_width);
    }
    inline void insert(uint64_t key, val_t&& value, size_t key_width) {
        insert_handler(key, key_width, InsertHandler {
            std::move(value)
        });
    }

    inline val_t& operator[](uint64_t key) {
        return index(key, m_width);
    }

    inline val_t& index(uint64_t key, size_t key_width) {
        val_t* addr = nullptr;

        insert_handler(key, key_width, AddressDefaultHandler {
            &addr
        });

        DCHECK(addr != nullptr);

        return *addr;
    }

    inline size_t size() const {
        return m_sizing.size();
    }

private:
    // Handler for inserting an element that exists as a rvalue reference.
    // This will overwrite an existing element.
    class InsertHandler {
        val_t&& m_value;
    public:
        InsertHandler(val_t&& value): m_value(std::move(value)) {}

        inline auto on_new() {
            struct InsertHandlerOnNew {
                val_t&& m_value;
                inline val_t& get() {
                    return m_value;
                }
                inline void new_location(val_t& value) {
                }
            };

            return InsertHandlerOnNew {
                std::move(m_value),
            };
        }

        inline void on_existing(val_t& value) {
            m_value = std::move(value);
        }
    };

    // Handler for getting the address of an element in the map.
    // If none exists yet, it will be default constructed.
    class AddressDefaultHandler {
        val_t** m_address = nullptr;
    public:
        AddressDefaultHandler(val_t** address): m_address(address) {}

        inline auto on_new() {
            struct AddressDefaultHandlerOnNew {
                val_t m_value;
                val_t** m_address;
                inline val_t& get() {
                    return m_value;
                }
                inline void new_location(val_t& value) {
                    *m_address = &value;
                }
            };

            return AddressDefaultHandlerOnNew {
                val_t(),
                m_address,
            };
        }

        inline void on_existing(val_t& value) {
            *m_address = &value;
        }
    };

    template<typename handler_t>
    inline void insert_after(SearchedGroup const& res,
                             DecomposedKey const& dkey,
                             handler_t&& handler)
    {
        // this will insert the value at the end of the range defined by res

        if (sparse_is_empty(res.group_end)) {
            // if there is no following group, just append the new entry

            sparse_set_at_empty_handler(res.group_end,
                                        dkey.stored_quotient,
                                        std::move(handler));
        } else {
            // else, shift all following elements

            shift_insert_handler(res.group_end,
                                 res.groups_terminator,
                                 dkey.stored_quotient,
                                 std::move(handler));
        }
    }

    template<typename handler_t>
    inline void insert_handler(uint64_t key, size_t key_width, handler_t&& handler) {
        grow_if_needed(key_width);
        auto const dkey = decompose_key(key);

        DCHECK_EQ(key, compose_key(dkey.initial_address, dkey.stored_quotient));

        // cases:
        // - initial address empty
        // - initial address full, v[initial address] = 1
        // - initial address full, v[initial address] = 0

        DCHECK_LT(dkey.initial_address >> BVS_WIDTH_SHIFT, m_buckets.size());

        if (sparse_is_empty(dkey.initial_address)) {
            // check if we can insert directly

            sparse_set_at_empty_handler(dkey.initial_address,
                                        dkey.stored_quotient,
                                        std::move(handler));

            // we created a new group, so update the bitflags
            set_v(dkey.initial_address, true);
            set_c(dkey.initial_address, true);
            m_sizing.size()++;
        } else {
            // check if there already is a group for this key
            bool const group_exists = get_v(dkey.initial_address);

            if (group_exists) {
                auto const res = search_existing_group(dkey);

                // check if element already exists
                auto p = search(res, dkey.stored_quotient);
                if (p != nullptr) {
                    handler.on_existing(*p);
                } else {
                    insert_after(res, dkey, std::move(handler));
                    m_sizing.size()++;
                }
            } else {
                // insert a new group

                // pretend we already inserted the new group
                // this makes insert_after() find the group
                // at the location _before_ the new group
                set_v(dkey.initial_address, true);
                auto const res = search_existing_group(dkey);

                // insert the element after the found group
                insert_after(res, dkey, std::move(handler));

                // mark the inserted element as the start of a new group,
                // thus fixing-up the v <-> c mapping
                set_c(res.group_end, true);

                m_sizing.size()++;
            }
        }
    }

    template<typename int_t>
    inline int_t mod_add(int_t v, int_t add = 1) {
        size_t mask = table_size() - 1;
        return (v + add) & mask;
    }
    template<typename int_t>
    inline int_t mod_sub(int_t v, int_t sub = 1) {
        size_t mask = table_size() - 1;
        return (v - sub) & mask;
    }

    struct iter_all_t {
        compact_hash<val_t>& m_self;
        size_t i = 0;
        size_t original_start = 0;
        uint64_t initial_address = 0;
        enum {
            EMPTY_LOCATIONS,
            FULL_LOCATIONS
        } state = EMPTY_LOCATIONS;

        inline iter_all_t(compact_hash<val_t>& self): m_self(self) {
            // first, skip forward to the first empty location
            // so that iteration can start at the beginning of the first complete group

            i = 0;

            for(;;i++) {
                if (m_self.sparse_is_empty(i)) {
                    break;
                }
            }

            // Remember our startpoint so that we can recognize it when
            // we wrapped around back to it
            original_start = i;

            // We proceed to the next position so that we can iterate until
            // we reach `original_start` again.
            i = m_self.mod_add(i);
        }

        inline bool next(uint64_t* out_initial_address, size_t* out_i) {
            while(true) {
                if (state == EMPTY_LOCATIONS) {
                    // skip empty locations
                    for(;;i = m_self.mod_add(i)) {
                        if (m_self.sparse_exists(i)) {
                            // we initialize init-addr at 1 pos before the start of
                            // a group of blocks, so that the blocks iteration logic works
                            initial_address = m_self.mod_sub(i);
                            state = FULL_LOCATIONS;
                            break;
                        }
                        if (i == original_start) {
                            return false;
                        }
                    }
                } else {
                    // process full locations
                    if (m_self.sparse_is_empty(i))  {
                        state = EMPTY_LOCATIONS;
                        continue;
                    }
                    if (m_self.get_c(i)) {
                        // skip forward m_v cursor
                        // to find initial address for this block
                        //
                        // this works for the first block because
                        // initial_address starts at 1 before the group
                        initial_address = m_self.mod_add(initial_address);

                        while(!m_self.get_v(initial_address)) {
                            initial_address = m_self.mod_add(initial_address);
                        }
                    }

                    *out_initial_address = initial_address;
                    *out_i = i;

                    i = m_self.mod_add(i);
                    return true;
                }
            }
        }
    };

    inline size_t quotient_width() {
        return real_width() - table_size_log2();
    }

    inline void grow_if_needed(size_t new_width) {
        auto needs_capacity_change = [&]() {
            return(m_sizing.capacity() / 2) <= (m_sizing.size() + 1);
        };

        auto needs_realloc = [&]() {
            return needs_capacity_change() || (new_width != m_width);
        };

        /*
        std::cout
                << "buckets size/cap: " << m_buckets.size()
                << ", size: " << m_sizing.size()
                << "\n";
        */

        if (needs_realloc()) {
            size_t new_capacity;
            if (needs_capacity_change()) {
                new_capacity = m_sizing.capacity() * 2;
            } else {
                new_capacity = m_sizing.capacity();
            }
            auto new_table = compact_hash(new_capacity, new_width);

            /*
            std::cout
                << "grow to cap " << new_table.table_size()
                << ", m_width: " << int(new_table.m_width)
                << ", real_width: " << int(new_table.real_width())
                << ", quot width: " << new_table.quotient_width()
                << "\n";
            */

            bool start_of_bucket = false;
            size_t bucket = 0;

            uint64_t initial_address;
            size_t i;
            auto iter = iter_all_t(*this);
            while(iter.next(&initial_address, &i)) {
                auto p = sparse_pos(i);

                // drop buckets of old table as they get emptied out
                if (p.offset_in_bucket() == 0) {
                    if (start_of_bucket) {
                        DCHECK_NE(bucket, p.bucket_pos);
                        sparse_drop_bucket(bucket);
                    }

                    start_of_bucket = true;
                    bucket = p.bucket_pos;
                }

                auto kv = sparse_get_at(p);
                auto stored_quotient = kv.get_quotient();
                auto& val = kv.val();
                key_t key = compose_key(initial_address, stored_quotient);

                new_table.insert(key, std::move(val));
            }

            *this = std::move(new_table);
        }

        DCHECK(!needs_realloc());
    }

public:
    // for tests:

    inline std::string debug_state() {
        std::stringstream ss;

        bool gap_active = false;
        size_t gap_start;
        size_t gap_end;

        auto print_gap = [&](){
            if (gap_active) {
                gap_active = false;
                ss << "    [" << gap_start << " to " << gap_end << " empty]\n";
            }
        };

        auto add_gap = [&](size_t i){
            if (!gap_active) {
                gap_active = true;
                gap_start = i;
            }
            gap_end = i;
        };

        std::vector<std::string> lines(table_size());

        uint64_t initial_address;
        size_t j;
        auto iter = iter_all_t(*this);
        while(iter.next(&initial_address, &j)) {
            std::stringstream ss2;

            auto kv = sparse_get_at(j);
            auto stored_quotient = kv.get_quotient();
            auto& val = kv.val();
            key_t key = compose_key(initial_address, stored_quotient);

            ss2 << j
                << "\t: v = " << get_v(j)
                << ", c = " << get_c(j)
                << ", quot = " << stored_quotient
                << ", iadr = " << initial_address
                << "\t, key = " << key
                << "\t, value = " << val
                << "\t (" << &val << ")";

            lines.at(j) = ss2.str();
        }

        ss << "[\n";
        for (size_t i = 0; i < table_size(); i++) {
            bool cv_exist = lines.at(i) != "";

            DCHECK_EQ(cv_exist, sparse_exists(i));

            if (cv_exist) {
                print_gap();
                ss << "    "
                    << lines.at(i)
                    << "\n";
            } else {
                add_gap(i);
            }
        }
        print_gap();
        ss << "]";

        return ss.str();
    }

    inline void debug_check_single(uint64_t key, val_t const& val) {
        auto ptr = search(key);
        ASSERT_NE(ptr, nullptr) << "key " << key << " not found!";
        if (ptr != nullptr) {
            ASSERT_EQ(*ptr, val) << "value is " << *ptr << " instead of " << val;
        }
    }

    inline void debug_print() {
        std::cout << "m_width: " << uint32_t(m_width) << "\n",
        std::cout << "size: " << size() << "\n",
        std::cout << debug_state() << "\n";
    }

private:
    template<typename handler_t>
    inline void sparse_set_at_empty_handler(size_t pos, key_t quot, handler_t&& handler) {
        auto data = sparse_pos(pos);
        DCHECK(!data.exists_in_bucket());

        // TODO: Check that the sparse_pos to a invalid position works correctly

        // figure out which bucket to access
        auto& bucket = m_buckets[data.bucket_pos];
        size_t const qw = quotient_width();

        // we will insert a new element
        auto value_handler = handler.on_new();
        auto& val = value_handler.get();

        // insert element & grow bucket as appropriate
        insert_in_bucket(bucket, data.offset_in_bucket(), data.b_mask, qw, std::move(val), quot);

        // notify handler with location of new element
        auto new_loc = bucket.at(data.offset_in_bucket(), qw);
        value_handler.new_location(new_loc.val());
    }

    inline bool sparse_is_empty(size_t i) {
        return !sparse_exists(i);
    }

    inline bool sparse_exists(size_t pos) {
        return sparse_pos(pos).exists_in_bucket();
    }

    // shifts all elements one to the right,
    // inserts val and quot at the from position,
    // and stores the old from element in val and quot.
    inline void sparse_shift(size_t from, size_t to, val_t& val, key_t& quot) {
        // pseudo-iterator for iterating over bucket elements
        // NB: does not wrap around!
        struct iter {
            Bucket<val_t> const* m_bucket;
            BucketElem<val_t>    m_b_start;
            BucketElem<val_t>    m_b_end;
            size_t               m_quotient_width;

            inline void set_bucket_elem_range(size_t end_offset) {
                size_t start_offset = 0;
                DCHECK_LE(start_offset, end_offset);

                m_b_start = m_bucket->at(start_offset, m_quotient_width);
                m_b_end   = m_bucket->at(end_offset, m_quotient_width);
            }

            inline iter(compact_hash& table,
                        SparsePos const& pos) {
                m_quotient_width = table.quotient_width();

                // NB: Using pointer arithmetic here, because
                // we can (intentionally) end up with the address 1-past
                // the end of the vector, which represents a end-iterator
                m_bucket = table.m_buckets.data() + pos.bucket_pos;

                if(pos.bucket_pos < table.m_buckets.size()) {
                    set_bucket_elem_range(pos.offset_in_bucket());
                } else {
                    // use default constructed nullptr BucketElems
                }
            }

            inline BucketElem<val_t> get() {
                return m_b_end;
            }

            inline void decrement() {
                if (!m_b_start.ptr_eq(m_b_end)) {
                    m_b_end.decrement_ptr();
                } else {
                    do {
                        --m_bucket;
                    } while(m_bucket->bv() == 0);
                    set_bucket_elem_range(m_bucket->size() - 1);
                }
            }

            inline bool operator!=(iter& other) {
                return !(m_b_end.ptr_eq(other.m_b_end));
            }
        };

        DCHECK_LT(from, to);

        // initialize iterators like this:
        // [         ]
        // ^from   to^
        //          ||
        //    <- src^|
        //    <- dest^

        auto from_loc = sparse_pos(from);
        auto from_iter = iter(*this, from_loc);

        auto last = sparse_pos(to - 1);
        auto src = iter(*this, last);
        auto dst = iter(*this, sparse_pos(to));

        // move the element at the last position to a temporary position
        auto  tmp_p    = sparse_get_at(last);
        val_t tmp_val  = std::move(tmp_p.val());
        key_t tmp_quot = tmp_p.get_quotient();

        // move all elements one to the right
        while(src != from_iter) {
            // Decrement first for backward iteration
            src.decrement();
            dst.decrement();

            // Get access to the value/quotient at src and dst
            auto src_be = src.get();
            auto dst_be = dst.get();

            // Copy value/quotient over
            dst_be.val() = std::move(src_be.val());
            dst_be.set_quotient(src_be.get_quotient());
        }

        // move new element into empty from position
        auto from_p = sparse_get_at(from_loc);
        from_p.val() = std::move(val);
        from_p.set_quotient(quot);

        // move temporary element into the parameters
        val = std::move(tmp_val);
        quot = tmp_quot;
    }

    template<typename handler_t>
    inline void shift_insert_sparse_handler(size_t from,
                                            size_t to,
                                            key_t quot,
                                            handler_t&& handler) {
        // move from...to one to the right, then insert at from

        DCHECK(from != to);

        auto value_handler = handler.on_new();
        auto& val = value_handler.get();

        if (to < from) {
            // if the range wraps around, we decompose into two ranges:
            // [   |      |      ]
            // | to^      ^from  |
            // ^start         end^
            // [ 2 ]      [  1   ]
            //
            // NB: because we require from != to, and insert 1 additional element,
            // we are always dealing with a minimum 2 element range,
            // and thus can not end up with a split range with length == 0

            // inserts the new element at the start of the range,
            // and temporarily stores the element at the end of the range
            // in `val` and `quot`
            sparse_shift(from,  table_size(), val, quot);
            sparse_shift(0, to, val, quot);
        } else {
            // inserts the new element at the start of the range,
            // and temporarily stores the element at the end of the range
            // in `val` and `quot`
            sparse_shift(from, to, val, quot);
        }

        // insert the element from the end of the range at the free
        // position to the right of it
        auto insert = InsertHandler(std::move(val));
        sparse_set_at_empty_handler(to, quot, std::move(insert));

        // after the previous insert and a potential reallocation,
        // notify the handler about the address of the new value
        auto new_loc = sparse_pos(from);
        value_handler.new_location(sparse_get_at(new_loc).val());
    }

    struct SparsePos {
    private:
        buckets_t* m_buckets;
    public:
        size_t const bucket_pos;
        size_t const bit_pos;
        uint64_t const b_mask;

        inline SparsePos() {}

        inline SparsePos(size_t pos, buckets_t& buckets):
            m_buckets(&buckets),

            // bucket index based on position (division by 64 bits)
            bucket_pos(pos >> BVS_WIDTH_SHIFT),

            // remainder position of pos inside the bucket (modulo by 64 bits)
            bit_pos(pos & BVS_WIDTH_MASK),

            // mask for the single bit we deal with
            b_mask(1ull << bit_pos)
        {}

        // check if the right bit is set in the bucket's bv
        inline bool exists_in_bucket() const {
            // bitvector of the bucket
            uint64_t bv = m_buckets->at(bucket_pos).bv();

            return (bv & b_mask) != 0;
        }

        // calculate offset of element in bucket for current pos
        // based on number of set bits in bv
        inline size_t offset_in_bucket() const {
            // bitvector of the bucket
            uint64_t bv = m_buckets->at(bucket_pos).bv();

            return __builtin_popcountll(bv & (b_mask - 1));
        }
    };

    SparsePos sparse_pos(size_t pos) {
        return SparsePos { pos, m_buckets };
    }

    inline BucketElem<val_t> sparse_get_at(SparsePos pos) {
        DCHECK(pos.exists_in_bucket());
        size_t qw = quotient_width();

        return m_buckets[pos.bucket_pos].at(pos.offset_in_bucket(), qw);
    }

    inline BucketElem<val_t> sparse_get_at(size_t pos) {
        return sparse_get_at(sparse_pos(pos));
    }
};

}
