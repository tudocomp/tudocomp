#include <tudocomp/lz78/Lz78Compressor.hpp>
#include <tudocomp/lz78/Lz78DebugCoder.hpp>
#include <tudocomp/lz78/Lz78BitCoder.hpp>
#include <tudocomp/lz78/lzcics/Lz78cicsCompressor.hpp>

using namespace tudocomp;

template class lz78::Lz78Compressor<lz78::Lz78DebugCoder>;
template class lz78::Lz78Compressor<lz78::Lz78BitCoder>;
template class lz78::lzcics::Lz78cicsCompressor<lz78::Lz78DebugCoder>;
template class lz78::lzcics::Lz78cicsCompressor<lz78::Lz78BitCoder>;
