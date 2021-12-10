#include <gtest/gtest.h>

#include <cstdint>
#include <algorithm>
#include <unordered_set>

using namespace tdc;
using namespace tdc::compact_hash;
using namespace tdc::compact_hash::set;

using compact_hash_type = COMPACT_TABLE;

struct shadow_sets_t {
    std::unordered_set<uint64_t> keys;
    std::unordered_set<uint64_t> ids;
    compact_hash_type& table;

    shadow_sets_t(compact_hash_type& t): table(t) {}

    struct on_resize_t {
        shadow_sets_t& self;

        inline void on_resize(size_t table_size) {
            self.keys.clear();
            self.ids.clear();
        }
        inline void on_reinsert(uint64_t key, uint64_t id) {
            self.new_key(key, id);
        }
    };
    auto on_resize() {
        return on_resize_t { *this };
    }

    void new_key(uint64_t key, uint64_t id) {
        // std::cout << "insert(key=" << key << ", id=" << id << ")\n";
        EXPECT_TRUE(keys.count(key) == 0) << "Key " << key << " already exists";
        EXPECT_TRUE(ids.count(id) == 0) << "Id " << id << " already exists";
        keys.insert(key);
        ids.insert(id);
    }

    void existing_key(uint64_t key, uint64_t id) {
        EXPECT_TRUE(keys.count(key) == 1) << "Key " << key << " does not exists";
        EXPECT_TRUE(ids.count(id) == 1) << "Id " << id << " does not exists";
    }

    auto lookup(uint64_t key) {
        auto r = table.lookup(key);

        if (r.found()) {
            EXPECT_TRUE(r.key_already_exist());
            existing_key(key, r.id());
        }

        return r;
    }
    auto lookup_insert(uint64_t key) {
        auto r = table.lookup_insert(key, on_resize());

        if (r.key_already_exist()) {
            existing_key(key, r.id());
        } else {
            new_key(key, r.id());
        }

        return r;
    }
    auto lookup_insert_key_width(uint64_t key, uint8_t key_width) {
        auto r = table.lookup_insert_key_width(key, key_width, on_resize());

        if (r.key_already_exist()) {
            existing_key(key, r.id());
        } else {
            new_key(key, r.id());
        }

        return r;
    }
    void max_load_factor(double v) {
        table.max_load_factor(v);
    }
};

/// Assert that a element exists in the hashtable
inline void debug_check_single(compact_hash_type& table, uint64_t key) {
    auto r = table.lookup(key);
    ASSERT_TRUE(r.found()) << "key " << key << " not found!";

    auto c = table.count(key);
    ASSERT_EQ(c, 1U) << "key " << key << " not found!";

    auto p = table.find(key);
    ASSERT_TRUE(p != decltype(p)()) << "key " << key << " not found!";
    ASSERT_TRUE(p != nullptr) << "key " << key << " not found!";
    ASSERT_TRUE(*p == key) << "key " << key << " not found!";
}

/// Assert that a element exists in the hashtable
inline void debug_check_single(shadow_sets_t& table, uint64_t key) {
    auto r = table.lookup(key);
    ASSERT_TRUE(r.found()) << "key " << key << " not found!";
    table.existing_key(key, r.id());
}

template<typename hashfn_t>
void test_hashfn() {
    for(uint32_t w = 1; w < 64; w++) {
        hashfn_t fn { w, {} };

        size_t max_val = std::min<size_t>(((1 << w) - 1), 1000);

        for (size_t i = 0; i < (max_val + 1); i++) {
            auto hi = fn.hash(i);
            auto hhi = fn.hash_inv(hi);
            /*
            std::cout
                << w << ", "
                << i << ", "
                << hi << ", "
                << hhi << "\n";
            */

            ASSERT_EQ(i, hhi);
        }
    }
}

TEST(hashfn, xorshift) {
    test_hashfn<compact_hash::xorshift_t>();
}

TEST(hashfn, poplar_xorshift) {
    test_hashfn<compact_hash::poplar_xorshift_t>();
}

TEST(hash, lookup_insert) {
    auto chx = compact_hash_type(256, 16);
    auto ch = shadow_sets_t(chx);

    ch.lookup_insert(44);
    ch.lookup_insert(45);
    ch.lookup_insert(45);
    ch.lookup_insert(44 + 256);
    ch.lookup_insert(45 + 256);
    ch.lookup_insert(46);

    ch.lookup_insert(44);
    ch.lookup_insert(45);
    ch.lookup_insert(44 + 256);
    ch.lookup_insert(45 + 256);
    ch.lookup_insert(46);

    //ch.lookup_insert(0);
    //ch.lookup_insert(4);
    //ch.lookup_insert(9);
    //ch.lookup_insert(128);

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, lookup_insert_wrap) {
    auto chx = compact_hash_type(4, 16);
    auto ch = shadow_sets_t(chx);
    ch.max_load_factor(1.0);

    ch.lookup_insert(3);
    ch.lookup_insert(7);
    ch.lookup_insert(15);

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, lookup_insert_move_wrap) {
    auto chx = compact_hash_type(8, 16);
    auto ch = shadow_sets_t(chx);
    ch.max_load_factor(1.0);

    ch.lookup_insert(3);
    ch.lookup_insert(3 + 8);

    ch.lookup_insert(5);
    ch.lookup_insert(5 + 8);
    ch.lookup_insert(5 + 16);
    ch.lookup_insert(5 + 24);

    ch.lookup_insert(4);

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

    debug_check_single(ch, 3);
    debug_check_single(ch, 3 + 8);
    debug_check_single(ch, 5);
    debug_check_single(ch, 5 + 8);
    debug_check_single(ch, 5 + 16);
    debug_check_single(ch, 5 + 24);
    debug_check_single(ch, 4);
}

TEST(hash, cornercase) {
    auto chx = compact_hash_type(8, 16);
    auto ch = shadow_sets_t(chx);

    ch.lookup_insert(0);
    ch.lookup_insert(0 + 8);

    debug_check_single(ch, 0);
    debug_check_single(ch, 0 + 8);

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, grow) {
    std::vector<uint64_t> lookup_inserted;

    auto chx = compact_hash_type(0, 10); // check that it grows to minimum 2
    auto ch = shadow_sets_t(chx);

    auto add = [&](auto key) {
        ch.lookup_insert(key);
        //lookup_inserted.clear();
        lookup_inserted.push_back(key);
        for (auto& k : lookup_inserted) {
            debug_check_single(ch, k);
        }
    };


    for(size_t i = 0; i < 1000; i++) {
        add(i);
    }

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, grow_bits) {
    std::vector<uint64_t> lookup_inserted;

    auto chx = compact_hash_type(0, 10); // check that it grows to minimum 2
    auto ch = shadow_sets_t(chx);

    uint8_t bits = 1;

    auto add = [&](auto key) {
        bits = std::max(bits, bits_for(key));

        ch.lookup_insert_key_width(key, bits);
        //lookup_inserted.clear();
        lookup_inserted.push_back(key);
        for (auto& k : lookup_inserted) {
            debug_check_single(ch, k);
        }
    };


    for(size_t i = 0; i < 1000; i++) {
        add(i);
    }

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, grow_bits_larger) {
    std::vector<uint64_t> lookup_inserted;

    auto chx = compact_hash_type(0, 0); // check that it grows to minimum 2
    auto ch = shadow_sets_t(chx);

    uint8_t bits = 1;

    auto add = [&](auto key) {
        bits = std::max(bits, bits_for(key));

        ch.lookup_insert_key_width(key, bits);
        lookup_inserted.clear();
        lookup_inserted.push_back(key);
        for (auto& k : lookup_inserted) {
            debug_check_single(ch, k);
        }
    };


    for(size_t i = 0; i < 10000; i++) {
        add(i*13ull);
    }
}

TEST(hash, grow_bits_larger_address) {
    std::vector<uint64_t> lookup_inserted;

    auto chx = compact_hash_type(0, 0); // check that it grows to minimum 2
    auto ch = shadow_sets_t(chx);

    uint8_t bits = 1;

    auto add = [&](auto key) {
        bits = std::max(bits, bits_for(key));

        auto r = ch.lookup_insert_key_width(key, bits);
        ASSERT_FALSE(r.key_already_exist());
        lookup_inserted.clear();
        lookup_inserted.push_back(key);
        for (auto& k : lookup_inserted) {
            debug_check_single(ch, k);
        }
    };


    for(size_t i = 0; i < 10000; i++) {
        add(i*13ull);
    }

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";
}

constexpr size_t load_max = 100000;
//constexpr size_t load_max = 100;

void load_factor_test(float z) {
    auto tablex = compact_hash_type(0, 1);
    auto table = shadow_sets_t(tablex);
    // TODO DEBUG
    // table.debug_state();

    table.max_load_factor(z);
    for(size_t i = 0; i < load_max; i++) {
        table.lookup_insert_key_width(i, bits_for(i));
    }
    //std::cout << table.debug_print_storage() << "\n";
    for(size_t i = 0; i < load_max; i++) {
        auto r = table.lookup(i);
        ASSERT_TRUE(r.found());
    }
    auto r = table.lookup(load_max);
    ASSERT_FALSE(r.found());

    // TODO DEBUG
    /*
    auto stats = table.stat_gather();

    std::cout << "stats.buckets: " << stats.buckets << "\n";
    std::cout << "stats.allocated_buckets: " << stats.allocated_buckets << "\n";
    std::cout << "stats.buckets_real_allocated_capacity_in_bytes: " << stats.buckets_real_allocated_capacity_in_bytes << "\n";
    std::cout << "stats.real_allocated_capacity_in_bytes: " << stats.real_allocated_capacity_in_bytes << "\n";
    std::cout << "stats.theoretical_minimum_size_in_bits: " << stats.theoretical_minimum_size_in_bits << "\n";
    */
}

TEST(hash_load, max_load_10) {
    load_factor_test(0.1);
}
TEST(hash_load, max_load_20) {
    load_factor_test(0.2);
}
TEST(hash_load, max_load_30) {
    load_factor_test(0.3);
}
TEST(hash_load, max_load_40) {
    load_factor_test(0.4);
}
TEST(hash_load, max_load_50) {
    load_factor_test(0.5);
}
TEST(hash_load, max_load_60) {
    load_factor_test(0.6);
}
TEST(hash_load, max_load_70) {
    load_factor_test(0.7);
}
TEST(hash_load, max_load_80) {
    load_factor_test(0.8);
}
TEST(hash_load, max_load_90) {
    load_factor_test(0.9);
}
TEST(hash_load, max_load_100) {
    load_factor_test(1.0);
}

TEST(hash, swap) {
    auto a = compact_hash_type(8, 16);
    {
        auto& ch = a;
        ch.max_load_factor(1.0);
        ch.lookup_insert(3);
        ch.lookup_insert(3 + 8);
        ch.lookup_insert(5);
        ch.lookup_insert(5 + 8);
        ch.lookup_insert(5 + 16);
        ch.lookup_insert(5 + 24);
        ch.lookup_insert(4);
    }
    auto b = compact_hash_type(8, 16);
    {
        auto& ch = b;
        ch.max_load_factor(1.0);
        ch.lookup_insert(3);
        ch.lookup_insert(3 + 8);
        ch.lookup_insert(5);
        ch.lookup_insert(5 + 8);
        ch.lookup_insert(5 + 16);
        ch.lookup_insert(5 + 24);
        ch.lookup_insert(4);
    }

    a.swap(b);
    std::swap(a, b);
}
