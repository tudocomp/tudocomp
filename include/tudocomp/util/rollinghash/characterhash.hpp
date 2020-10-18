#pragma once

// Source: https://github.com/tudocomp/rollinghashcpp

#include <cassert>
#include <iostream>
#include <stdexcept>

#include <tudocomp/util/rollinghash/mersennetwister.hpp>

/// \cond INTERNAL
namespace tdc{namespace rollinghash {

typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned int uint;

using namespace std;

class mersenneRNG {
public:
    inline mersenneRNG(uint32 maxval) : mtr(),n(maxval) {};
    inline uint32 operator()() {
        return mtr.randInt(n);
    }
    inline void seed(uint32 seedval) {
        mtr.seed(seedval);
    }
    inline void seed() {
        mtr.seed();
    }
    inline uint32 rand_max() {
        return n;
    }
private:
    MTRand mtr;
    int n;
};

template <typename hashvaluetype>
inline hashvaluetype maskfnc(uint64_t bits) {
    assert(bits>0);
    assert(bits<=sizeof(hashvaluetype)*8);
    hashvaluetype x = static_cast<hashvaluetype>(1) << (bits - 1);
    return x ^ (x - 1);
}

template <typename hashvaluetype = uint32, typename chartype =  unsigned char>
class CharacterHash {
public:
    inline CharacterHash(hashvaluetype maxval) {
        if(sizeof(hashvaluetype) <=4) {
            mersenneRNG randomgenerator(maxval);
            for(size_t k =0; k<nbrofchars; ++k)
                hashvalues[k] = static_cast<hashvaluetype>(randomgenerator());
        } else if (sizeof(hashvaluetype) == 8) {
            mersenneRNG randomgenerator(maxval>>32);
            mersenneRNG randomgeneratorbase((maxval>>32) ==0 ? maxval : 0xFFFFFFFFU);
            for(size_t k =0; k<nbrofchars; ++k)
                hashvalues[k] = static_cast<hashvaluetype>(randomgeneratorbase())
                                | (static_cast<hashvaluetype>(randomgenerator()) << 32);
        } else throw runtime_error("unsupported hash value type");
    }

    inline CharacterHash(hashvaluetype maxval, uint32 seed1, uint32 seed2) {
        if(sizeof(hashvaluetype) <=4) {
            mersenneRNG randomgenerator(maxval);
            randomgenerator.seed(seed1);
            for(size_t k =0; k<nbrofchars; ++k)
                hashvalues[k] = static_cast<hashvaluetype>(randomgenerator());
        } else if (sizeof(hashvaluetype) == 8) {
            mersenneRNG randomgenerator(maxval>>32);
            mersenneRNG randomgeneratorbase((maxval>>32) ==0 ? maxval : 0xFFFFFFFFU);
            randomgenerator.seed(seed1);
            randomgeneratorbase.seed(seed2);
            for(size_t k =0; k<nbrofchars; ++k)
                hashvalues[k] = static_cast<hashvaluetype>(randomgeneratorbase())
                                | (static_cast<hashvaluetype>(randomgenerator()) << 32);
        } else throw runtime_error("unsupported hash value type");
    }

    enum {nbrofchars = 1 << ( sizeof(chartype)*8 )};

    hashvaluetype hashvalues[1 << ( sizeof(chartype)*8 )];
};

}}

/// \endcond
