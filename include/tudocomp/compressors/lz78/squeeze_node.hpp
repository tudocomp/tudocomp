#ifndef SQUEEZE_NODE_HPP
#define SQUEEZE_NODE_HPP

namespace tdc {
namespace lz78 {

#ifndef NODE_T
typedef uint64_t node_t;
#else
typedef NODE_T node_t;
#endif


#ifndef ALPHABET_BITS
	#define ALPHABET_BITS (sizeof(literal_t)*8)
#endif //TODO alphabet_bits -> effective alphabet size

inline factorid_t get_id(node_t data) {
	return data>>ALPHABET_BITS;
}
inline literal_t get_letter(node_t data) {
	return static_cast<char>(data & 0xff); //TODO 0xff hard coded
}
inline node_t create_node(factorid_t id, uliteral_t c) {
	return (static_cast<uint64_t>(id)<<ALPHABET_BITS) + static_cast<uint64_t>(c);
}
#undef ALPHABET_BITS

}}//ns

#endif /* SQUEEZE_NODE_HPP */
