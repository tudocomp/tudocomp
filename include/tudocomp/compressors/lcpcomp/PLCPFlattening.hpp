#pragma once

#include <utility>
#include <tudocomp/compressors/lcpcomp/PLCPTypes.hpp>
#include <stxxl/bits/containers/vector.h>
#include <stxxl/bits/algo/sort.h>
#include <stxxl/bits/io/iostats.h>

namespace tdc { namespace lcpcomp {

template <typename unsigned_t>
class PLCPFlattener {
 public:
    typedef reference_t<unsigned_t> ref_t;
    typedef typename stxxl::VECTOR_GENERATOR<ref_t>::result vector_of_ref_t;
    typedef typename vector_of_ref_t::bufwriter_type vector_of_ref_writer_t;
    typedef typename vector_of_ref_t::bufreader_type vector_of_ref_reader_t;

 private:
 
    compare_by_copyTo_t<unsigned_t> compare_by_copyTo = compare_by_copyTo_t<unsigned_t>();
    compare_by_copyFrom_t<unsigned_t> compare_by_copyFrom = compare_by_copyFrom_t<unsigned_t>();

    vector_of_ref_t flat;
    vector_of_ref_t sharpByCopyFrom;
    vector_of_ref_t * sharpByCopyTo;
    vector_of_ref_t * sharpNew;
    
    vector_of_ref_writer_t * flatWriter;
    vector_of_ref_writer_t * sharpWriter;
    
    bool closed = false;
    
    void scan(uint64_t bytesMemory) {
        delete flatWriter;
        delete sharpWriter;
        
        std::swap(sharpByCopyTo, sharpNew);
        sharpNew->clear();
        
        sharpByCopyFrom = *sharpByCopyTo;
        sharpByCopyTo->push_back(compare_by_copyTo.max_value());
        
        std::cout << "    Sorting SharpCopyTo..." << sharpByCopyTo->size() << std::endl;
        stxxl::sort(sharpByCopyTo->begin(), sharpByCopyTo->end(), compare_by_copyTo, bytesMemory);
        std::cout << "    Sorting SharpCopyFrom..." << sharpByCopyFrom.size() << std::endl;
        stxxl::sort(sharpByCopyFrom.begin(), sharpByCopyFrom.end(), compare_by_copyFrom, bytesMemory);
        std::cout << "    Sorting Flat..." << flat.size() << std::endl;
        stxxl::sort(flat.begin(), flat.end(), compare_by_copyTo, bytesMemory);
        
        flatWriter = new vector_of_ref_writer_t(flat.end());
        sharpWriter = new vector_of_ref_writer_t(*sharpNew);

        std::cout << "  Flattening..." << std::endl;
        vector_of_ref_reader_t fromReader(sharpByCopyFrom);
        vector_of_ref_reader_t toReader1(*sharpByCopyTo);
        vector_of_ref_reader_t toReader2(flat);
        
        auto toFactor1 = *toReader1;
        auto toFactor2 = *toReader2;
        
        for (; !fromReader.empty(); ++fromReader)
        {
            auto fromFactor = *fromReader;
            
            while(toFactor1.copyToEnd() <= fromFactor.copyFrom) {
                ++toReader1;
                toFactor1 = *toReader1;
            }
            
            while(toFactor2.copyToEnd() <= fromFactor.copyFrom) {
                ++toReader2;
                toFactor2 = *toReader2;
            }
            
            ref_t * toFactor = nullptr;
            if(toFactor1.copyTo <= fromFactor.copyFrom && toFactor1.copyToEnd() >= fromFactor.copyFromEnd()) {
                toFactor = &toFactor1;
            }
            else if(toFactor2.copyTo <= fromFactor.copyFrom && toFactor2.copyToEnd() >= fromFactor.copyFromEnd()) {
                toFactor = &toFactor2;
            }
            
            // jump possible
            if(toFactor) {
                auto jumpOffset = fromFactor.copyFrom - toFactor->copyTo;
                *sharpWriter << ref_t(fromFactor.copyTo, toFactor->copyFrom + jumpOffset, fromFactor.length);
            }
            else {
                *flatWriter << fromFactor;
            }
        }
        
        flatWriter->finish();
        sharpWriter->finish();
        
    }

 public:    
    PLCPFlattener() {
        sharpByCopyTo = new vector_of_ref_t();
        sharpNew = new vector_of_ref_t();
        flatWriter = new vector_of_ref_writer_t(flat);
        sharpWriter = new vector_of_ref_writer_t(*sharpNew);
        
        *flatWriter << compare_by_copyTo.max_value();
    }
    
    ~PLCPFlattener() {
        delete sharpByCopyTo;
        delete sharpNew;
        delete flatWriter;
        delete sharpWriter;
    }
    
    void add(unsigned_t copyTo, unsigned_t copyFrom, unsigned_t length) {
        *sharpWriter << ref_t(copyTo, copyFrom, length);
    }
    
    void flatten(uint64_t rounds = 3, uint64_t bytesMemory = 1 * 1024 * 1024 * 1024) {
        if(closed) {
            std::cerr << "Cannot apply flattening: Flattener has been used before. [Skipping]" << std::endl;
        }
        else {        
            flatWriter->finish();
            sharpWriter->finish();
            unsigned scanId = 0;        
            while(scanId < rounds && sharpNew->size() > 0) {
                ++scanId;
                std::cout << "Scan " << scanId << ":" << std::endl;
                std::cout << "    SharpNew:  " << sharpNew->size() << std::endl;
                //~ std::cout << "    SharpTo:   " << sharpByCopyTo->size() << std::endl;
                //~ std::cout << "    SharpFrom: " << sharpByCopyFrom.size() << std::endl;
                std::cout << "    Flat:      " << flat.size() << std::endl;
                std::cout << "    Total:     " << sharpNew->size() + flat.size() << std::endl;
                std::cout << "    Mem Sort:  " << bytesMemory << std::endl;
                scan(bytesMemory);
            }

            delete flatWriter;
            flatWriter = new vector_of_ref_writer_t(flat.end());
            vector_of_ref_reader_t sharpReader(*sharpNew);
            for(auto f : sharpReader) *flatWriter << f;
            flatWriter->finish();
            
            sharpByCopyFrom.clear();
            sharpByCopyTo->clear();
            sharpNew->clear();
            stxxl::sort(flat.begin(), flat.end(), compare_by_copyTo, bytesMemory);
            flat.pop_back();
        }
        closed = true;
    }
    
    decltype(flat.cbegin()) begin() {
        if(closed) {
            return flat.cbegin();
        }
        else {
            std::cout << "Cannot get flattening result. Flatten has not been called yet. [Returning empty iterator]" << std::endl;
            return decltype(flat.cbegin())();
        }
    }
    
    decltype(flat.cend()) end() {
        if(closed) {
            return flat.cend();
        }
        else {
            std::cout << "Cannot get flattening result. Flatten has not been called yet. [Returning empty iterator]" << std::endl;
            return decltype(flat.cend())();
        }
    }
};

}} //ns
