#pragma once

#include <tudocomp/config.h>

#include <fstream>
#include <iostream>
#include <vector>

#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/compressors/lzss/FactorBuffer.hpp>
#include <tudocomp/compressors/lcpcomp/lcpcomp.hpp>
#include <tudocomp/ds/LCPSada.hpp>

#include <tudocomp_stat/StatPhase.hpp>

#ifdef STXXL_FOUND
#include <stxxl/bits/containers/vector.h>
#endif

namespace tdc {
namespace lcpcomp {

inline size_t filesize( const char*const filepath ){
    std::ifstream file(filepath, std::ios::binary | std::ios::ate | std::ios::in);
    if(!file.good()) return 0;
    return file.tellg();
}

/** Strategy for generating the final factors
 *  plcpcomp adds factors with add_factor, and calls sort
 *  whenever it is sure that the next factors' target text position are always be greater
 *  than all factors currently added. This is a good time to sort the already stored factors.
 *  The method @factorize will finally produce the factors, and store them in @reflist.
 */
template<class sa_t, class isa_t>
class RefRAMStrategy {
    sa_t& m_sa; //TODO: add const
    isa_t& m_isa; //TODO: add const
    std::vector<std::pair<len_t,len_t>> m_factors;
    std::vector<std::pair<len_t,len_t>> m_factors_unsorted;

    public:
    RefRAMStrategy(sa_t& sa, isa_t& isa) : m_sa(sa), m_isa(isa) {

    }
    void sort() { // sort m_factors_unsorted and append the sorted list to m_factors
        std::sort(m_factors_unsorted.begin(), m_factors_unsorted.end());
        // only give a resize hint if we need more space than doubling it, what is the default behavior of a vector
        if(m_factors.capacity()*2 < m_factors.size()+m_factors_unsorted.size()) {
            m_factors.reserve(m_factors.size()+m_factors_unsorted.size());
        }
        std::move(m_factors_unsorted.begin(), m_factors_unsorted.end(), std::inserter(m_factors, m_factors.end()));
        m_factors_unsorted.clear();
    }
    void add_factor(len_t source_position, len_t factor_length) {
        return m_factors_unsorted.emplace_back(source_position,factor_length);
    }
    void factorize(lzss::FactorBufferRAM& reflist) {
        for(len_t i = 0; i < m_factors.size(); ++i) {
            const auto& el = m_factors[i];
            reflist.emplace_back(el.first, m_sa[m_isa[el.first]-1], el.second);
        }
    }
};

#ifdef STXXL_FOUND

/** Same as @RefRAMStrategy, but works with STXXL vectors
 */
template<class phi_t>
class RefDiskStrategy {
    const phi_t& m_phi; // tuples (i, phi[i]) for irreducible positions i only
    stxxl::VECTOR_GENERATOR<std::pair<len_t,len_t>>::result m_factors; // (text_position, factor_length)
    std::vector<std::pair<len_t,len_t>> m_factors_unsorted;

public:
    RefDiskStrategy(const phi_t& phi) : m_phi(phi) {
    }

    void sort() {
        std::sort(m_factors_unsorted.begin(), m_factors_unsorted.end());
        // only give a resize hint if we need more space than doubling it, what is the default behavior of a vector
        if(m_factors.capacity()*2 < m_factors.size()+m_factors_unsorted.size()) {
            m_factors.reserve(m_factors.size()+m_factors_unsorted.size());
        }
        //m_factors.reserve(std::max(m_factors.capacity(), m_factors.size()+m_factors_unsorted.size()));
        for(auto it = m_factors_unsorted.begin(); it != m_factors_unsorted.end(); ++it) {
            m_factors.push_back(*it);
        }
        m_factors_unsorted.clear();
    }

    void add_factor(len_t source_position, len_t factor_length) {
        return m_factors_unsorted.push_back(std::make_pair(source_position,factor_length));
    }

    void factorize(lzss::FactorBufferDisk& reflist) {
        typename phi_t::bufreader_type phi_reader(m_phi);
        auto phi_iterator = phi_reader.begin();
        CHECK_EQ(0ULL, size_t(phi_iterator->first)); // first phi array entry needs to be for i=0

        typename decltype(m_factors)::bufreader_type factor_reader(m_factors);

        len_t irreducible_pos = 0;
        len_t irreducible_phi = phi_iterator->second;
        ++phi_iterator;

        for(auto f = factor_reader.begin(); f != factor_reader.end(); ++f) {
            auto pos = f->first;

            // find phi[pos]
            while(phi_iterator != phi_reader.end() && pos >= phi_iterator->first) {
                irreducible_pos = phi_iterator->first;
                irreducible_phi = phi_iterator->second;
                ++phi_iterator;
            }
            DCHECK_LE(irreducible_pos, pos);
            auto phi_pos = irreducible_phi + (pos - irreducible_pos);

            //DLOG(INFO) << "phi[" << pos << "] = " << phi_pos;
            reflist.push_back(pos, phi_pos, f->second);
        }
    }
};

#endif

/** Iterates over the PLCP-file
 *  this file stores all plcp values unary in the representation of Sadakane using 2n bits for storing n entries
 */
class PLCPFileForwardIterator {
    std::ifstream m_is;

    uint64_t m_chunk = 0; // current data chunk
    len_t m_idx = 0; // current select parameter
    len_t m_block = 0; // block index
    len_t m_blockrank = 0; //number of ones up to previous block
    uint_fast8_t m_ones; // number of ones in the current block `m_block`

    void read_chunk() {
        m_is.read(reinterpret_cast<char*>(&m_chunk), sizeof(decltype(m_chunk)));
        m_ones = sdsl::bits::cnt(m_chunk);
    }

    public:
    static constexpr const len_t eof = -1;
    PLCPFileForwardIterator(const char* filepath)
        : m_is(filepath)
    {
        read_chunk();
    }

    len_t index() const { return m_idx; }
    bool has_next() const {
        return !m_is.fail();
    }

    len_t next_select() {
        while(m_blockrank+m_ones < m_idx+1) {
            if(!m_is) {break;}
            ++m_block;
            m_blockrank += m_ones;
            read_chunk();
        }
        DCHECK_GE(m_blockrank+m_ones, m_idx+1);
        return 64*m_block + sdsl::bits::sel(m_chunk, m_idx+1-m_blockrank);
    }
    len_t operator()() {
        const len_t ret = next_select() - 2*m_idx;
        return ret;
    }
    void advance() {
        ++m_idx;
    }
};


/**
 * The actual PLCPcomp compressor: it searches for peaks, replaces them and adds the replacements to the RefStrategy
 * @tparam RefStrategy container class that stores the factors (source,len). This class has to reconstruct the target.
 */
template<class RefStrategy,class plcp_type>
void compute_references(const size_t n, RefStrategy& refStrategy, plcp_type& pplcp, size_t threshold) {
    // Point of interest
    struct Poi {
        len_t pos;
        len_t lcp;

        Poi(len_t _pos, len_t _lcp) : pos(_pos), lcp(_lcp) {
        }

        inline len_t end() const {
            return pos + lcp;
        }

        inline bool operator<(const Poi& o) const {
            DCHECK_NE(o.pos, this->pos);
            if(o.lcp == this->lcp) return this->pos > o.pos;
            return this->lcp < o.lcp;
        }

        inline bool operator==(const Poi& o) const {
            return (o.pos == this->pos);
        }

        inline bool operator!=(const Poi& o) const {
            return !(o == *this);
        }

        inline operator bool() const {
            return (this->lcp > 0);
        }

        inline void clear() {
            this->lcp = 0;
        }

        inline std::string str() const {
            std::stringstream ss;
            ss << '(' << pos << ',' << lcp << ')';
            return ss.str();
        }
    };

    std::vector<Poi> pois;
    IF_STATS(size_t max_array_size = 0);
    IF_STATS(size_t num_factors = 0);

    // process text
    for(len_t i = 0; i+1 < n; ++i) {
        DCHECK_EQ(pplcp.index(), i);

        const len_t plcp_i = pplcp();
        if(!pois.empty()) {
            const auto& last = pois.back();
            if(i - last.pos >= last.lcp || tdc_unlikely(i+1 == n)) {
                DCHECK_EQ(i - last.pos, last.lcp);

                // no new peak after last - factorize!
                IF_STATS(max_array_size = std::max(max_array_size, pois.size()));

                Poi* current = &pois.back(); // last is initially the highest
                IF_DEBUG(
                    // ensure current is actually a maximal peak
                    for(const auto& poi : pois) {
                        DCHECK_LE(poi.lcp, current->lcp);
                    }
                )

                while(current) {
                    // look at surrounding peaks
                    Poi* next = nullptr;

                    len_t rightpeak_lcp = 0;
                    bool rightpeak_exists = false;

                    for(auto& poi : pois) {
                        if(poi == *current) continue; // all except current
                        if(!poi) continue; // already obsolete

                        if(poi.pos < current->pos) {
                            // left of current
                            if(poi.end() > current->pos) {
                                // left overlap!
                                len_t d = poi.end() - current->pos;

                                // shorten POI by overlap length or remove
                                len_t newlcp = poi.lcp - d;
                                if(newlcp >= threshold) {
                                    // shorten
                                    poi.lcp = newlcp;
                                } else {
                                    // too short - remove
                                    poi.clear();
                                }
                            }
                        } else if(poi.pos > current->pos) {
                            // right of current
                            if(current->end() > poi.pos) {
                                // right overlap!
                                len_t d = current->end() - poi.pos;

                                // candidate for new peak right of current
                                rightpeak_lcp = std::max(rightpeak_lcp, poi.lcp - d);

                                // remove
                                poi.clear();
                            } else if(current->end() == poi.pos) {
                                // there's a peak to the right
                                rightpeak_exists = true;
                            }
                        } else {
                            DCHECK(false) << "shouldn't happen";
                        }

                        if(poi) {
                            // if still intact, test if maximal
                            if(!next || poi.lcp > next->lcp) {
                                next = &poi;
                            }
                        }
                    }

                    // introduce factor
                    ++num_factors;
                    refStrategy.add_factor(current->pos, current->lcp);

                    if(!rightpeak_exists && rightpeak_lcp >= threshold) {
                        // re-use current as new peak to the right
                        current->pos = current->end();
                        current->lcp = rightpeak_lcp;

                        // potentially highest peak
                        if(!next || rightpeak_lcp > next->lcp) {
                            next = current;
                        }
                    } else {
                        // no peak to the right - remove current
                        current->clear();
                    }

                    // select next
                    current = next;
                }

                // the found references can now be sorted and appended to the already sorted list of references (appending preserves the ordering)
                refStrategy.sort(); // EM: sort in RAM and then write to disk

                // restart looking from here
                pois.clear();
            } else {
                if(plcp_i > last.lcp) {
                    // higher peak than last
                    pois.emplace_back(i, plcp_i);
                }
            }
        }

        if(pois.empty()) {
            if(plcp_i >= threshold) {
                // first POI of a chunk
                pois.emplace_back(i, plcp_i);
            }
        }

        pplcp.advance();
    }

    IF_STATS(StatPhase::log("max_array_size", max_array_size));
    IF_STATS(StatPhase::log("num_factors", num_factors));
}

/// A very naive selection strategy for LCPComp.
///
/// TODO: Describe
class PLCPStrategy : public Algorithm {
public:
    using Algorithm::Algorithm;

    inline static Meta meta() {
        Meta m(comp_strategy_type(), "plcp", "Uses the PLCP array");
        return m;
    }

    inline static ds::dsflags_t textds_flags() {
        return ds::SA | ds::ISA;
    }

    /**
     *  Called by the LCPcompCompressor.
     *  The compressor works in RAM mode, so this method produces the factors in RAM.
     */
    template<typename text_t, typename factorbuffer_t>
    inline void factorize(text_t& text, size_t threshold, factorbuffer_t& refs) {
        StatPhase phase("Load Index DS");
        text.require(text_t::SA | text_t::ISA);

        const auto& sa = text.require_sa();
        const auto& isa = text.require_isa();
        //RefDiskStrategy<decltype(sa),decltype(isa)> refStrategy(sa,isa);
        RefRAMStrategy<decltype(sa),decltype(isa)> refStrategy(sa,isa);
        LCPForwardIterator pplcp { (construct_plcp_bitvector(sa, text)) };
        phase.split("Compute References");
        compute_references(text.size(), refStrategy, pplcp, threshold);
        phase.split("Factorize");
        refStrategy.factorize(refs);
    }
};

}}//ns

