#include "tudocomp/tudocomp.hpp"
#include "tudocomp/compressors/EspCompressor.hpp"
#include "test/util.hpp"
#include <gtest/gtest.h>

#include <tudocomp/compressors/esp/meta_blocks.hpp>

using namespace tdc;

TEST(ESP, test) {
    std::vector<uint8_t> small_alpha = "abcabageheadbag"_v;
    //for (auto& e : small_alpha) { e -= 'a'; }

    // TODO: factor out recution code to be paramtric over
    // alphabet size

    std::vector<View> cases {
        "0000dkasxxxcsdacjzsbkhvfaghskcbsaaaaaaaaaaaaaaaaaadkcbgasdbkjcbackscfa",
        "bananaoobananaoobananaoobananaoo",
        "aaaaa",
        "asdf",
        "aaaxaaa",
        //"",
        "a",
        "aa",
        "aaa",
        "aaaa",
        "aaaaa",
        "aaaaaa",
        "a",
        "as",
        "asd",
        "asdf",
        "asdfg",
        "asdfgh",
        small_alpha,
    };

    for (auto& c : cases) {
        Input i(c);
        std::vector<uint8_t> v;
        Output o(v);

        auto comp = tdc::create_algo<EspCompressor>();

        comp.compress(i, o);

        std::cout << "\n";
    }
}


struct Empty {};

std::ostream& operator<<(std::ostream& os, Empty e) {
    return os << "Empty";
}

template<class Prev, class T, class F>
struct Context;

template<class Self, class T>
struct ContextBase {
    std::deque<T> buffer;
    size_t offset = -1;
    size_t min_index = -1;
    size_t debug_depth = 0;

    template<class G>
    auto stage(G g) -> Context<Self, decltype(g((Self&) *this, 0)), G> {
        auto r = Context<Self, decltype(g((Self&) *this, 0)), G> {
            (Self&) *this,
            g,
        };

        r.debug_depth = debug_depth + 1;

        return r;
    }

    void print_debug() {
        std::cout << "@" << debug_depth << " buffer size: " << buffer.size() << "\n";
    }
};

template<class Prev, class T, class F>
struct Context: ContextBase<Context<Prev, T, F>, T> {
    Prev prev;
    F f;

    Context(Prev prev_, F f_): prev(prev_), f(f_) {}

    T unbuffered_at(size_t i) {
        auto& cx = prev;
        cx.min_index = -1;
        auto r = f(cx, i);

        /*
        auto debug_old_size = cx.buffer.size();
        auto debug_old_offset = cx.offset;
        auto debug_old_min_index = cx.min_index;
        */

        if (cx.min_index != size_t(-1)) {
            for (size_t i = 0; i < (cx.min_index - cx.offset); i++) {
                cx.buffer.pop_front();
            }
            cx.offset += (cx.min_index - cx.offset);
        }

        /*
        std::cout
            << std::string(this->debug_depth * 4, ' ')
            << "[@" << i << ": "
            << "len: (" << debug_old_size
                << " -> " << cx.buffer.size()
            << "), offset: (" << (debug_old_offset == size_t(-1) ? -1 : int(debug_old_offset))
                << " -> " << (cx.offset == size_t(-1) ? -1 : int(cx.offset))
            << "), min_index: (" << (debug_old_min_index == size_t(-1) ? -1 : int(debug_old_min_index))
                << " -> " << (cx.min_index == size_t(-1) ? -1 : int(cx.min_index))
            << ")]\n";
        */

        return r;
    }

    T operator[](size_t i) {
        this->min_index = std::min(this->min_index, i);
        if (this->offset == size_t(-1)) {
            this->offset = i;
        }
        ptrdiff_t lokal_i = ptrdiff_t (i) - ptrdiff_t (this->offset);

        if (lokal_i < 0) {
            throw std::runtime_error("to bad");
        }

        while (size_t(lokal_i) >= this->buffer.size()) {
            this->buffer.push_back(unbuffered_at(this->buffer.size() + this->offset));
        }

        return this->buffer[lokal_i];
    }

    void print_debug() {
        prev.print_debug();
        ContextBase<Context<Prev, T, F>, T>::print_debug();
    }
};

struct InitialContext: ContextBase<InitialContext, Empty> {
    InitialContext() {}
};

template<class Prev, class T, class F>
struct VectorElementContext: ContextBase<VectorElementContext<Prev, T, F>, T> {
    std::shared_ptr<F> f;
    std::shared_ptr<Prev> first_prev;
    std::shared_ptr<VectorElementContext<Prev, T, F>> other_prev;
    size_t vector_index;
};

template<class Prev, class T, class F>
struct VectorContext: ContextBase<VectorContext<Prev, T, F>, T> {
    std::shared_ptr<Prev> prev;
    std::shared_ptr<F> f;
    std::vector<VectorElementContext<Prev, T, F>> elements;
};

TEST(OffsetOp, test) {
    std::vector<uint8_t> v { 1, 5, 7, 3, 5, 2, 9, 0 };

    //////////////////////////////////////////

    using T0 = InitialContext;
    auto s0 = [&](T0& c, size_t i) -> uint32_t {
        return v.at(i);
    };

    //////////////////////////////////////////

    using T1 = Context<T0, uint32_t, decltype(s0)>;
    auto s1 = [&](T1& c, size_t j) -> uint32_t {
        auto x = c[j - 1];
        auto y = c[j];
        auto z = c[j + 1];

        std::cout << int(x) << ", " << int(y) << ", " << int(z) << "\n";

        return x * 100 + y * 10 + z;
    };

    //////////////////////////////////////////

    using T2 = Context<T1, uint32_t, decltype(s1)>;
    auto s2 = [&](T2& c, size_t k) -> uint32_t {
        auto a = c[k];
        auto b = c[k + 1];

        std::cout << int(a) << ", " << int(b) << "\n";

        return a * 1000 + b;
    };

    //////////////////////////////////////////

    auto chain = InitialContext().stage(s0).stage(s1).stage(s2);

    //////////////////////////////////////////

    for (size_t k = 1; k < v.size() - 2; k++) {
        std::cout << chain.unbuffered_at(k) << "\n";
    }

    //////////////////////////////////////////

    std::cout << "\n";
    chain.print_debug();

}

TEST(Esp, new_split) {
    auto s {
        "0000dkasxxxcsdacjzsbkhvfaghskcbsaaaaaaaaaaaaaaaaaadkcbgasdbkjcbackscfa"_v
    };

    {
        std::vector<size_t> block_sizes;
        size_t bsi = 0;
        auto sb = s;

        std::vector<size_t> scratchpad;
        esp::Context ctx {
            256,
            &scratchpad
        };

        std::cout << "             [" << s << "]\n";

        while (sb.size() > 0) {
            auto old_bs = block_sizes.size();
            esp::split(sb, block_sizes, ctx);
            auto new_count = block_sizes.size() - old_bs;
            DCHECK_NE(new_count, 0);
            for (size_t i = 0; i < new_count; i++) {
                auto l = block_sizes[bsi];
                auto front_cut = sb.substr(0, l);
                auto back_cut = sb.substr(l);

                auto n = s.size() - (back_cut.size() + front_cut.size());

                std::cout << "block: " << l << ", ";
                std::cout << std::setw(n) << "";
                std::cout << "[" << front_cut;
                std::cout << "] [" << back_cut;
                std::cout << "]\n";

                sb = back_cut;
                bsi++;
                DCHECK_GE(l, 2);
                DCHECK_LE(l, 3);
            }
        }
    }
}
