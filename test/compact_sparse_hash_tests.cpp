#include <gtest/gtest.h>
#include "test/util.hpp"

#include <cstdint>
#include <algorithm>
#include <tudocomp/util/compact_sparse_hash.hpp>
#include <tudocomp/util.hpp>

using namespace tdc;

bool print_init = false;

static uint64_t global_c = 0;
struct Init {
    uint16_t c = 0;
    bool empty = false;
    bool fake = false;

    Init() {
        c = global_c++;
        if (print_init) std::cout << "construct(" << c << ")\n";
    }
    Init(size_t v) {
        c = v;
        if (print_init) std::cout << "construct fake(" << c << ")\n";
        fake = true;
    }
    ~Init() {
        if (empty) {
            if (print_init) std::cout << "destruct(-)\n";
        } else if (fake) {
            if (print_init) std::cout << "destruct fake(" << c << ")\n";
        } else {
            if (print_init) std::cout << "destruct(" << c << ")\n";
        }
    }
    Init(Init&& other) {
        c = other.c;
        other.empty = true;
        if (print_init) std::cout << "move(" << c << ")\n";
    }
    Init& operator=(Init&& other) {
        if (!empty) {
            auto old_c = c;
            c = other.c;
            other.empty = true;
            if (print_init) std::cout << "move(" << other.c << " -> " << old_c << ")\n";
        } else {
            empty = false;
            c = other.c;
            other.empty = true;
            if (print_init) std::cout << "move(" << other.c << " -> -)\n";
        }
        return *this;
    }

    Init(const Init& other) = delete;
    Init& operator=(const Init& other) = delete;

    static void reset() {
        global_c = 0;
    }
};

std::ostream& operator<<(std::ostream& os, Init const& v) {
    return os << "Init(" << v.c << ")";
}

bool operator==(Init const& lhs, Init const& rhs) {
    return lhs.c == rhs.c && lhs.empty == rhs.empty;
}

TEST(hash, xorshift) {
    for(size_t w = 10; w < 65; w++) {
        for (size_t i = 0; i < 1000; i++) {
            auto hi = compact_hashfn(i, w);
            auto hhi = compact_reverse_hashfn(hi, w);
            /*std::cout
                << i << ", "
                << hi << ", "
                << hhi << "\n";*/

            ASSERT_EQ(i, hhi);
        }
    }
}

TEST(hash, insert) {
    Init::reset();

    auto ch = compact_hash<Init>(256);
    ch.insert(44, Init());
    ch.insert(45, Init());
    ch.insert(45, Init());
    ch.insert(44 + 256, Init());
    ch.insert(45 + 256, Init());
    ch.insert(46, Init());

    ch.insert(44, Init());
    ch.insert(45, Init());
    ch.insert(44 + 256, Init());
    ch.insert(45 + 256, Init());
    ch.insert(46, Init());

    //ch.insert(0);
    //ch.insert(4);
    //ch.insert(9);
    //ch.insert(128);

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, insert_wrap) {
    Init::reset();

    auto ch = compact_hash<Init>(4);
    ch.insert(3, Init());
    ch.insert(7, Init());
    ch.insert(15, Init());

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, insert_move_wrap) {
    Init::reset();

    auto ch = compact_hash<Init>(8);

    ch.insert(3, Init());
    ch.insert(3 + 8, Init());

    ch.insert(5, Init());
    ch.insert(5 + 8, Init());
    ch.insert(5 + 16, Init());
    ch.insert(5 + 24, Init());

    ch.insert(4, Init());

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

    ch.debug_check_single(3,      Init(0));
    ch.debug_check_single(3 + 8,  Init(1));
    ch.debug_check_single(5,      Init(2));
    ch.debug_check_single(5 + 8,  Init(3));
    ch.debug_check_single(5 + 16, Init(4));
    ch.debug_check_single(5 + 24, Init(5));
    ch.debug_check_single(4,      Init(6));
}

TEST(hash, cornercase) {
    Init::reset();

    auto ch = compact_hash<Init>(8);
    ch.insert(0, Init());
    ch.insert(0 + 8, Init());

    ch.debug_check_single(0,      Init(0));
    ch.debug_check_single(0 + 8,  Init(1));

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, grow) {
    Init::reset();

    std::vector<std::pair<uint64_t, Init>> inserted;

    auto ch = compact_hash<Init>(0, 10); // check that it grows to minimum 2

    auto add = [&](auto key, auto&& v0, auto&& v1) {
        ch.insert(key, std::move(v0));
        //inserted.clear();
        inserted.push_back({ key, std::move(v1) });
        for (auto& kv : inserted) {
            ch.debug_check_single(kv.first, kv.second);
        }
    };


    for(size_t i = 0; i < 1000; i++) {
        add(i, Init(), Init(i));
    }

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, grow_bits) {
    Init::reset();

    std::vector<std::pair<uint64_t, Init>> inserted;

    auto ch = compact_hash<Init>(0, 10); // check that it grows to minimum 2

    uint8_t bits = 1;

    auto add = [&](auto key, auto&& v0, auto&& v1) {
        bits = std::max(bits, bits_for(key));

        ch.insert(key, std::move(v0), bits);
        //inserted.clear();
        inserted.push_back({ key, std::move(v1) });
        for (auto& kv : inserted) {
            ch.debug_check_single(kv.first, kv.second);
        }
    };


    for(size_t i = 0; i < 1000; i++) {
        add(i, Init(), Init(i));
    }

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";

}

TEST(hash, grow_bits_larger) {
    Init::reset();

    std::vector<std::pair<uint64_t, Init>> inserted;

    auto ch = compact_hash<Init>(0, 0); // check that it grows to minimum 2

    uint8_t bits = 1;

    auto add = [&](auto key, auto&& v0, auto&& v1) {
        bits = std::max(bits, bits_for(key));

        ch.insert(key, std::move(v0), bits);
        inserted.clear();
        inserted.push_back({ key, std::move(v1) });
        for (auto& kv : inserted) {
            ch.debug_check_single(kv.first, kv.second);
        }
    };


    for(size_t i = 0; i < 10000; i++) {
        add(i*13ull, Init(), Init(i));
    }
}

TEST(hash, grow_bits_larger_address) {
    Init::reset();

    std::vector<std::pair<uint64_t, Init>> inserted;

    auto ch = compact_hash<Init>(0, 0); // check that it grows to minimum 2

    uint8_t bits = 1;

    auto add = [&](auto key, auto&& v1) {
        bits = std::max(bits, bits_for(key));

        ASSERT_EQ(ch.index(key, bits), v1);
        inserted.clear();
        inserted.push_back({ key, std::move(v1) });
        for (auto& kv : inserted) {
            ch.debug_check_single(kv.first, kv.second);
        }
    };


    for(size_t i = 0; i < 10000; i++) {
        add(i*13ull, Init(i));
    }

    //std::cout << "=======================\n";
    //std::cout << ch.debug_state() << "\n";
    //std::cout << "=======================\n";
}

TEST(hash, lookup_bug) {
    auto ch = compact_hash<uint64_t>(0);
    size_t i = 0;
    bool abort = false;
    std::string last;
    auto find_or_insert = [&](auto key, uint64_t existing_value, uint64_t new_value) {
        if (abort) return;

        auto& val = ch[key];
        if (val == 0) {
            val = new_value;
        }
        std::cout << "find_or_insert(" << key << ", " << existing_value << ", " << new_value << ") \t#" << (i++) << "\n";

        if (val != existing_value) {
            abort = true;
            std::cout << "[before]" << "\n";
            std::cout << last << "\n";
            std::cout << "[after]" << "\n";
            std::cout << ch.debug_state() << "\n";
        }
        ASSERT_EQ(val, existing_value);
        last = ch.debug_state();
    };

    find_or_insert(0, 0, 0);
    find_or_insert(97, 1, 1);
    find_or_insert(98, 2, 2);
    find_or_insert(99, 3, 3);
    find_or_insert(100, 4, 4);
    find_or_insert(101, 5, 5);
    find_or_insert(98, 2, 6);
    find_or_insert(611, 6, 6);
    find_or_insert(100, 4, 7);
    find_or_insert(1125, 7, 7);
    find_or_insert(97, 1, 8);
    find_or_insert(354, 8, 8);
    find_or_insert(99, 3, 9);

}


TEST(hash, lookup_bug2) {
    auto ch = compact_hash<uint64_t>(0);
    size_t i = 0;
    bool abort = false;
    std::string last;
    auto find_or_insert = [&](auto key, uint64_t existing_value, uint64_t new_value) {
        if (abort) return;

        auto& val = ch[key];
        if (val == 0) {
            val = new_value;
        }
        std::cout << "find_or_insert(" << key << ", " << existing_value << ", " << new_value << ") \t#" << (i++) << "\n";

        if (val != existing_value) {
            abort = true;
            std::cout << "[before]" << "\n";
            std::cout << last << "\n";
            std::cout << "[after]" << "\n";
            std::cout << ch.debug_state() << "\n";
        }
        ASSERT_EQ(val, existing_value);
        last = ch.debug_state();
    };

    find_or_insert(0, 0, 0);
    find_or_insert(97, 1, 1);
    find_or_insert(115, 2, 2);
    find_or_insert(100, 3, 3);
    find_or_insert(102, 4, 4);
    find_or_insert(97, 1, 1);
    find_or_insert(371, 5, 5);
    find_or_insert(99, 6, 6);
    find_or_insert(116, 7, 7);
    find_or_insert(106, 8, 8);
    find_or_insert(107, 9, 9);
    find_or_insert(99, 6, 6);
    find_or_insert(1634, 10, 10);
    find_or_insert(119, 11, 11);
    find_or_insert(101, 12, 12);
    find_or_insert(97, 1, 1);
    find_or_insert(371, 5, 5);
    find_or_insert(1378, 13, 13);
    find_or_insert(101, 12, 12);
    find_or_insert(3170, 14, 14);
    find_or_insert(118, 15, 15);
    find_or_insert(116, 7, 7);
    find_or_insert(1897, 16, 16);
    find_or_insert(119, 11, 11);
    find_or_insert(2917, 17, 17);
    find_or_insert(116, 7, 7);
    find_or_insert(1911, 18, 18);
    find_or_insert(99, 6, 6);
    find_or_insert(1646, 19, 19);
    find_or_insert(98, 20, 20);
    find_or_insert(119, 11, 11);
    find_or_insert(2914, 21, 21);
    find_or_insert(98, 20, 20);
    find_or_insert(5233, 22, 22);
    find_or_insert(110, 23, 23);
    find_or_insert(113, 24, 24);
    find_or_insert(120, 25, 25);
    find_or_insert(101, 12, 12);
    find_or_insert(3186, 26, 26);
    find_or_insert(110, 23, 23);
    find_or_insert(6001, 27, 27);
    find_or_insert(122, 28, 28);
    find_or_insert(101, 12, 12);
    find_or_insert(3194, 29, 29);
    find_or_insert(119, 11, 11);
    find_or_insert(2933, 30, 30);
    find_or_insert(113, 24, 24);
    find_or_insert(6263, 31, 31);
    find_or_insert(101, 12, 12);
    find_or_insert(3194, 29, 29);
    find_or_insert(7541, 32, 32);
    find_or_insert(101, 12, 12);
    find_or_insert(3188, 33, 33);
    find_or_insert(113, 24, 24);
    find_or_insert(6243, 34, 34);
    find_or_insert(114, 35, 35);
    find_or_insert(110, 23, 23);
    find_or_insert(6010, 36, 36);
    find_or_insert(120, 25, 25);
    find_or_insert(6498, 37, 37);
    find_or_insert(110, 23, 23);
    find_or_insert(5989, 38, 38);
    find_or_insert(113, 24, 24);
    find_or_insert(6245, 39, 39);
    find_or_insert(98, 20, 20);
    find_or_insert(5239, 40, 40);
    find_or_insert(99, 6, 6);
    find_or_insert(1634, 10, 10);
    find_or_insert(2673, 41, 41);
    find_or_insert(119, 11, 11);
    find_or_insert(2921, 42, 42);
    find_or_insert(99, 6, 6);
    find_or_insert(1634, 10, 10);
    find_or_insert(2673, 41, 41);
    find_or_insert(10595, 43, 43);
    find_or_insert(98, 20, 20);
    find_or_insert(5236, 44, 44);
    find_or_insert(110, 23, 23);
    find_or_insert(6001, 27, 27);
    find_or_insert(7031, 45, 45);
    find_or_insert(101, 12, 12);
    find_or_insert(3185, 46, 46);
    find_or_insert(120, 25, 25);
    find_or_insert(6499, 47, 47);
    find_or_insert(98, 20, 20);
    find_or_insert(5239, 40, 40);
    find_or_insert(10357, 48, 48);
    find_or_insert(101, 12, 12);
    find_or_insert(3192, 49, 49);
    find_or_insert(99, 6, 6);
    find_or_insert(1634, 10, 10);
    find_or_insert(2682, 50, 50);
    find_or_insert(113, 24, 24);
    find_or_insert(6263, 31, 31);
    find_or_insert(8037, 51, 51);
    find_or_insert(122, 28, 28);
    find_or_insert(7267, 52, 52);
    find_or_insert(113, 24, 24);
    find_or_insert(6242, 53, 53);
    find_or_insert(119, 11, 11);
    find_or_insert(2917, 17, 17);
    find_or_insert(4451, 54, 54);
    find_or_insert(113, 24, 24);
    find_or_insert(6242, 53, 53);
    find_or_insert(13687, 55, 55);
    find_or_insert(100, 3, 3);
    find_or_insert(865, 56, 56);
    find_or_insert(115, 2, 2);
    find_or_insert(627, 57, 57);
    find_or_insert(100, 3, 3);
    find_or_insert(865, 56, 56);
    find_or_insert(14451, 58, 58);
    find_or_insert(100, 3, 3);
    find_or_insert(870, 59, 59);
    find_or_insert(122, 28, 28);
    find_or_insert(7268, 60, 60);
    find_or_insert(102, 4, 4);
    find_or_insert(1127, 61, 61);
    find_or_insert(102, 4, 4);
    find_or_insert(1139, 62, 62);
    find_or_insert(100, 3, 3);
    find_or_insert(870, 59, 59);
    find_or_insert(15219, 63, 63);
    find_or_insert(100, 3, 3);
    find_or_insert(871, 64, 64);
    find_or_insert(102, 4, 4);
    find_or_insert(1124, 65, 65);
    find_or_insert(117, 66, 66);
    find_or_insert(99, 6, 6);
    find_or_insert(1637, 67, 67);
    find_or_insert(122, 28, 28);
    find_or_insert(7267, 52, 52);
    find_or_insert(13428, 68, 68);
    find_or_insert(122, 28, 28);
    find_or_insert(7281, 69, 69);
    find_or_insert(119, 11, 11);
    find_or_insert(2917, 17, 17);
    find_or_insert(4450, 70, 70);
    find_or_insert(99, 6, 6);
    find_or_insert(1652, 71, 71);
    find_or_insert(117, 66, 66);
    find_or_insert(17001, 72, 72);
    find_or_insert(113, 24, 24);
    find_or_insert(6263, 31, 31);
    find_or_insert(8041, 73, 73);
    find_or_insert(105, 74, 74);
    find_or_insert(113, 24, 24);
    find_or_insert(6243, 34, 34);
    find_or_insert(8802, 75, 75);
    find_or_insert(110, 23, 23);
    find_or_insert(6010, 36, 36);
    find_or_insert(9315, 76, 76);
    find_or_insert(101, 12, 12);
    find_or_insert(3170, 14, 14);
    find_or_insert(3706, 77, 77);
    find_or_insert(113, 24, 24);
    find_or_insert(6243, 34, 34);
}
