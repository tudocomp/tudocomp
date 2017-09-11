#pragma once

#include <unordered_set>

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/compressors/esp/TypedBlock.hpp>

namespace tdc {
namespace esp {
    //[debugging]///////////////////////////////////////////////////////////////

    std::ostream& nice_block_lengths(GenericView<TypedBlock> tbs, std::ostream& o) {
        for (auto& tb: tbs) {
            if (tb.len == 1) {
                o << "[" << int(tb.type) << "]";
            } else if (tb.len == 2) {
                o << "[  " << int(tb.type) << " ]";
            } else if (tb.len == 3) {
                o << "[   " << int(tb.type) << "   ]";
            } else {
                o << "<Err: " << tb <<  ">";
            }
        }
        return o;
    }
    template<typename T>
    class DebugPrint {
        ConstGenericView<T> m_view;
        size_t m_alpha;
    public:
        inline DebugPrint(ConstGenericView<T> v, size_t alpha):
            m_view(v), m_alpha(alpha) {}
        template<typename U>
        friend std::ostream& operator<<(std::ostream&, const DebugPrint<U>&);
        size_t char_mult() {
            if (m_alpha == 256) {
                return 1;
            } else {
                return 4;
            }
        }
    };
    template<typename T>
    inline std::ostream& operator<<(std::ostream& o, const DebugPrint<T>& d) {
        if (d.m_alpha == 256) {
            o << "[";
            for (auto c: d.m_view) {
                o << char(uint8_t(c));
            }
            o << "]";
            return o;
        } else {
            return o << vec_to_debug_string(d.m_view, 2);
        }
    }
    template<typename T>
    DebugPrint<T> debug_p(ConstGenericView<T> v, size_t alpha) {
        return DebugPrint<T>(v, alpha);
    }

    //[end debugging]///////////////////////////////////////////////////////////

    template<class T>
    uint64_t calc_alphabet_size(const T& t) {
        // TODO
        // Optimize for alphabets with large repeating numbers
        std::unordered_set<size_t> alpha;
        for (const auto& v : t) {
            alpha.insert(v);
        }
        return alpha.size();
    }

    template<class T>
    bool no_adjacent_identical(const T& t) {
        for(size_t i = 1; i < t.size(); i++) {
            if (t[i] == t[i - 1]) return false;
        }
        return true;
    }

    template<class T, class F>
    void do_for_neighbors(T& t, F f) {
        for (size_t i = 0; i < t.size(); i++) {
            std::array<typename T::value_type, 2> neighbors;
            uint8_t neighbor_len = 0;

            if (i == 0 && i == t.size() - 1) {
                neighbor_len = 0;
            } else if (i == 0) {
                neighbor_len = 1;
                neighbors[0] = t[i + 1];
            } else if (i == t.size() - 1) {
                neighbor_len = 1;
                neighbors[0] = t[i - 1];
            } else {
                neighbor_len = 2;
                neighbors[0] = t[i - 1];
                neighbors[1] = t[i + 1];
            }

            f(i, ConstGenericView<typename T::value_type>(neighbors.data(), neighbor_len));
        }
    }
}
}
