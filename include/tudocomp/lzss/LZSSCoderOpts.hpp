#ifndef _INCLUDED_LZSS_CODER_OPTS_HPP
#define _INCLUDED_LZSS_CODER_OPTS_HPP

namespace tdc {
namespace lzss {

/// Options for LZSS coders.
///
/// These options are passed to the coder when the compressor instantiates it
/// and should always be decided by the compressor, not by the user.
///
/// Furthermore, they need to be seen merely as hints, which a coder may
/// just as well ignore.
struct LZSSCoderOpts {

    /// Hints that factors will only represent backward references and that
    /// the coder may encode the delta between target and source position,
    /// rather than the absolute source position.
    bool use_src_delta;
    
    /// The amount of bits to use for encoding source positions (or deltas).
    size_t src_bits;
    
    /// Constructor.
    LZSSCoderOpts(bool _use_src_delta)
        : use_src_delta(_use_src_delta), src_bits(64) {
    }
    
    /// Constructor.
    LZSSCoderOpts(bool _use_src_delta, size_t _src_bits)
        : use_src_delta(_use_src_delta), src_bits(_src_bits) {
    }

};
    
}}

#endif
