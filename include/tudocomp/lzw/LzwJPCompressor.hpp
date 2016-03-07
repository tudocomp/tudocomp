#ifndef _INCLUDED_LZW_JP_COMPRESSOR_HPP_
#define _INCLUDED_LZW_JP_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>

#include <lzw_jp.hpp>

namespace tudocomp {

namespace lzw {

/**
 * A simple compressor that echoes the input into the coder without
 * generating any factors whatsoever.
 */
class LzwJpCompressor: public Compressor {
public:
    using Compressor::Compressor;

    inline virtual void compress(Input& in, Output& out) override final {
        auto i_guard = in.as_stream();
        auto& i_stream = *i_guard;
        auto o_guard = out.as_stream();
        auto& o_stream = *o_guard;

        lzw_jp::compress(i_stream, o_stream);
    }

    inline virtual void decompress(Input& in, Output& out) override final {
        auto i_guard = in.as_stream();
        auto& i_stream = *i_guard;
        auto o_guard = out.as_stream();
        auto& o_stream = *o_guard;

        lzw_jp::decompress(i_stream, o_stream);
    }

};

}

}

#endif
