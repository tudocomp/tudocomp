#ifndef LZ78COMMON_HPP
#define LZ78COMMON_HPP


namespace tdc {
namespace lz78 {

using factorid_t = std::uint32_t; //! type for the factor indices, bounded by the number of LZ78 trie nodes
static constexpr factorid_t undef_id = std::numeric_limits<factorid_t>::max(); // for a non-existing factor

/// Maximum legal dictionary size.
 const factorid_t DMS_MAX = std::numeric_limits<factorid_t>::max(); //TODO

#define LZ78_DICT_SIZE_DESC \
			"`dict_size` has to either be 0 (unlimited), or a positive integer,\n" \
			"and determines the maximum size of the backing storage of\n" \
			"the dictionary before it gets reset." 


}}//ns


#endif /* LZ78COMMON_HPP */
