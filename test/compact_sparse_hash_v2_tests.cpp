#include <gtest/gtest.h>

#include <cstdint>
#include <algorithm>
#include <tudocomp/util/compact_hash/map/hashmap_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/cv_bvs_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/displacement_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/elias_gamma_displacement_table_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/layered_displacement_table_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/naive_displacement_table_t.hpp>
#include <tudocomp/util/compact_hash/storage/bucket_t.hpp>
#include <tudocomp/util/compact_hash/storage/buckets_bv_t.hpp>
#include <tudocomp/util/compact_hash/storage/plain_sentinel_t.hpp>
#include <tudocomp/util/compact_hash/hash_functions.hpp>
#include <tudocomp/util/bit_packed_layout_t.hpp>

using namespace tdc::compact_hash::map;
using namespace tdc::compact_hash;
using namespace tdc;

template<typename val_t>
using map_bucket_t = bucket_t<8, satellite_data_t<val_t>>;

template<typename val_t>
void BucketTest() {
    using widths_t = typename satellite_data_t<val_t>::entry_bit_width_t;

    auto b = map_bucket_t<val_t>();

    widths_t ws { 5, 7 };
    b = map_bucket_t<val_t>(0b10, ws);

    ASSERT_EQ(b.bv(), 2U);
    ASSERT_EQ(b.size(), 1U);
    ASSERT_EQ(b.is_empty(), false);

    auto p1 = b.at(0, ws);
    p1.set_no_drop(3, 4);

    b.stat_allocation_size_in_bytes(ws);

    auto p2 = b.insert_at(0, 0b11, ws);
    p2.set_no_drop(5, 6);

    p2.set(7, 8);

    b.destroy_vals(ws);
}

#define MakeBucketTest(tname) \
TEST(Bucket, tname##_test) {  \
    BucketTest<tname>();      \
}

using uint_t40 = uint_t<40>;
MakeBucketTest(uint8_t);
MakeBucketTest(uint64_t);
MakeBucketTest(dynamic_t);
MakeBucketTest(uint_t40);

template<template<typename> typename table_t, typename val_t>
void TableTest() {
    using tab_t = table_t<satellite_data_t<val_t>>;
    using widths_t = typename satellite_data_t<val_t>::entry_bit_width_t;

    {
        auto t = tab_t();

        widths_t ws { 5, 7 };
        size_t table_size = 16;
        t = tab_t(table_size, ws, {});
        auto ctx = t.context(table_size, ws);

        for(size_t i = 0; i < table_size; i++) {
            auto pos = ctx.table_pos(i);
            ASSERT_EQ(ctx.pos_is_empty(pos), true);

            auto elem = ctx.allocate_pos(pos);
            elem.set_no_drop(i + 1, i + 2);
        }

        for(size_t i = 0; i < table_size; i++) {
            auto pos = ctx.table_pos(i);
            ASSERT_EQ(ctx.pos_is_empty(pos), false);

            auto elem = ctx.at(pos);
            ASSERT_EQ(*elem.val_ptr(), i + 1);
            ASSERT_EQ(elem.get_quotient(), i + 2);
        }
    }

    {
        widths_t ws { 5, 7 };
        size_t table_size = 128;
        auto t = tab_t(table_size, ws, {});
        auto ctx = t.context(table_size, ws);

        for(size_t i = 60; i < 80; i++) {
            auto pos = ctx.table_pos(i);
            ASSERT_EQ(ctx.pos_is_empty(pos), true);

            auto elem = ctx.allocate_pos(pos);
            elem.set_no_drop(i - 60 + 1, i - 60 + 2);

            ASSERT_EQ(ctx.pos_is_empty(pos), false);
        }

        auto iter = ctx.make_iter(ctx.table_pos(80));
        (void) iter;
        for(size_t i = 0; i < 20; i++) {
            iter.decrement();
            auto elem = iter.get();

            ASSERT_EQ(*elem.val_ptr(), 20 - i);
            ASSERT_EQ(elem.get_quotient(), 20 - i + 1);
        }
    }

}

#define MakeTableTest(tab, tname)   \
TEST(Table, tab##_##tname##_test) { \
    TableTest<tab, tname>();        \
}

MakeTableTest(plain_sentinel_t, uint8_t);
MakeTableTest(buckets_bv_t,     uint8_t);
MakeTableTest(plain_sentinel_t, uint64_t);
MakeTableTest(buckets_bv_t,     uint64_t);
MakeTableTest(plain_sentinel_t, dynamic_t);
MakeTableTest(buckets_bv_t,     dynamic_t);
MakeTableTest(plain_sentinel_t, uint_t40);
MakeTableTest(buckets_bv_t,     uint_t40);

template<typename placement_t, template<typename> typename table_t, typename val_t>
void CVTableTest() {
    using tab_t = table_t<satellite_data_t<val_t>>;
    using widths_t = typename satellite_data_t<val_t>::entry_bit_width_t;
    using value_type = typename cbp::cbp_repr_t<val_t>::value_type;

    widths_t ws { 5, 7 };
    auto size_mgr = size_manager_t(128);

    auto t = tab_t(size_mgr.capacity(), ws, {});
    auto p = placement_t(size_mgr.capacity(), {});

    auto tctx = t.context(size_mgr.capacity(), ws);
    auto pctx = p.context(t, size_mgr.capacity(), ws, size_mgr);

    auto check_insert = [&](auto ia, auto value, auto sq, bool should_exists) {
        auto res = pctx.lookup_insert(ia, sq);
        ASSERT_EQ(res.key_already_exist(), should_exists);
        ASSERT_EQ(res.ptr().get_quotient(), sq);
        *res.ptr().val_ptr() = value;
    };
    auto table_state = [&](std::vector<std::array<uint64_t, 3>> const& should) {
        std::vector<std::array<uint64_t, 3>> r;
        for (size_t i = 0; i < size_mgr.capacity(); i++) {
            auto tpos = tctx.table_pos(i);
            if (!tctx.pos_is_empty(tpos)) {
                // TODO: Replace with search()
                auto ptr = tctx.at(tpos);
                r.push_back(std::array<uint64_t, 3>{
                    i, value_type(*ptr.val_ptr()), ptr.get_quotient()
                });
            }
        }
        auto is = r;
        ASSERT_EQ(is, should);
    };

    check_insert(60, 1, 5U, false);
    table_state({
        {60, 1, 5},
    });

    check_insert(66, 2, 5U, false);
    table_state({
        {60, 1, 5},
        {66, 2, 5},
    });

    check_insert(64, 3, 5U, false);
    table_state({
        {60, 1, 5},
        {64, 3, 5},
        {66, 2, 5},
    });

    check_insert(62, 4, 5U, false);
    table_state({
        {60, 1, 5},
        {62, 4, 5},
        {64, 3, 5},
        {66, 2, 5},
    });

    check_insert(62, 5, 6U, false);
    table_state({
        {60, 1, 5},
        {62, 4, 5},
        {63, 5, 6},
        {64, 3, 5},
        {66, 2, 5},
    });

    check_insert(62, 10, 6U, true);
    table_state({
        {60, 1, 5},
        {62, 4, 5},
        {63, 10, 6},
        {64, 3, 5},
        {66, 2, 5},
    });

    check_insert(62, 9, 7U, false);
    table_state({
        {60, 1, 5},
        {62, 4, 5},
        {63, 10, 6},
        {64, 9, 7},
        {65, 3, 5},
        {66, 2, 5},
    });

    /*
     Test:
     - multiple independ inserts
     - appends to same group
     - appends to displaced group

     */
}

#define MakeCVTableTest(place, tab, tname)      \
TEST(CVTable, place##_##tab##_##tname##_test) { \
    CVTableTest<place, tab, tname>();           \
}

MakeCVTableTest(cv_bvs_t, plain_sentinel_t, uint8_t);
MakeCVTableTest(cv_bvs_t, plain_sentinel_t, uint64_t);
MakeCVTableTest(cv_bvs_t, plain_sentinel_t, dynamic_t);
MakeCVTableTest(cv_bvs_t, plain_sentinel_t, uint_t40);
MakeCVTableTest(cv_bvs_t, buckets_bv_t,     uint8_t);
MakeCVTableTest(cv_bvs_t, buckets_bv_t,     uint64_t);
MakeCVTableTest(cv_bvs_t, buckets_bv_t,     dynamic_t);
MakeCVTableTest(cv_bvs_t, buckets_bv_t,     uint_t40);

template<typename placement_t, template<typename> typename table_t, typename val_t>
void DPTableTest() {
    using tab_t = table_t<satellite_data_t<val_t>>;
    using widths_t = typename satellite_data_t<val_t>::entry_bit_width_t;
    using value_type = typename cbp::cbp_repr_t<val_t>::value_type;

    struct TestSizeMgr {
        size_t table_size;
        inline size_t mod_add(size_t i, size_t delta = 1) const {
            return (i + delta) % table_size;
        }
        inline size_t mod_sub(size_t i, size_t delta = 1) const {
            return (i + table_size - delta) % table_size;
        }
    };

    widths_t ws { 5, 7 };
    auto size_mgr = TestSizeMgr { 128 };
    auto t = tab_t(size_mgr.table_size, ws, {});
    auto p = placement_t(size_mgr.table_size, {});

    auto tctx = t.context(size_mgr.table_size, ws);
    auto pctx = p.context(t, size_mgr.table_size, ws, size_mgr);

    auto check_insert = [&](auto ia, auto value, auto sq, bool should_exists) {
        auto res = pctx.lookup_insert(ia, sq);
        ASSERT_EQ(res.key_already_exist(), should_exists);
        ASSERT_EQ(res.ptr().get_quotient(), sq);
        *res.ptr().val_ptr() = value;
    };
    auto table_state = [&](std::vector<std::array<uint64_t, 3>> const& should) {
        std::vector<std::array<uint64_t, 3>> r;
        for (size_t i = 0; i < size_mgr.table_size; i++) {
            auto tpos = tctx.table_pos(i);
            if (!tctx.pos_is_empty(tpos)) {
                // TODO: Replace with search()
                auto ptr = tctx.at(tpos);
                r.push_back(std::array<uint64_t, 3>{
                    i, value_type(*ptr.val_ptr()), ptr.get_quotient()
                });
            }
        }
        auto is = r;
        ASSERT_EQ(is, should);
    };

    check_insert(60, 1, 5U, false);
    table_state({
        {60, 1, 5},
    });

    check_insert(66, 2, 5U, false);
    table_state({
        {60, 1, 5},
        {66, 2, 5},
    });

    check_insert(64, 3, 5U, false);
    table_state({
        {60, 1, 5},
        {64, 3, 5},
        {66, 2, 5},
    });

    check_insert(62, 4, 5U, false);
    table_state({
        {60, 1, 5},
        {62, 4, 5},
        {64, 3, 5},
        {66, 2, 5},
    });

    check_insert(62, 5, 6U, false);
    table_state({
        {60, 1, 5},
        {62, 4, 5},
        {63, 5, 6},
        {64, 3, 5},
        {66, 2, 5},
    });

    check_insert(62, 10, 6U, true);
    table_state({
        {60, 1, 5},
        {62, 4, 5},
        {63, 10, 6},
        {64, 3, 5},
        {66, 2, 5},
    });

    check_insert(62, 9, 7U, false);
    table_state({
        {60, 1, 5},
        {62, 4, 5},
        {63, 10, 6},
        {64, 3, 5},
        {65, 9, 7},
        {66, 2, 5},
    });

    /*
     Test:
     - multiple independ inserts
     - appends to same group
     - appends to displaced group

     */
}

#define MakeDPTableTest(place, tab, tname)      \
TEST(DPTable, place##_##tab##_##tname##_test) { \
    DPTableTest<place, tab, tname>();           \
}

using naive_displacement_t = displacement_t<naive_displacement_table_t>;
MakeDPTableTest(naive_displacement_t, plain_sentinel_t, uint8_t);
MakeDPTableTest(naive_displacement_t, plain_sentinel_t, uint64_t);
MakeDPTableTest(naive_displacement_t, plain_sentinel_t, dynamic_t);
MakeDPTableTest(naive_displacement_t, plain_sentinel_t, uint_t40);
MakeDPTableTest(naive_displacement_t, buckets_bv_t,     uint8_t);
MakeDPTableTest(naive_displacement_t, buckets_bv_t,     uint64_t);
MakeDPTableTest(naive_displacement_t, buckets_bv_t,     dynamic_t);
MakeDPTableTest(naive_displacement_t, buckets_bv_t,     uint_t40);

using layered_displacement_t = displacement_t<layered_displacement_table_t<static_layered_bit_width_t<4>>>;
MakeDPTableTest(layered_displacement_t, plain_sentinel_t, uint8_t);
MakeDPTableTest(layered_displacement_t, plain_sentinel_t, uint64_t);
MakeDPTableTest(layered_displacement_t, plain_sentinel_t, dynamic_t);
MakeDPTableTest(layered_displacement_t, plain_sentinel_t, uint_t40);
MakeDPTableTest(layered_displacement_t, buckets_bv_t,     uint8_t);
MakeDPTableTest(layered_displacement_t, buckets_bv_t,     uint64_t);
MakeDPTableTest(layered_displacement_t, buckets_bv_t,     dynamic_t);
MakeDPTableTest(layered_displacement_t, buckets_bv_t,     uint_t40);

using layered_displacement2_t = displacement_t<layered_displacement_table_t<dynamic_layered_bit_width_t>>;
MakeDPTableTest(layered_displacement2_t, plain_sentinel_t, uint8_t);
MakeDPTableTest(layered_displacement2_t, plain_sentinel_t, uint64_t);
MakeDPTableTest(layered_displacement2_t, plain_sentinel_t, dynamic_t);
MakeDPTableTest(layered_displacement2_t, plain_sentinel_t, uint_t40);
MakeDPTableTest(layered_displacement2_t, buckets_bv_t,     uint8_t);
MakeDPTableTest(layered_displacement2_t, buckets_bv_t,     uint64_t);
MakeDPTableTest(layered_displacement2_t, buckets_bv_t,     dynamic_t);
MakeDPTableTest(layered_displacement2_t, buckets_bv_t,     uint_t40);

using elias_gamma_displacement_t = displacement_t<elias_gamma_displacement_table_t<fixed_elias_gamma_bucket_size_t<1024>>>;
MakeDPTableTest(elias_gamma_displacement_t, plain_sentinel_t, uint8_t);
MakeDPTableTest(elias_gamma_displacement_t, plain_sentinel_t, uint64_t);
MakeDPTableTest(elias_gamma_displacement_t, plain_sentinel_t, dynamic_t);
MakeDPTableTest(elias_gamma_displacement_t, plain_sentinel_t, uint_t40);
MakeDPTableTest(elias_gamma_displacement_t, buckets_bv_t,     uint8_t);
MakeDPTableTest(elias_gamma_displacement_t, buckets_bv_t,     uint64_t);
MakeDPTableTest(elias_gamma_displacement_t, buckets_bv_t,     dynamic_t);
MakeDPTableTest(elias_gamma_displacement_t, buckets_bv_t,     uint_t40);

using elias_gamma_displacement2_t = displacement_t<elias_gamma_displacement_table_t<dynamic_fixed_elias_gamma_bucket_size_t>>;
MakeDPTableTest(elias_gamma_displacement2_t, plain_sentinel_t, uint8_t);
MakeDPTableTest(elias_gamma_displacement2_t, plain_sentinel_t, uint64_t);
MakeDPTableTest(elias_gamma_displacement2_t, plain_sentinel_t, dynamic_t);
MakeDPTableTest(elias_gamma_displacement2_t, plain_sentinel_t, uint_t40);
MakeDPTableTest(elias_gamma_displacement2_t, buckets_bv_t,     uint8_t);
MakeDPTableTest(elias_gamma_displacement2_t, buckets_bv_t,     uint64_t);
MakeDPTableTest(elias_gamma_displacement2_t, buckets_bv_t,     dynamic_t);
MakeDPTableTest(elias_gamma_displacement2_t, buckets_bv_t,     uint_t40);

using elias_gamma_displacement3_t = displacement_t<elias_gamma_displacement_table_t<growing_elias_gamma_bucket_size_t>>;
MakeDPTableTest(elias_gamma_displacement3_t, plain_sentinel_t, uint8_t);
MakeDPTableTest(elias_gamma_displacement3_t, plain_sentinel_t, uint64_t);
MakeDPTableTest(elias_gamma_displacement3_t, plain_sentinel_t, dynamic_t);
MakeDPTableTest(elias_gamma_displacement3_t, plain_sentinel_t, uint_t40);
MakeDPTableTest(elias_gamma_displacement3_t, buckets_bv_t,     uint8_t);
MakeDPTableTest(elias_gamma_displacement3_t, buckets_bv_t,     uint64_t);
MakeDPTableTest(elias_gamma_displacement3_t, buckets_bv_t,     dynamic_t);
MakeDPTableTest(elias_gamma_displacement3_t, buckets_bv_t,     uint_t40);

template<template<typename> typename table_t, typename val_t>
void FullTableTest() {
    {
        table_t<val_t> table;

        table.insert_kv_width(42, 124, 8, 8);

        auto r = table[42];
        ASSERT_EQ(r, 124u);
    }
    {
        table_t<val_t> table;

        auto tchk = [&](size_t end) {
            for (uint64_t w = 1; w < end; w++) {
                auto r = table[w];
                ASSERT_EQ(r, w);
            }
            auto nptr = typename table_t<val_t>::pointer_type();
            for (uint64_t w = 1; w < end; w++) {
                auto r = table.search(w);
                ASSERT_NE(r, nptr);
                ASSERT_EQ(*r, w);
            }
        };
        bool quick = true;

        size_t last_bits = 0;
        for (uint64_t v = 1; v < 1000; v++) {
            size_t bits = bits_for(v);
            if (last_bits != bits) {
                //std::cout << "bits: " << bits << "\n";
                last_bits = bits;
            }
            table.insert_kv_width(v, std::move(v), bits, bits);

            if (!quick) {
                tchk(v + 1);
            }
        }
        if (quick) {
            tchk(1000);
        }
    }
}

#define MakeFullTableTest(tab, tname)   \
TEST(FullTable, tab##_##tname##_test) { \
    FullTableTest<tab, tname>(); \
}

template<typename val_t>
using csh_test_t = hashmap_t<val_t, poplar_xorshift_t, buckets_bv_t, cv_bvs_t>;
template<typename val_t>
using ch_test_t = hashmap_t<val_t, poplar_xorshift_t, plain_sentinel_t, cv_bvs_t>;

template<typename val_t>
using csh_disp_test_t = hashmap_t<val_t, poplar_xorshift_t, buckets_bv_t, naive_displacement_t>;
template<typename val_t>
using ch_disp_test_t = hashmap_t<val_t, poplar_xorshift_t, plain_sentinel_t, naive_displacement_t>;

MakeFullTableTest(csh_test_t, uint16_t)
MakeFullTableTest(csh_test_t, uint64_t)
MakeFullTableTest(csh_test_t, dynamic_t)
MakeFullTableTest(csh_test_t, uint_t40)
MakeFullTableTest(ch_test_t, uint16_t)
MakeFullTableTest(ch_test_t, uint64_t)
MakeFullTableTest(ch_test_t, dynamic_t)
MakeFullTableTest(ch_test_t, uint_t40)
MakeFullTableTest(csh_disp_test_t, uint16_t)
MakeFullTableTest(csh_disp_test_t, uint64_t)
MakeFullTableTest(csh_disp_test_t, dynamic_t)
MakeFullTableTest(csh_disp_test_t, uint_t40)
MakeFullTableTest(ch_disp_test_t, uint16_t)
MakeFullTableTest(ch_disp_test_t, uint64_t)
MakeFullTableTest(ch_disp_test_t, dynamic_t)
MakeFullTableTest(ch_disp_test_t, uint_t40)
