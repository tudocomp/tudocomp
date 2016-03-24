#include <tudocomp/lzw/LzwCompressor.hpp>
#include <tudocomp/lzw/LzwDebugCoder.hpp>
#include <tudocomp/lzw/LzwBitCoder.hpp>

using namespace tudocomp;

template class lzw::LzwCompressor<lzw::LzwDebugCoder>;
template class lzw::LzwCompressor<lzw::LzwBitCoder>;
