#pragma once

#include <algorithm>
#include <vector>
#include <tudocomp/def.hpp>

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp_stat/StatPhase.hpp>
#include <stxxl/vector>

namespace tdc {
namespace lzss {

class Factor {
public:
    len_compact_t pos, src, len;

    inline Factor() {} // needed by STXXL to create Factor's

    inline Factor(len_t fpos, len_t fsrc, len_t flen)
        : pos(fpos), src(fsrc), len(flen) {
    }
}  __attribute__((__packed__));



template<class vector_type = std::vector<Factor>> // or vector_type =  stxxl::VECTOR_GENERATOR<Factor>::result
class FactorBuffer {
public:
    using backing_vector_type = vector_type;
    typedef typename vector_type::const_iterator const_iterator;
private:

    vector_type m_factors;
//    std::vector<Factor> m_factors;
    bool m_sorted; //! factors need to be sorted before they are output

    len_t m_shortest_factor;
    len_t m_longest_factor;

public:

    inline FactorBuffer()
        : m_sorted(true)
        , m_shortest_factor(INDEX_MAX)
        , m_longest_factor(0)
    {
    }

    vector_type& factors = m_factors;

    inline void emplace_back(len_t fpos, len_t fsrc, len_t flen) {
        m_sorted = m_sorted && (m_factors.empty() || fpos >= m_factors.back().pos);
        m_factors.push_back(Factor { fpos, fsrc, flen });

        m_shortest_factor = std::min(m_shortest_factor, flen);
        m_longest_factor = std::max(m_longest_factor, flen);
    }
    
    inline void push_back(Factor &factor) {
        m_sorted = m_sorted && (m_factors.empty() || factor.pos >= m_factors.back().pos);
        m_factors.push_back(factor);

        m_shortest_factor = std::min(m_shortest_factor, len_t(factor.len));
        m_longest_factor = std::max(m_longest_factor, len_t(factor.len));
    }
    
    inline const_iterator begin() const {
        return m_factors.cbegin();
    }
    inline const_iterator end() const {
        return m_factors.cend();
    }

    inline bool empty() const {
        return m_factors.empty();
    }

    inline size_t size() const {
        return m_factors.size();
    }

    inline bool is_sorted() const {
        return m_sorted;
    }

    inline void sort() { //TODO: use radix sort
        if(!m_sorted) { // TODO: exchange when using stxxl
            std::sort(m_factors.begin(), m_factors.end(),
                [](const Factor& a, const Factor& b) -> bool { return a.pos < b.pos; });

            m_sorted = true;
        }
    }

public:
    inline void flatten() {
        if(m_factors.empty()) return; //nothing to do

        CHECK(m_sorted)
            << "factors need to be sorted before they can be flattened";

        // create pos -> factor map
        auto& last = m_factors.back();
        DynamicIntVector fmap(
            last.pos + last.len,
            0,
            bits_for(m_factors.size() + 1));

        for(size_t i = 0; i < m_factors.size(); i++) {
            auto& f = m_factors[i];
            for(size_t j = 0; j < f.len; j++) {
                fmap[f.pos + j] = i + 1;
            }
        }

        // process factors
        size_t num_flattened = 0;
        size_t max_depth = 0;
        for(auto& f : m_factors) {
            size_t depth = 0;

            size_t src = f.src;
            while(src < fmap.size() && fmap[src]) {
                auto& s = m_factors[fmap[src] - 1];

                size_t d = src - s.pos;
                if((s.src + d + f.len) <= (s.src + s.len)) {
                    src = s.src + d;

                    //FIXME: actually, one would have to add the flatten
                    //       depth of the referred factors recursively,
                    //       so the depth here is only a lower bound
                    ++depth;
                } else {
                    break;
                }
            }

            if(depth) {
                f.src = src;

                ++num_flattened;
                max_depth = std::max(max_depth, depth);
            }
        }

        StatPhase::log("num_flattened", num_flattened);
        StatPhase::log("max_depth_lb", max_depth);
    }

    inline size_t shortest_factor() const {
        return m_shortest_factor;
    }

    inline size_t longest_factor() const {
        return m_longest_factor;
    }

};

typedef FactorBuffer<std::vector<Factor>> FactorBufferRAM;
typedef FactorBuffer<stxxl::VECTOR_GENERATOR<Factor>::result> FactorBufferDisk;

}} //ns

