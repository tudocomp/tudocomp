#include <algorithm>
#include <iostream>
#include <fstream>
#include <tudocomp_stat/StatPhaseStxxl.hpp>
#include <tudocomp/CreateAlgorithm.hpp>
#include <tudocomp/compressors/lcpcomp/compress/PLCPStrategy.hpp>
#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>
#include <tudocomp/compressors/lzss/LZSSCoding.hpp>
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>
#include <stxxl/bits/containers/vector.h>
#include <stxxl/bits/algo/sort.h>
#include <stxxl/bits/io/iostats.h>

namespace tdc { namespace lcpcomp {


template <typename unsigned_t>
struct factor_t {
 public:
    unsigned_t copyFrom, length;
    factor_t() {}        
    factor_t(unsigned_t copyFrom, unsigned_t length) {
        this->copyFrom = copyFrom;
        this->length = length;
    }
    
    unsigned_t copyFromEnd() const {
        return copyFrom + length;
    }
};

template <typename unsigned_t>
struct reference_t : public factor_t<unsigned_t> {
    unsigned_t copyTo;
    reference_t() {}        
    reference_t(unsigned_t copyTo, unsigned_t copyFrom, unsigned_t length) 
            : factor_t<unsigned_t>(copyFrom, length) {
        this->copyTo = copyTo;
    }
    void removePrefix(unsigned_t prefixLength) {
        copyTo += prefixLength;
        factor_t<unsigned_t>::copyFrom += prefixLength;
        factor_t<unsigned_t>::length -= prefixLength;
    }
    
    unsigned_t copyToEnd() const {
        return copyTo + factor_t<unsigned_t>::length;
    }
};

template <typename unsigned_t>
class PLCPDecomp {
    friend class PLCPDecompGenerator;
 public:
    typedef reference_t<unsigned_t> ref_t;
    typedef typename stxxl::VECTOR_GENERATOR<ref_t>::result vector_of_ref_t;
    typedef typename stxxl::VECTOR_GENERATOR<char>::result vector_of_char_t;
    
 private:
 
    void print(vector_of_ref_t vector) {
        for (unsigned i = 0; i < vector.size(); i++)
        {
            std::cout << "(" << vector[i].copyTo << ", " << vector[i].copyFrom << ", " << vector[i].length << "), ";
        }
        std::cout << std::endl;
    }
    
    void print(vector_of_char_t vector) {
        for (unsigned i = 0; i < vector.size(); i++)
        {
            if(vector[i] == '\0') std::cout << "_";
            else std::cout << vector[i];
        }
        std::cout << std::endl;
    }

    // Comparison structs for stxxl sort

    struct reference_limits_t {
        const unsigned_t min_unsigned = std::numeric_limits<unsigned_t>::min();
        const unsigned_t max_unsigned = std::numeric_limits<unsigned_t>::max();
        const ref_t min_factor = ref_t(min_unsigned, min_unsigned, min_unsigned);
        const ref_t max_factor = ref_t(max_unsigned, max_unsigned, max_unsigned);
        ref_t min_value() const { return reference_limits_t::min_factor; }
        ref_t max_value() const { return reference_limits_t::max_factor; }
    };

    struct compare_by_copyTo_t : reference_limits_t {
        bool operator()(const ref_t &a, const ref_t &b) const 
        { return a.copyTo < b.copyTo; }
    } compare_by_copyTo;
    
    struct compare_by_copyFrom_t : reference_limits_t {
        bool operator()(const ref_t &a, const ref_t &b) const 
        { return a.copyFrom < b.copyFrom; }
    } compare_by_copyFrom;
 
    len_t bytes_memory;
    
    vector_of_char_t textBuffer;
    vector_of_ref_t * byCopyToV;
    vector_of_ref_t * byCopyToV_new;
    vector_of_ref_t * byCopyFromV;
    
    
    PLCPDecomp() {
        byCopyToV = new vector_of_ref_t();
        byCopyToV_new = new vector_of_ref_t();
        byCopyFromV = new vector_of_ref_t();
    }
    
    ~PLCPDecomp() {
        delete byCopyToV;
        delete byCopyToV_new;
        delete byCopyFromV;
    }
    
    void scan();
    bool resolve(
        typename vector_of_ref_t::iterator &fromIter, 
        typename vector_of_ref_t::const_iterator &toIter);
    void restore(ref_t &factor);
    
 public:
    vector_of_char_t decompress();
};

        
template <typename unsigned_t>
typename PLCPDecomp<unsigned_t>::vector_of_char_t PLCPDecomp<unsigned_t>::decompress() {

    std::cout << "Decompressing. Type: " << typeid(unsigned_t).name() << std::endl;
    unsigned scanCount = 0;
    while(byCopyToV->size() > 0) {
        tdc::StatPhase scanPhase("Scan " + std::to_string(++scanCount) + " (Factors: " + std::to_string(byCopyToV->size()) + ")");
        scan();
    }
    //~ print(textBuffer);
    
    return textBuffer;
}

template <typename unsigned_t>
void PLCPDecomp<unsigned_t>::scan() {
    std::cout << std::endl;
    std::cout << "SCAN..." << std::endl;
    std::cout << "  Copy from CopyToV to CopyFromV..." << std::endl;
    (*byCopyFromV) = (*byCopyToV);
    std::cout << "  Sort CopyFromV..." << std::endl;
    stxxl::sort(byCopyFromV->begin(), byCopyFromV->end(), compare_by_copyFrom, bytes_memory);
    std::cout << "  Add guard to CopyToV..." << std::endl;
    byCopyToV->push_back(compare_by_copyTo.max_value());
    
    //~ print(textBuffer);
    //~ print(*byCopyToV);
    //~ print(*byCopyFromV);
    
    std::cout << "  Resolve & Jump..." << std::endl;
    auto toIter = byCopyToV->cbegin();
    auto fromIter = byCopyFromV->begin();
    auto fromEnd = byCopyFromV->cend();
    
    for (; fromIter != fromEnd; fromIter++)
    {
        while((*toIter).copyToEnd() <= (*fromIter).copyFrom)
            toIter++;
        
        if(!resolve(fromIter, toIter)) {
            auto nextToIter = toIter + 1;
            while(!resolve(fromIter, nextToIter)) {
                nextToIter++;
            }
        };
    }
    
    std::cout << "  Update CopyToV..." << std::endl;
    vector_of_ref_t * swap = byCopyToV;
    byCopyToV = byCopyToV_new;
    byCopyToV_new = swap;
    byCopyToV_new->clear();
    
    std::cout << "  Sort CopyToV..." << std::endl;
    stxxl::sort(byCopyToV->begin(), byCopyToV->end(), compare_by_copyTo, bytes_memory);
}

template <typename unsigned_t>
bool PLCPDecomp<unsigned_t>::resolve(
        typename vector_of_ref_t::iterator &fromIter, 
        typename vector_of_ref_t::const_iterator &toIter) {
    
    auto const &toFactor = (*toIter);
    auto &fromFactor = (*fromIter);
    
    //~ std::cout << "(" << fromFactor.copyTo << ", " << fromFactor.copyFrom << ", " << fromFactor.length << "), ";
    //~ std::cout << "(" << toFactor.copyTo << ", " << toFactor.copyFrom << ", " << toFactor.length << ")";
    //~ std::cout << "   ";
    
    
    bool wasResolve = false;
    unsigned_t prefixLen;    
    if(toFactor.copyTo <= fromFactor.copyFrom) {
        // prefix of fromFactor cannot be resolved
        prefixLen = std::min(fromFactor.length, unsigned_t(toFactor.copyToEnd() - fromFactor.copyFrom));
        auto jumpOffset = fromFactor.copyFrom - toFactor.copyTo;
        ref_t jumped(fromFactor.copyTo, toFactor.copyFrom + jumpOffset, prefixLen);
        byCopyToV_new->push_back(jumped);
        //~ std::cout << "JUMPED:  (" << jumped.copyTo << ", " << jumped.copyFrom << ", " << jumped.length << ")";
        //~ std::cout << "   ";
    } else {
        // prefix of fromFactor can be resolved
        prefixLen = std::min(fromFactor.length, unsigned_t(toFactor.copyTo - fromFactor.copyFrom));
        ref_t resolved(fromFactor.copyTo, fromFactor.copyFrom, prefixLen);
        restore(resolved);
        wasResolve = true;
        //~ std::cout << "RESOLVED:  (" << resolved.copyTo << ", " << resolved.copyFrom << ", " << resolved.length << ")";
        //~ std::cout << "   ";
    }
    
    if(prefixLen == fromFactor.length) {
        //~ std::cout << "REMAINS: NOTHING" << std::endl;
        return true;
    } else {
        fromFactor.copyTo += prefixLen;
        fromFactor.copyFrom += prefixLen;
        fromFactor.length -= prefixLen;
        //~ std::cout << "REMAINS: (" << fromFactor.copyTo << ", " << fromFactor.copyFrom << ", " << fromFactor.length << ")" << std::endl;
        if(wasResolve) return resolve(fromIter, toIter);
        else return false;
    }    
}

template <typename unsigned_t>
void PLCPDecomp<unsigned_t>::restore(ref_t &reference) {
    auto fromIter = textBuffer.cbegin() + reference.copyFrom - 1;
    auto toIter = textBuffer.begin() + reference.copyTo - 1;
    
    for (unsigned_t i = 0; i < reference.length; i++)
    {
        (*toIter++) = (*fromIter++);
    }    
}

}} //ns
