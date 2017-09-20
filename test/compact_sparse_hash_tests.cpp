#pragma once

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

    std::cout << "=======================\n";
    std::cout << ch.debug_state() << "\n";
}

TEST(hash, insert_wrap) {
    Init::reset();

    auto ch = compact_hash<Init>(4);
    ch.insert(3, Init());
    ch.insert(7, Init());
    ch.insert(15, Init());

    std::cout << "=======================\n";
    std::cout << ch.debug_state() << "\n";
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

    std::cout << "=======================\n";
    std::cout << ch.debug_state() << "\n";
    std::cout << "=======================\n";

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

    std::cout << "=======================\n";
    std::cout << ch.debug_state() << "\n";
    std::cout << "=======================\n";
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

    std::cout << "=======================\n";
    std::cout << ch.debug_state() << "\n";
    std::cout << "=======================\n";

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

    std::cout << "=======================\n";
    std::cout << ch.debug_state() << "\n";
    std::cout << "=======================\n";

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

        DCHECK_EQ(ch.at(key, bits), v1);
        inserted.clear();
        inserted.push_back({ key, std::move(v1) });
        for (auto& kv : inserted) {
            ch.debug_check_single(kv.first, kv.second);
        }
    };


    for(size_t i = 0; i < 10000; i++) {
        add(i*13ull, Init(i));
    }
}
