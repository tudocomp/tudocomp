#pragma once

#include <tudocomp/config.h>

#include <algorithm>
#include <functional>
#include <vector>
#include <tudocomp/def.hpp>

#include <tudocomp/compressors/lzss/Factor.hpp>

#include <tudocomp/util.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp_stat/StatPhase.hpp>

#ifdef STXXL_FOUND
#include <stxxl/vector>
#endif

namespace tdc {
namespace lzss {

template<typename vector_type = std::vector<Factor>>
class FactorBuffer {
public:
    using backing_vector_type = vector_type;
    using const_iterator = typename vector_type::const_iterator;

private:
    vector_type m_factors;
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

    inline void push_back(Factor f) {
        // TODO: specialize for FactorBufferDisk by using a writer?
        m_sorted = m_sorted && (m_factors.empty() || f.pos >= m_factors.back().pos);
        m_factors.push_back(f);

        m_shortest_factor = std::min(m_shortest_factor, len_t(f.len));
        m_longest_factor = std::max(m_longest_factor, len_t(f.len));
    }

    inline void push_back(len_t fpos, len_t fsrc, len_t flen) {
        push_back(Factor{fpos,fsrc,flen});
    }

    inline void emplace_back(len_t fpos, len_t fsrc, len_t flen) {
        // FIXME: just a transitional alias
        push_back(Factor{fpos,fsrc,flen});
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

    template<typename text_t, typename encoder_t>
    inline void encode_text(const text_t& text, encoder_t& encoder) const {
        CHECK(m_sorted)
            << "factors need to be sorted before they can be encoded";

        // walk over text
        encoder.encode_header();

        size_t p = 0;
        for(auto& f : m_factors) {
            encoder.encode_run(text, p, f.pos);
            encoder.encode_factor(f);

            p = f.pos + f.len;
        }
        encoder.encode_run(text, p, text.size());
    }

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

    inline Range factor_length_range() const {
        return Range(m_shortest_factor, m_longest_factor);
    }
};

using FactorBufferRAM = FactorBuffer<std::vector<Factor>>;

#ifdef STXXL_FOUND
using FactorBufferDisk = FactorBuffer<stxxl::VECTOR_GENERATOR<Factor>::result>;
#endif

}} //ns

