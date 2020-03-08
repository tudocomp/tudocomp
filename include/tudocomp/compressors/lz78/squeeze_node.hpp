#pragma once
#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/def.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>

namespace tdc {
namespace lz78 {

// typedef uint32_t squeeze_node_t; // TODO: change this to bits_for(literal_t) + bits_for(len_t)
typedef uint_t<40> squeeze_node_t; // TODO: change this to bits_for(literal_t) + bits_for(len_t)
// #ifndef NODE_T
//typedef uint64_t squeeze_node_t;
// #else
// typedef NODE_T node_t;
// #endif


#ifndef ALPHABET_BITS
	#define ALPHABET_BITS (sizeof(uliteral_t)*8)
#endif //TODO alphabet_bits -> effective alphabet size

inline factorid_t get_id(squeeze_node_t data) {
	return static_cast<uint64_t>(data)>>ALPHABET_BITS;
}
inline uliteral_t get_letter(squeeze_node_t data) {
	return static_cast<char>(static_cast<uint64_t>(data)) & 0xff; //TODO 0xff hard coded
}
inline squeeze_node_t create_node(factorid_t id, uliteral_t c) {
	return (static_cast<uint64_t>(id)<<ALPHABET_BITS) + static_cast<uint64_t>(c);
}
// #undef ALPHABET_BITS

}}//ns

