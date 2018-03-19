#pragma once
#include <tudocomp/config.h>

#include <vector>

#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/ds/LCPSada.hpp>
#include <boost/heap/pairing_heap.hpp>
//#include <stxxl/bits/stream/sort_stream.h>
#include <stxxl/bits/containers/vector.h>
#include <stxxl/bits/algo/ksort.h>


#include <iostream>
#include <fstream>


#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lcpcomp {


inline size_t filesize( const char*const filepath ){
	std::ifstream file(filepath, std::ios::binary | std::ios::ate | std::ios::in);
	if(!file.good()) return 0;
	return file.tellg();
}

//! use read/seek to emulate an array on a file
template<class int_t>
class IntegerFileArray {
	const size_t m_size;
	std::ifstream m_is;
	public:
	IntegerFileArray(const char*const filename)
	: m_size { filesize(filename) }
	, m_is {filename, std::ios::binary | std::ios::in }
	{}
	int_t operator[](size_t i) {
		DCHECK_LT(i, size());
		m_is.seekg(i*sizeof(int_t), std::ios_base::beg);
		char buf[sizeof(int_t)];
		m_is.read(buf, sizeof(int_t));
		return *reinterpret_cast<int_t*>(buf);
	}
	size_t size() const { return m_size/sizeof(int_t); }
};

//! wrapper to stream a file as an array of integers/characters
template<class int_t>
class IntegerFileForwardIterator {
	const size_t m_size;
	std::ifstream m_is;
	size_t m_index;
	char m_buf[sizeof(int_t)];
	public:

	IntegerFileForwardIterator(const char*const filename)
	: m_size { filesize(filename) }
	, m_is {filename, std::ios::binary | std::ios::in }
	, m_index {0}
	{}

	size_t size() const { return m_size/sizeof(int_t); }
	size_t index() const { return m_index; }
	int_t operator*() { return *reinterpret_cast<int_t*>(m_buf); }
	int_t operator()() { return *reinterpret_cast<int_t*>(m_buf); }
	void advance() {
		(*this)++;
	}
	IntegerFileForwardIterator& operator++(int) {
		m_is.read(m_buf, sizeof(int_t));
		++m_index;
		return *this;
	}
};

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



/** Same as @RefRAMStrategy, but works with STXXL vectors
 */
template<class sa_t, class isa_t>
class RefDiskStrategy {
	sa_t& m_sa;
	isa_t& m_isa;
    len_t m_memory;
    stxxl::VECTOR_GENERATOR<std::pair<len_t,len_t>>::result m_factors; // (text_position, factor_length)
	std::vector<std::pair<len_t,len_t>> m_factors_unsorted;

	public:
	RefDiskStrategy(sa_t& sa, isa_t& isa, len_t memory = 512*1024*1024) : m_sa(sa), m_isa(isa), m_memory(memory) {

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

		struct KeyExtractor {
			typedef len_t key_type;
			typedef std::pair<len_t,len_t> value_type;
			key_type m_key;

			KeyExtractor() {}
			KeyExtractor(const key_type& k) : m_key(k) {}
			key_type operator()(const value_type& v) const { return v.first; }
			value_type min_value() const { return std::make_pair(0,0); }
			//value_type max_value() const { return std::make_pair(std::numeric_limits<key_type>::max(),0); }
			value_type max_value() const { return std::make_pair(m_key,0); }
		};



		StatPhase sorting_phase("Sort by ISA");

		stxxl::VECTOR_GENERATOR<std::pair<len_t,len_t>>::result m_sources;
		StatPhase::wrap("Put factors", [&]{
            typename decltype(m_factors)::bufreader_type reader(m_factors);
            decltype(m_sources)::bufwriter_type writer(m_sources);
			for(auto it = reader.begin(); it != reader.end(); ++it) {
				//m_sources.push_back(std::make_pair(m_isa[it->first]-1, it->first));
                writer << std::make_pair(m_isa[it->first]-1, it->first);
			}
		});
		StatPhase::wrap("Sort factors", [&]{
			stxxl::ksort(m_sources.begin(), m_sources.end(), KeyExtractor(m_sa.size()),m_memory); //, STXXL_DEFAULT_ALLOC_STRATEGY());
		});
		sorting_phase.split("Sort by SA");
		stxxl::VECTOR_GENERATOR<std::pair<len_t,len_t>>::result m_sources2;
		StatPhase::wrap("Put factors", [&]{
            typename decltype(m_sources)::bufreader_type reader(m_sources);
            decltype(m_sources2)::bufwriter_type writer(m_sources2);
		    for(auto it = reader.begin(); it != reader.end(); ++it) {
			    //m_sources2.push_back(std::make_pair(it->second, m_sa[it->first]));
                writer << std::make_pair(it->second, m_sa[it->first]);
		    }
    		m_sources.clear();
		});
		StatPhase::wrap("Sort factors", [&]{
		stxxl::ksort(m_sources2.begin(), m_sources2.end(), KeyExtractor(m_sa.size()),m_memory); //, STXXL_DEFAULT_ALLOC_STRATEGY());
		});

		sorting_phase.split("Write factors to buffer");

        {
        lzss::FactorBufferDisk::backing_vector_type::bufwriter_type writer(reflist.factors);
		for(size_t i = 0; i < m_factors.size(); ++i) {
		 	//reflist.emplace_back(m_factors[i].first, m_sources2[i].second, m_factors[i].second);
            writer << lzss::Factor { m_factors[i].first, m_sources2[i].second, m_factors[i].second };
		}
        }

		// for(auto it = m_factors.begin(); it != m_factors.end(); ++it) {
		// 	const auto& el = *it;
		// 	reflist.emplace_back(el.first, m_sa[m_isa[el.first]-1], el.second);
		// }

	}
};

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


		struct Poi {
			len_t pos;
			len_t lcp;
			len_t no;
			Poi(len_t _pos, len_t _lcp, len_t _no) : pos(_pos), lcp(_lcp), no(_no) {}
			bool operator<(const Poi& o) const {
				DCHECK_NE(o.pos, this->pos);
				if(o.lcp == this->lcp) return this->pos > o.pos;
				return this->lcp < o.lcp;
			}
		};

		boost::heap::pairing_heap<Poi> heap;
		std::vector<typename boost::heap::pairing_heap<Poi>::handle_type> handles;

		IF_STATS(len_t max_heap_size = 0);

		// std::stack<poi> pois; // text positions of interest, i.e., starting positions of factors we want to replace

		len_t lastpos = 0;
		len_t lastpos_lcp = 0;
		for(len_t i = 0; i+1 < n; ++i) {
			while(pplcp.index() < i) pplcp.advance();
			const len_t plcp_i = pplcp(); DCHECK_EQ(pplcp.index(), i);
			if(heap.empty()) {
				if(plcp_i >= threshold) {
					handles.emplace_back(heap.emplace(i, plcp_i, handles.size()));
					lastpos = i;
					lastpos_lcp = plcp_i;
				}
				continue;
			}
			if(i - lastpos >= lastpos_lcp || tdc_unlikely(i+1 == n)) {
				IF_DEBUG(bool first = true);
				IF_STATS(max_heap_size = std::max<len_t>(max_heap_size, heap.size()));
				DCHECK_EQ(heap.size(), handles.size());
				while(!heap.empty()) {
					const Poi& top = heap.top();
					refStrategy.add_factor(top.pos, top.lcp);
					const len_t next_pos = top.pos; // store top, this is the current position that gets factorized
					IF_DEBUG(if(first) DCHECK_EQ(top.pos, lastpos); first = false;)

					{
						len_t newlcp_peak = 0; // a new peak can emerge at top.pos+top.lcp
						bool peak_exists = false;
						if(top.pos+top.lcp < i)
						for(len_t j = top.no+1; j < handles.size(); ++j) { // erase all right peaks that got substituted
							if( handles[j].node_ == nullptr) continue;
							const Poi poi = *(handles[j]);
							DCHECK_LT(next_pos, poi.pos);
							if(poi.pos < next_pos+top.lcp) {
								heap.erase(handles[j]);
								handles[j].node_ = nullptr;
								if(poi.lcp + poi.pos > next_pos+top.lcp) {
									const len_t remaining_lcp = poi.lcp+poi.pos - (next_pos+top.lcp);
									DCHECK_NE(remaining_lcp,0);
									if(newlcp_peak != 0) DCHECK_LE(remaining_lcp, newlcp_peak);
									newlcp_peak = std::max(remaining_lcp, newlcp_peak);
								}
							} else if( poi.pos == next_pos+top.lcp) { peak_exists=true; }
							else { break; }  // only for performance
						}
#ifdef DEBUG
						if(peak_exists) {
							for(len_t j = top.no+1; j < handles.size(); ++j) {
								if( handles[j].node_ == nullptr) continue;
								const Poi& poi = *(handles[j]);
								if(poi.pos == next_pos+top.lcp) {
									DCHECK_LE(newlcp_peak, poi.lcp);
									break;
								}
							}
						}
#endif
						if(!peak_exists && newlcp_peak >= threshold) {
							len_t j = top.no+1;
							DCHECK(handles[j].node_ == nullptr);
							handles[j] = heap.emplace(next_pos+top.lcp, newlcp_peak, j);
						}

					}
					handles[top.no].node_ = nullptr;
					heap.pop(); // top now gets erased

					for(auto it = handles.rbegin(); it != handles.rend(); ++it) {
						if( (*it).node_ == nullptr) continue;
						Poi& poi = (*(*it));
						if(poi.pos > next_pos)  continue;
						const len_t newlcp = next_pos - poi.pos;
						if(newlcp < poi.lcp) {
							if(newlcp < threshold) {
								heap.erase(*it);
								it->node_ = nullptr;
							} else {
								poi.lcp = newlcp;
								heap.decrease(*it);

							}
						} else {
							break;
						}
					}
				}
				refStrategy.sort(); // the found references can now be sorted and appended to the already sorted list of references (appending preserves the ordering)
				handles.clear();
				--i;
				continue;
			}
			DCHECK_EQ(pplcp.index(), i);
			DCHECK_EQ(plcp_i, pplcp());
			if(plcp_i <= lastpos_lcp) continue;
			DCHECK_LE(threshold, plcp_i);
			handles.emplace_back(heap.emplace(i,plcp_i, handles.size()));
			lastpos = i;
//			DCHECK_EQ(plcp[lastpos], plcp_i);
			lastpos_lcp = plcp_i;
		}
        IF_STATS(StatPhase::log("max heap size", max_heap_size));
		}



/// A very naive selection strategy for LCPComp.
///
/// TODO: Describe
class PLCPStrategy : public Algorithm {
private:
    typedef TextDS<> text_t;

public:
    using Algorithm::Algorithm;

    inline static Meta meta() {
        Meta m("lcpcomp_comp", "plcp", "compressor using PLCP array");
        return m;
    }



    /**
     *  Called by the LCPcompCompressor.
     *  The compressor works in RAM mode, so this method produces the factors in RAM.
     */
    inline void factorize(text_t& text, size_t threshold, lzss::FactorBufferRAM& refs) {
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

    inline static ds::dsflags_t textds_flags() {
        return text_t::SA | text_t::ISA;
    }


//     inline void factorize(text_t& text, size_t threshold, lzss::FactorBufferRAM& refs) { //TODO!
// 		StatPhase phase("Load Index DS");
// //		const std::string textfilename  = "/bighome/workspace/compreSuite/tudocomp/datasets/abracadabra.0";
// 		const std::string textfilename  = "/local1/cc_commoncrawl.ascii.10MB.0";
// 		IntegerFileArray<uint_t<40>> sa  ((textfilename + ".sa5").c_str());
// 		IntegerFileArray<uint_t<40>> isa ((textfilename + ".isa5").c_str());
// 		// SAFileArray<uint_t<40>> sa((textfilename + ".sa5").c_str());
// 		// ISAFileArray<uint_t<40>> isa((textfilename + ".isa5").c_str());
// 		//IntegerFileForwardIterator<uint_t<40>> pplcp("/bighome/workspace/compreSuite/tudocomp/datasets/pc_english.200MB.plcp5");
// 		DCHECK_EQ(sa.size(), text.size());
//
// IF_DEBUG(
// 		StatPhase::wrap("Check Index DS", [&]{
// 			const auto& tsa = text.require_sa();
// 			const auto& tisa = text.require_isa();
// 			const auto& plcp = text.require_plcp();
// 				PLCPFileForwardIterator pplcp    ((textfilename + ".plcp").c_str());
// 			for(size_t i = 0; i < sa.size(); ++i) {
// 			DCHECK_EQ(sa.size(),tsa.size());
// 			DCHECK_EQ(sa[i], (uint64_t)tsa[i]);
// 			}
// 			DCHECK_EQ(isa.size(),tisa.size());
// 			for(size_t i = 0; i < isa.size(); ++i) {
// 			DCHECK_EQ(isa[i], (uint64_t)tisa[i]);
// 			}
// 			for(size_t i = 0; i < plcp.size()-1; ++i) {
// 			DCHECK_EQ(pplcp(),(uint64_t) plcp[i]);
// 			pplcp.advance();
// 			}
// 		});
// 		);
//
// 		PLCPFileForwardIterator pplcp    ((textfilename + ".plcp").c_str());
//
// 		RefRAMStrategy<decltype(sa),decltype(isa)> refStrategy(sa,isa);
// 		//RefDiskStrategy<decltype(sa),decltype(isa)> refStrategy(sa,isa);
// 		StatPhase::wrap("Search Peaks", [&]{
// 				compute_references(text.size()-1, refStrategy, pplcp, threshold);
// 		});
// 		StatPhase::wrap("Compute References", [&]{
// 				refStrategy.factorize(refs);
// 				});
//
// 	}


};

}}//ns

