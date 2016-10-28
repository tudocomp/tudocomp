#include <string>
#include <vector>

#include <benchpress/benchpress.hpp>

#include <tudocomp/ds/uint_t.hpp>

using namespace benchpress;

const uint32_t N_SUB_ITER = 10000;

#define ADD x += 1;
#define ADD2 ADD ADD
#define ADD3 ADD2 ADD2
#define ADD4 ADD3 ADD3
#define ADD5 ADD4 ADD4
#define ADD6 ADD5 ADD5
#define ADD7 ADD6 ADD6
#define ADD8 ADD7 ADD7
#define ADD9 ADD8 ADD8
#define ADD10 ADD9 ADD9
#define ADD11 ADD10 ADD10
#define ADD12 ADD11 ADD11
#define ADD13 ADD12 ADD12

struct Inc {
    template<class T>
    static void op(T& x) {
        x += 1;
    }
};

struct NoOp {
    template<class T>
    static void op(T& x) {
    }
};

template<class T, class Op = Inc>
inline void int_test(benchpress::context* ctx) {
    T n = 0;

    for (size_t i = 0; i < ctx->num_iterations(); ++i) {
        escape(&n);

        for (size_t j = 0; j < N_SUB_ITER; j++) {
            Op::op(n);
        }
    }

    escape(&n);
}

BENCHMARK("inc::empty", (int_test<uint32_t, NoOp>))
BENCHMARK("inc::uint8_t", int_test<uint8_t>)
BENCHMARK("inc::uint16_t", int_test<uint16_t>)
BENCHMARK("inc::uint32_t", int_test<uint32_t>)
BENCHMARK("inc::uint64_t", int_test<uint64_t>)
BENCHMARK("inc::uint_t<1>", int_test<uint_t<1>>)
BENCHMARK("inc::uint_t<8>", int_test<uint_t<8>>)
BENCHMARK("inc::uint_t<16>", int_test<uint_t<16>>)
BENCHMARK("inc::uint_t<24>", int_test<uint_t<24>>)
BENCHMARK("inc::uint_t<32>", int_test<uint_t<32>>)
BENCHMARK("inc::uint_t<40>", int_test<uint_t<40>>)
BENCHMARK("inc::uint_t<64>", int_test<uint_t<64>>)

BENCHMARK("ref::uint32_t&", [](benchpress::context* ctx) {
    uint32_t n = 0;

    for (size_t i = 0; i < ctx->num_iterations(); ++i) {
        escape(&n);
        auto& x = n;
        for (size_t j = 0; j < N_SUB_ITER; j++) {
            x += 1;
        }
    }

    escape(&n);
})

BENCHMARK("ref::uint_t<40>&", [](benchpress::context* ctx) {
    uint_t<40> n = 0;

    for (size_t i = 0; i < ctx->num_iterations(); ++i) {
        escape(&n);
        auto& x = n;
        for (size_t j = 0; j < N_SUB_ITER; j++) {
            x += 1;
        }
    }

    escape(&n);
})
