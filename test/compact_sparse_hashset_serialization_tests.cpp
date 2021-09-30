#include <gtest/gtest.h>

#include <cstdint>
#include <algorithm>

#include <tudocomp/util/compact_hash/set/hashset_t.hpp>
#include <tudocomp/util/compact_hash/map/hashmap_t.hpp>

#include <tudocomp/util/compact_hash/index_structure/displacement_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/elias_gamma_displacement_table_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/layered_displacement_table_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/naive_displacement_table_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/cv_bvs_t.hpp>
#include <tudocomp/util/compact_hash/hash_functions.hpp>
#include <tudocomp/util/compact_hash/storage/buckets_bv_t.hpp>
#include <tudocomp/util/compact_hash/storage/plain_sentinel_t.hpp>
#include <tudocomp/util/serialization.hpp>

using namespace tdc::compact_hash;
using namespace tdc::compact_hash::set;
using namespace tdc::compact_hash::map;

template<typename table_t, typename build_func>
void serialize_test_builder(build_func f) {
    using tdc::serialize;
    using tdc::heap_size;
    auto a = f();

    std::stringstream ss;
    auto bytes = serialize<table_t>::write(ss, a);
    size_t stream_bytes = ss.tellp();
    ASSERT_EQ(bytes.size_in_bytes(), stream_bytes);

    auto b = serialize<table_t>::read(ss);

    ASSERT_TRUE(serialize<table_t>::equal_check(a, b));

    auto c = f();

    ASSERT_TRUE(serialize<table_t>::equal_check(a, c));
    ASSERT_TRUE(serialize<table_t>::equal_check(b, c));

    std::cout << "heap size: "
        << heap_size<table_t>::compute(a)
        << ", written bytes: "
        << bytes
        << "\n";
}


template<typename table_t>
void serialize_test_set() {
    serialize_test_builder<table_t>([] {
        auto ch = table_t(8, 16);
        ch.max_load_factor(1.0);
        ch.lookup_insert(3);
        ch.lookup_insert(3 + 8);
        ch.lookup_insert(5);
        ch.lookup_insert(5 + 8);
        ch.lookup_insert(5 + 16);
        ch.lookup_insert(5 + 24);
        ch.lookup_insert(4);
        return ch;
    });
    serialize_test_builder<table_t>([] {
        auto ch = table_t(8, 16);
        ch.max_load_factor(1.0);
        ch.lookup_insert(3);
        ch.lookup_insert(3 + 8);
        ch.lookup_insert(5);
        ch.lookup_insert(5 + 8);
        ch.lookup_insert(5 + 16);
        ch.lookup_insert(5 + 24);
        ch.lookup_insert(4);
        return ch;
    });

    serialize_test_builder<table_t>([] {
        auto ch = table_t(0, 10);

        auto add = [&](auto key) {
            ch.lookup_insert(key);
        };

        for(size_t i = 0; i < 1000; i++) {
            add(i);
        }

        return ch;
    });

    serialize_test_builder<table_t>([] {
        auto ch = table_t(0, 10);

        uint8_t bits = 1;

        auto add = [&](auto key) {
            bits = std::max(bits, tdc::bits_for(key));

            ch.lookup_insert_key_width(key, bits);

        };

        for(size_t i = 0; i < 1000; i++) {
            add(i);
        }

        return ch;
    });

    serialize_test_builder<table_t>([] {
        auto ch = table_t(0, 0);

        uint8_t bits = 1;

        auto add = [&](auto key) {
            bits = std::max(bits, tdc::bits_for(key));
            ch.lookup_insert_key_width(key, bits);
        };


        for(size_t i = 0; i < 10000; i++) {
            add(i*13ull);
        }

        return ch;
    });
}

#define gen_test_set(name, ...)        \
TEST(serialize, name) {            \
    serialize_test_set<__VA_ARGS__>(); \
}

gen_test_set(set_poplar_displacement_compact_fixed_4,
    hashset_t<
        poplar_xorshift_t,
        displacement_t<
            layered_displacement_table_t<static_layered_bit_width_t<4>>
        >
    >
)

gen_test_set(set_poplar_displacement_compact_dynamic,
    hashset_t<
        poplar_xorshift_t,
        displacement_t<
            layered_displacement_table_t<dynamic_layered_bit_width_t>
        >
    >
)

gen_test_set(set_poplar_cv,
    hashset_t<
        poplar_xorshift_t,
        cv_bvs_t
    >
)

gen_test_set(set_poplar_displacement_elias_fixed_1024,
    hashset_t<
        poplar_xorshift_t,
        displacement_t<
            elias_gamma_displacement_table_t<
                fixed_elias_gamma_bucket_size_t<1024>
            >
        >
    >
)

gen_test_set(set_poplar_displacement_elias_growing,
    hashset_t<
        poplar_xorshift_t,
        displacement_t<
            elias_gamma_displacement_table_t<
                growing_elias_gamma_bucket_size_t
            >
        >
    >
)

gen_test_set(set_poplar_displacement_elias_dynamic,
    hashset_t<
        poplar_xorshift_t,
        displacement_t<
            elias_gamma_displacement_table_t<
                dynamic_fixed_elias_gamma_bucket_size_t
            >
        >
    >
)

template<typename table_t>
void serialize_test_map() {
    serialize_test_builder<table_t>([] {
        auto ch = table_t(8, 16);
        ch.max_load_factor(1.0);
        ch.insert(3, 42);
        ch.insert(3 + 8, 43);
        ch.insert(5, 44);
        ch.insert(5 + 8, 45);
        ch.insert(5 + 16, 46);
        ch.insert(5 + 24, 47);
        ch.insert(4, 48);
        return ch;
    });
    serialize_test_builder<table_t>([] {
        auto ch = table_t(8, 16);
        ch.max_load_factor(1.0);
        ch.insert(3, 49);
        ch.insert(3 + 8, 50);
        ch.insert(5, 51);
        ch.insert(5 + 8, 52);
        ch.insert(5 + 16, 53);
        ch.insert(5 + 24, 54);
        ch.insert(4, 55);
        return ch;
    });

    serialize_test_builder<table_t>([] {
        auto ch = table_t(0, 10);

        auto add = [&](auto key) {
            ch.insert(key, key * 3);
        };

        for(size_t i = 0; i < 1000; i++) {
            add(i);
        }

        return ch;
    });

    serialize_test_builder<table_t>([] {
        auto ch = table_t(0, 10);

        uint8_t bits = 1;

        auto add = [&](auto key) {
            bits = std::max(bits, tdc::bits_for(key));

            ch.insert_key_width(key, key * 4, bits);

        };

        for(size_t i = 0; i < 1000; i++) {
            add(i);
        }

        return ch;
    });

    serialize_test_builder<table_t>([] {
        auto ch = table_t(0, 0);

        uint8_t bits = 1;

        auto add = [&](auto key) {
            bits = std::max(bits, tdc::bits_for(key));
            ch.insert_key_width(key, key * 5, bits);
        };


        for(size_t i = 0; i < 10000; i++) {
            add(i*13ull);
        }

        return ch;
    });
}

#define gen_test_map(name, ...)        \
TEST(serialize, name) {            \
    serialize_test_map<__VA_ARGS__>(); \
}

using val_t = uint64_t;

gen_test_map(map_poplar_bbv_displacement_compact_fixed_4,
    hashmap_t<
        val_t,
        poplar_xorshift_t,
        buckets_bv_t,
        displacement_t<
            layered_displacement_table_t<static_layered_bit_width_t<4>>
        >
    >
)

gen_test_map(map_poplar_bbv_displacement_compact_dynamic,
    hashmap_t<
        val_t,
        poplar_xorshift_t,
        buckets_bv_t,
        displacement_t<
            layered_displacement_table_t<dynamic_layered_bit_width_t>
        >
    >
)

gen_test_map(map_poplar_bbv_cv,
    hashmap_t<
        val_t,
        poplar_xorshift_t,
        buckets_bv_t,
        cv_bvs_t
    >
)

gen_test_map(map_poplar_bbv_displacement_elias_fixed_1024,
    hashmap_t<
        val_t,
        poplar_xorshift_t,
        buckets_bv_t,
        displacement_t<
            elias_gamma_displacement_table_t<
                fixed_elias_gamma_bucket_size_t<1024>
            >
        >
    >
)

gen_test_map(map_poplar_bbv_displacement_elias_growing,
    hashmap_t<
        val_t,
        poplar_xorshift_t,
        buckets_bv_t,
        displacement_t<
            elias_gamma_displacement_table_t<
                growing_elias_gamma_bucket_size_t
            >
        >
    >
)

gen_test_map(map_poplar_bbv_displacement_elias_dynamic,
    hashmap_t<
        val_t,
        poplar_xorshift_t,
        buckets_bv_t,
        displacement_t<
            elias_gamma_displacement_table_t<
                dynamic_fixed_elias_gamma_bucket_size_t
            >
        >
    >
)

gen_test_map(map_poplar_ps_displacement_compact_fixed_4,
    hashmap_t<
        val_t,
        poplar_xorshift_t,
        plain_sentinel_t,
        displacement_t<
            layered_displacement_table_t<static_layered_bit_width_t<4>>
        >
    >
)

gen_test_map(map_poplar_ps_displacement_compact_dynamic,
    hashmap_t<
        val_t,
        poplar_xorshift_t,
        plain_sentinel_t,
        displacement_t<
            layered_displacement_table_t<dynamic_layered_bit_width_t>
        >
    >
)

gen_test_map(map_poplar_ps_cv,
    hashmap_t<
        val_t,
        poplar_xorshift_t,
        plain_sentinel_t,
        cv_bvs_t
    >
)

gen_test_map(map_poplar_ps_displacement_elias_fixed_1024,
    hashmap_t<
        val_t,
        poplar_xorshift_t,
        plain_sentinel_t,
        displacement_t<
            elias_gamma_displacement_table_t<
                fixed_elias_gamma_bucket_size_t<1024>
            >
        >
    >
)

gen_test_map(map_poplar_ps_displacement_elias_growing,
    hashmap_t<
        val_t,
        poplar_xorshift_t,
        plain_sentinel_t,
        displacement_t<
            elias_gamma_displacement_table_t<
                growing_elias_gamma_bucket_size_t
            >
        >
    >
)

gen_test_map(map_poplar_ps_displacement_elias_dynamic,
    hashmap_t<
        val_t,
        poplar_xorshift_t,
        plain_sentinel_t,
        displacement_t<
            elias_gamma_displacement_table_t<
                dynamic_fixed_elias_gamma_bucket_size_t
            >
        >
    >
)
