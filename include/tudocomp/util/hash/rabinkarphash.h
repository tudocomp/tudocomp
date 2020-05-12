#ifndef KARPRABINHASH
#define KARPRABINHASH


#include "characterhash.h"

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/util/HashTypes.hpp>

namespace tdc {

/**
* This is a randomized version of the Karp-Rabin hash function.
* Each instance is a rolling hash function meant to hash streams of characters.
* Each new instance of this class comes with new random keys.
*
* Recommended usage to get L-bit hash values over n-grams:
*        KarpRabinHash<> hf(n,L );
*        for(uint32 k = 0; k<n;++k) {
*                  unsigned char c = ... ; // grab some character
*                  hf.eat(c); // feed it to the hasher
*        }
*        while(...) { // go over your string
*           hf.hashvalue; // at all times, this contains the hash value
*           unsigned char c = ... ;// points to the next character
*           unsigned char out = ...; // character we want to forget
*           hf.update(out,c); // update hash value
*        }
*/
template<typename hashvaluetype>
class KarpRabinHash  : public Algorithm {
public:
	static constexpr size_t hashvalue_bitwidth = std::numeric_limits<hashvaluetype>::digits;
	// typedef uint64_t hashvaluetype;
	typedef unsigned char chartype;
	typedef hashvaluetype key_type;
    inline static Meta meta() {
        Meta m(hash_roller_type(), "rk" + std::to_string(hashvalue_bitwidth), "Karp-Rabin Rolling Hash");
		return m;
	}
	void operator+=(char c) { eat(c); }
	hashvaluetype operator()() const { return hashvalue; }
	void clear() { hashvalue= 0;}

    // myn is the length of the sequences, e.g., 3 means that you want to hash sequences of 3 characters
    // mywordsize is the number of bits you which to receive as hash values, e.g., 19 means that the hash values are 19-bit integers
    KarpRabinHash(Config&& cfg) : Algorithm(std::move(cfg)), hashvalue(0),
        hasher( maskfnc<hashvaluetype>(wordsize))
	{
    }

    // // this is a convenience function, use eat,update and .hashvalue to use as a rolling hash function
    // template<class container>
    // hashvaluetype  hash(container & c) {
    //     hashvaluetype answer(0);
    //     for(uint k = 0; k<c.size(); ++k) {
    //         hashvaluetype x(1);
    //         for(uint j = 0; j< c.size()-1-k; ++j) {
    //             x= (x * B) & HASHMASK;
    //         }
    //         x= (x * hasher.hashvalues[c[k]]) & HASHMASK;
    //         answer=(answer+x) & HASHMASK;
    //     }
    //     return answer;
    // }

    // add inchar as an input, this is used typically only at the start
    // the hash value is updated to that of a longer string (one where inchar was appended)
    void eat(chartype inchar) {
        hashvalue = (B * hashvalue +  hasher.hashvalues[inchar] );
    }


    hashvaluetype hashvalue;
    static constexpr size_t wordsize = 64;
    CharacterHash<uint64_t,chartype> hasher;
    static constexpr hashvaluetype B=37;
};
	
using KarpRabinHash64 = KarpRabinHash<uint64_t>;
using KarpRabinHash128 = KarpRabinHash<__uint128_t>;


}//ns
#endif


