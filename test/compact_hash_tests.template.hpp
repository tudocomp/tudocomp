#include <gtest/gtest.h>

#include <cstdint>
#include <algorithm>

using namespace tdc;
using namespace tdc::compact_hash;
using namespace tdc::compact_hash::map;

template<typename val_t>
using compact_hash_type = COMPACT_TABLE <val_t>;

struct Init {
    uint32_t value = 0;
    bool m_copyable;

    Init() {
        value = 1;
        m_copyable = true;
    }

    Init(size_t v, bool copyable = false) {
        value = v + 2;
        m_copyable = copyable;
    }

    ~Init() {
        EXPECT_NE(value, 0U) << "destroying a already destroyed value";
        value = 0;
    }

    Init(Init&& other) {
        EXPECT_NE(other.value, 0U) << "moving from a already destroyed value";

        m_copyable = other.m_copyable;
        this->value = other.value;
    }

    Init& operator=(Init&& other) {
        EXPECT_NE(other.value, 0U) << "moving from a already destroyed value";
        EXPECT_NE(value, 0U) << "moving into a already destroyed value";

        m_copyable = other.m_copyable;
        this->value = other.value;

        return *this;
    }

    Init(Init const& other) {
        EXPECT_NE(other.value, 0U) << "moving from a already destroyed value";
        EXPECT_TRUE(other.m_copyable) << "copying a non-copyable value";

        if (!other.m_copyable) {
            throw std::runtime_error("asd");
        }

        m_copyable = other.m_copyable;
        this->value = other.value;
    }

    Init& operator=(Init const& other) {
        EXPECT_NE(value, 0U) << "moving into a already destroyed value";
        EXPECT_NE(other.value, 0U) << "moving from a already destroyed value";
        EXPECT_TRUE(other.m_copyable) << "copying a non-copyable value";

        m_copyable = other.m_copyable;
        this->value = other.value;

        return *this;
    }

    static Init copyable(size_t v) {
        return Init(v, true);
    }
};

std::ostream& operator<<(std::ostream& os, Init const& v) {
    if (v.value == 0) {
        return os << "Init(<destroyed>)";
    } else if (v.value == 1) {
        return os << "Init()";
    } else {
        return os << "Init(" << (v.value - 2) << ")";
    }
}

bool operator==(Init const& lhs, Init const& rhs) {
    EXPECT_NE(lhs.value, 0U) << "comparing against destroyed value";
    EXPECT_NE(rhs.value, 0U) << "comparing against destroyed value";
    return lhs.value == rhs.value;
}

/// Assert that a element exists in the hashtable
template<typename table_t>
inline void debug_check_single(table_t& table, uint64_t key, typename table_t::value_type const& val) {
    {
        auto ptr = table.search(key);
        EXPECT_NE(ptr, nullptr) << "key " << key << " not found!";
        if (ptr != nullptr) {
            EXPECT_EQ(*ptr, val) << "value is " << *ptr << " instead of " << val;
        }
        if (ptr == nullptr) abort();
    }
    {
        auto ptr = table.find(key);
        EXPECT_NE(ptr, nullptr) << "key " << key << " not found!";
        if (ptr != nullptr) {
            EXPECT_EQ(*ptr, val) << "value is " << *ptr << " instead of " << val;
        }
        if (ptr == nullptr) abort();
    }
    {
        auto c = table.count(key);
        EXPECT_NE(c, 0U) << "key " << key << " not found!";
    }
}

template<typename hashfn_t>
void test_hashfn() {
    for(uint32_t w = 1; w <= 64; w++) {
        hashfn_t fn { w, {} };

        size_t max_val = std::min<size_t>((((1 << (w-1)) << 1) - 1), 1000);

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

TEST(hash, insert) {
    auto ch = compact_hash_type<Init>(256, 16, 1);
    ch.insert(44, Init(0));
    ch.insert(45, Init(1));
    ch.insert(45, Init(2));
    ch.insert(44 + 256, Init(3));
    ch.insert(45 + 256, Init(4));
    ch.insert(46, Init(5));

    ch.insert(44, Init(6));
    ch.insert(45, Init(7));
    ch.insert(44 + 256, Init(8));
    ch.insert(45 + 256, Init(9));
    ch.insert(46, Init(10));

    //ch.insert(0);
    //ch.insert(4);
    //ch.insert(9);
    //ch.insert(128);

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, insert_wrap) {
    auto ch = compact_hash_type<Init>(4, 16, 1);
    ch.insert(3, Init(0));
    ch.insert(7, Init(1));
    ch.insert(15, Init(2));

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, insert_move_wrap) {
    auto ch = compact_hash_type<Init>(8, 16, 1);

    ch.insert(3, Init(0));
    ch.insert(3 + 8, Init(1));

    ch.insert(5, Init(2));
    ch.insert(5 + 8, Init(3));
    ch.insert(5 + 16, Init(4));
    ch.insert(5 + 24, Init(5));

    ch.insert(4, Init(6));

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

    debug_check_single(ch, 3,      Init::copyable(0));
    debug_check_single(ch, 3 + 8,  Init::copyable(1));
    debug_check_single(ch, 5,      Init::copyable(2));
    debug_check_single(ch, 5 + 8,  Init::copyable(3));
    debug_check_single(ch, 5 + 16, Init::copyable(4));
    debug_check_single(ch, 5 + 24, Init::copyable(5));
    debug_check_single(ch, 4,      Init::copyable(6));
}

TEST(hash, cornercase) {
    auto ch = compact_hash_type<Init>(8, 16, 1);

    ch.insert(0, Init(0));
    ch.insert(0 + 8, Init(1));

    debug_check_single(ch, 0,      Init::copyable(0));
    debug_check_single(ch, 0 + 8,  Init::copyable(1));

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, grow) {
    std::vector<std::pair<uint64_t, Init>> inserted;

    auto ch = compact_hash_type<Init>(0, 10, 1); // check that it grows to minimum 2

    auto add = [&](auto key, auto&& v0, auto&& v1) {
        ch.insert(key, std::move(v0));
        //inserted.clear();
        inserted.push_back({ key, std::move(v1) });
        for (auto& kv : inserted) {
            debug_check_single(ch, kv.first, kv.second);
        }
    };


    for(size_t i = 0; i < 1000; i++) {
        add(i, Init(i), Init::copyable(i));
    }

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, grow_bits) {
    std::vector<std::pair<uint64_t, Init>> inserted;

    auto ch = compact_hash_type<Init>(0, 10, 1); // check that it grows to minimum 2

    uint8_t bits = 1;

    auto add = [&](auto key, auto&& v0, auto&& v1) {
        bits = std::max(bits, bits_for(key));

        ch.insert_key_width(key, std::move(v0), bits);
        //inserted.clear();
        inserted.push_back({ key, std::move(v1) });
        for (auto& kv : inserted) {
            debug_check_single(ch, kv.first, kv.second);
        }
    };


    for(size_t i = 0; i < 1000; i++) {
        add(i, Init(i), Init::copyable(i));
    }

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, grow_bits_larger) {
    std::vector<std::pair<uint64_t, Init>> inserted;

    auto ch = compact_hash_type<Init>(0, 0, 1); // check that it grows to minimum 2

    uint8_t bits = 1;

    auto add = [&](auto key, auto&& v0, auto&& v1) {
        bits = std::max(bits, bits_for(key));

        ch.insert_key_width(key, std::move(v0), bits);
        inserted.clear();
        inserted.push_back({ key, std::move(v1) });
        for (auto& kv : inserted) {
            debug_check_single(ch, kv.first, kv.second);
        }
    };


    for(size_t i = 0; i < 10000; i++) {
        add(i*13ull, Init(i), Init::copyable(i));
    }
}

TEST(hash, grow_bits_larger_address) {
    std::vector<std::pair<uint64_t, Init>> inserted;

    auto ch = compact_hash_type<Init>(0, 0, 1); // check that it grows to minimum 2

    uint8_t bits = 1;

    auto add = [&](auto key, auto&& v0, auto&& v1) {
        bits = std::max(bits, bits_for(key));

        auto&& r = ch.access_key_width(key, bits);
        ASSERT_EQ(r, Init());
        r = std::move(v0);
        ASSERT_EQ(r, v1);
        inserted.clear();
        inserted.push_back({ key, std::move(v1) });
        for (auto& kv : inserted) {
            debug_check_single(ch, kv.first, kv.second);
        }
    };


    for(size_t i = 0; i < 10000; i++) {
        add(i*13ull, Init(i), Init::copyable(i));
    }

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";
}

struct find_or_insert_tracer_t {
    compact_hash_type<uint64_t> ch;
    find_or_insert_tracer_t(size_t bit_width):
        ch {0, bit_width} {}
    size_t i = 0;
    bool abort = false;
    std::string last;
    bool print = false;
    inline void find_or_insert(uint64_t key, uint64_t existing_value, uint64_t new_value) {
        if (abort) return;

        auto& val = ch[key];
        if (val == 0) {
            val = new_value;
        }
        if (print) std::cout << "find_or_insert(" << key << ", " << existing_value << ", " << new_value << ") \t#" << (i++) << "\n";

        if (val != existing_value) {
            abort = true;
            std::cout << "[before]" << "\n";
            std::cout << last << "\n";
            std::cout << "[after]" << "\n";
            // TODO DEBUG
            // std::cout << ch.debug_state() << "\n";
        }
        ASSERT_EQ(val, existing_value);
        // TODO DEBUG
        // last = ch.debug_state();
    };
};

TEST(hash, lookup_bug) {
    auto tracer = find_or_insert_tracer_t {16};

    tracer.find_or_insert(0, 0, 0);
    tracer.find_or_insert(97, 1, 1);
    tracer.find_or_insert(98, 2, 2);
    tracer.find_or_insert(99, 3, 3);
    tracer.find_or_insert(100, 4, 4);
    tracer.find_or_insert(101, 5, 5);
    tracer.find_or_insert(98, 2, 6);
    tracer.find_or_insert(611, 6, 6);
    tracer.find_or_insert(100, 4, 7);
    tracer.find_or_insert(1125, 7, 7);
    tracer.find_or_insert(97, 1, 8);
    tracer.find_or_insert(354, 8, 8);
    tracer.find_or_insert(99, 3, 9);
}

TEST(hash, lookup_bug2) {
    auto tracer = find_or_insert_tracer_t {16};

    tracer.find_or_insert(0, 0, 0);
    tracer.find_or_insert(97, 1, 1);
    tracer.find_or_insert(115, 2, 2);
    tracer.find_or_insert(100, 3, 3);
    tracer.find_or_insert(102, 4, 4);
    tracer.find_or_insert(97, 1, 1);
    tracer.find_or_insert(371, 5, 5);
    tracer.find_or_insert(99, 6, 6);
    tracer.find_or_insert(116, 7, 7);
    tracer.find_or_insert(106, 8, 8);
    tracer.find_or_insert(107, 9, 9);
    tracer.find_or_insert(99, 6, 6);
    tracer.find_or_insert(1634, 10, 10);
    tracer.find_or_insert(119, 11, 11);
    tracer.find_or_insert(101, 12, 12);
    tracer.find_or_insert(97, 1, 1);
    tracer.find_or_insert(371, 5, 5);
    tracer.find_or_insert(1378, 13, 13);
    tracer.find_or_insert(101, 12, 12);
    tracer.find_or_insert(3170, 14, 14);
    tracer.find_or_insert(118, 15, 15);
    tracer.find_or_insert(116, 7, 7);
    tracer.find_or_insert(1897, 16, 16);
    tracer.find_or_insert(119, 11, 11);
    tracer.find_or_insert(2917, 17, 17);
    tracer.find_or_insert(116, 7, 7);
    tracer.find_or_insert(1911, 18, 18);
    tracer.find_or_insert(99, 6, 6);
    tracer.find_or_insert(1646, 19, 19);
    tracer.find_or_insert(98, 20, 20);
    tracer.find_or_insert(119, 11, 11);
    tracer.find_or_insert(2914, 21, 21);
    tracer.find_or_insert(98, 20, 20);
    tracer.find_or_insert(5233, 22, 22);
    tracer.find_or_insert(110, 23, 23);
    tracer.find_or_insert(113, 24, 24);
    tracer.find_or_insert(120, 25, 25);
    tracer.find_or_insert(101, 12, 12);
    tracer.find_or_insert(3186, 26, 26);
    tracer.find_or_insert(110, 23, 23);
    tracer.find_or_insert(6001, 27, 27);
    tracer.find_or_insert(122, 28, 28);
    tracer.find_or_insert(101, 12, 12);
    tracer.find_or_insert(3194, 29, 29);
    tracer.find_or_insert(119, 11, 11);
    tracer.find_or_insert(2933, 30, 30);
    tracer.find_or_insert(113, 24, 24);
    tracer.find_or_insert(6263, 31, 31);
    tracer.find_or_insert(101, 12, 12);
    tracer.find_or_insert(3194, 29, 29);
    tracer.find_or_insert(7541, 32, 32);
    tracer.find_or_insert(101, 12, 12);
    tracer.find_or_insert(3188, 33, 33);
    tracer.find_or_insert(113, 24, 24);
    tracer.find_or_insert(6243, 34, 34);
    tracer.find_or_insert(114, 35, 35);
    tracer.find_or_insert(110, 23, 23);
    tracer.find_or_insert(6010, 36, 36);
    tracer.find_or_insert(120, 25, 25);
    tracer.find_or_insert(6498, 37, 37);
    tracer.find_or_insert(110, 23, 23);
    tracer.find_or_insert(5989, 38, 38);
    tracer.find_or_insert(113, 24, 24);
    tracer.find_or_insert(6245, 39, 39);
    tracer.find_or_insert(98, 20, 20);
    tracer.find_or_insert(5239, 40, 40);
    tracer.find_or_insert(99, 6, 6);
    tracer.find_or_insert(1634, 10, 10);
    tracer.find_or_insert(2673, 41, 41);
    tracer.find_or_insert(119, 11, 11);
    tracer.find_or_insert(2921, 42, 42);
    tracer.find_or_insert(99, 6, 6);
    tracer.find_or_insert(1634, 10, 10);
    tracer.find_or_insert(2673, 41, 41);
    tracer.find_or_insert(10595, 43, 43);
    tracer.find_or_insert(98, 20, 20);
    tracer.find_or_insert(5236, 44, 44);
    tracer.find_or_insert(110, 23, 23);
    tracer.find_or_insert(6001, 27, 27);
    tracer.find_or_insert(7031, 45, 45);
    tracer.find_or_insert(101, 12, 12);
    tracer.find_or_insert(3185, 46, 46);
    tracer.find_or_insert(120, 25, 25);
    tracer.find_or_insert(6499, 47, 47);
    tracer.find_or_insert(98, 20, 20);
    tracer.find_or_insert(5239, 40, 40);
    tracer.find_or_insert(10357, 48, 48);
    tracer.find_or_insert(101, 12, 12);
    tracer.find_or_insert(3192, 49, 49);
    tracer.find_or_insert(99, 6, 6);
    tracer.find_or_insert(1634, 10, 10);
    tracer.find_or_insert(2682, 50, 50);
    tracer.find_or_insert(113, 24, 24);
    tracer.find_or_insert(6263, 31, 31);
    tracer.find_or_insert(8037, 51, 51);
    tracer.find_or_insert(122, 28, 28);
    tracer.find_or_insert(7267, 52, 52);
    tracer.find_or_insert(113, 24, 24);
    tracer.find_or_insert(6242, 53, 53);
    tracer.find_or_insert(119, 11, 11);
    tracer.find_or_insert(2917, 17, 17);
    tracer.find_or_insert(4451, 54, 54);
    tracer.find_or_insert(113, 24, 24);
    tracer.find_or_insert(6242, 53, 53);
    tracer.find_or_insert(13687, 55, 55);
    tracer.find_or_insert(100, 3, 3);
    tracer.find_or_insert(865, 56, 56);
    tracer.find_or_insert(115, 2, 2);
    tracer.find_or_insert(627, 57, 57);
    tracer.find_or_insert(100, 3, 3);
    tracer.find_or_insert(865, 56, 56);
    tracer.find_or_insert(14451, 58, 58);
    tracer.find_or_insert(100, 3, 3);
    tracer.find_or_insert(870, 59, 59);
    tracer.find_or_insert(122, 28, 28);
    tracer.find_or_insert(7268, 60, 60);
    tracer.find_or_insert(102, 4, 4);
    tracer.find_or_insert(1127, 61, 61);
    tracer.find_or_insert(102, 4, 4);
    tracer.find_or_insert(1139, 62, 62);
    tracer.find_or_insert(100, 3, 3);
    tracer.find_or_insert(870, 59, 59);
    tracer.find_or_insert(15219, 63, 63);
    tracer.find_or_insert(100, 3, 3);
    tracer.find_or_insert(871, 64, 64);
    tracer.find_or_insert(102, 4, 4);
    tracer.find_or_insert(1124, 65, 65);
    tracer.find_or_insert(117, 66, 66);
    tracer.find_or_insert(99, 6, 6);
    tracer.find_or_insert(1637, 67, 67);
    tracer.find_or_insert(122, 28, 28);
    tracer.find_or_insert(7267, 52, 52);
    tracer.find_or_insert(13428, 68, 68);
    tracer.find_or_insert(122, 28, 28);
    tracer.find_or_insert(7281, 69, 69);
    tracer.find_or_insert(119, 11, 11);
    tracer.find_or_insert(2917, 17, 17);
    tracer.find_or_insert(4450, 70, 70);
    tracer.find_or_insert(99, 6, 6);
    tracer.find_or_insert(1652, 71, 71);
    tracer.find_or_insert(117, 66, 66);
    tracer.find_or_insert(17001, 72, 72);
    tracer.find_or_insert(113, 24, 24);
    tracer.find_or_insert(6263, 31, 31);
    tracer.find_or_insert(8041, 73, 73);
    tracer.find_or_insert(105, 74, 74);
    tracer.find_or_insert(113, 24, 24);
    tracer.find_or_insert(6243, 34, 34);
    tracer.find_or_insert(8802, 75, 75);
    tracer.find_or_insert(110, 23, 23);
    tracer.find_or_insert(6010, 36, 36);
    tracer.find_or_insert(9315, 76, 76);
    tracer.find_or_insert(101, 12, 12);
    tracer.find_or_insert(3170, 14, 14);
    tracer.find_or_insert(3706, 77, 77);
    tracer.find_or_insert(113, 24, 24);
    tracer.find_or_insert(6243, 34, 34);
}

constexpr size_t load_max = 100000;
//constexpr size_t load_max = 100;

template<typename val_t = uint64_t, bool use_index = false, bool grow_values = false>
void load_factor_test(float z) {
    auto table = compact_hash_type<val_t>(0, 1);
    // TODO DEBUG
    // table.debug_state();

    table.max_load_factor(z);
    for(size_t i = 0; i < load_max; i++) {
        auto new_value = i * 2 + 1;

        if (grow_values) {
            if (use_index) {
                table.access_kv_width(i, bits_for(i), bits_for(new_value)) = new_value;
            } else {
                table.insert_kv_width(i, size_t(new_value), bits_for(i), bits_for(new_value));
            }
        } else {
            if (use_index) {
                table.access_key_width(i, bits_for(i)) = new_value;
            } else {
                table.insert_key_width(i, size_t(new_value), bits_for(i));
            }
        }
    }
    //std::cout << table.debug_print_storage() << "\n";
    for(size_t i = 0; i < load_max; i++) {
        auto p = table.search(i);
        ASSERT_NE(p, ValPtr<val_t>());
        ASSERT_EQ(*p, i*2 + 1);
    }
    auto p = table.search(load_max);
    ASSERT_EQ(p, ValPtr<val_t>());

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

TEST(hash_bit_value_a, bit_value_100_50_a) {
    load_factor_test<uint_t<50>>(1.0);
}
TEST(hash_bit_value_a, bit_value_100_40_a) {
    load_factor_test<uint_t<40>>(1.0);
}
TEST(hash_bit_value_a, bit_value_100_20_a) {
    load_factor_test<uint_t<20>>(1.0);
}
TEST(hash_bit_value_a, bit_value_100_19_a) {
    load_factor_test<uint_t<19>>(1.0);
}
TEST(hash_bit_value_b, bit_value_100_50_b) {
    load_factor_test<uint_t<50>, true>(1.0);
}
TEST(hash_bit_value_b, bit_value_100_40_b) {
    load_factor_test<uint_t<40>, true>(1.0);
}
TEST(hash_bit_value_b, bit_value_100_20_b) {
    load_factor_test<uint_t<20>, true>(1.0);
}
TEST(hash_bit_value_b, bit_value_100_19_b) {
    load_factor_test<uint_t<19>, true>(1.0);
}

TEST(hash_max_dynamic, max_load_10) {
    load_factor_test<dynamic_t, true, true>(0.1);
}
TEST(hash_max_dynamic, max_load_20) {
    load_factor_test<dynamic_t, true, true>(0.2);
}
TEST(hash_max_dynamic, max_load_30) {
    load_factor_test<dynamic_t, true, true>(0.3);
}
TEST(hash_max_dynamic, max_load_40) {
    load_factor_test<dynamic_t, true, true>(0.4);
}
TEST(hash_max_dynamic, max_load_50) {
    load_factor_test<dynamic_t, true, true>(0.5);
}
TEST(hash_max_dynamic, max_load_60) {
    load_factor_test<dynamic_t, true, true>(0.6);
}
TEST(hash_max_dynamic, max_load_70) {
    load_factor_test<dynamic_t, true, true>(0.7);
}
TEST(hash_max_dynamic, max_load_80) {
    load_factor_test<dynamic_t, true, true>(0.8);
}
TEST(hash_max_dynamic, max_load_90) {
    load_factor_test<dynamic_t, true, true>(0.9);
}
TEST(hash_max_dynamic, max_load_100) {
    load_factor_test<dynamic_t, true, true>(1.0);
}
