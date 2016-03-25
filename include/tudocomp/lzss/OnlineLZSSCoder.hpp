#ifndef _INCLUDED_ONLINE_LZSS_CODER_HPP
#define _INCLUDED_ONLINE_LZSS_CODER_HPP

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/lzss/LZSSCoderOpts.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {

/// Encoder for LZSS factors using a static amount of bits to encode factor
/// components.
///
/// The coder will pick a fixed bit size for factor components based on the
/// input text and hints by the compressor. These may be far from optimal,
/// but in return, factors to not need to be buffered in memory.
///
/// \tparam A the alphabet coder to use for encoding raw symbols.
template<typename A>
class OnlineLZSSCoder {
//TODO more unique name (there may be more online coders in the future...)

private:
    BitOStream* m_out;
    std::shared_ptr<A> m_alphabet_coder;
    
    bool m_src_use_delta;
    size_t m_src_bits;
    size_t m_num_bits;
    
public:
    /// Constructor.
    ///
    /// \param env The environment.
    /// \param in The input text.
    /// \param out The (bitwise) output stream.
    /// \param opts Coder options determined by the compressor.
    inline OnlineLZSSCoder(Env& env, Input& in, BitOStream& out, LZSSCoderOpts opts)
            : m_out(&out) {

        size_t len = in.size();
        m_src_bits = std::min(bitsFor(len), opts.src_bits);
        m_num_bits = bitsFor(len);
        m_src_use_delta = opts.use_src_delta;

        //TODO write magic
        out.write_compressed_int(len);
        out.writeBit(m_src_use_delta);
        
        m_alphabet_coder = std::shared_ptr<A>(new A(env, in, out));
    }
    
    /// Initializes the encoding by writing information to the output that
    /// will be needed for decoding.
    inline void encode_init() {
    }
    
    /// Encodes a LZSS factor to the output.
    inline void encode_fact(const LZSSFactor& f) {
        m_alphabet_coder->encode_sym_flush();
        
        m_out->writeBit(1);
        m_out->write(m_src_use_delta ? (f.pos - f.src) : f.src, m_src_bits);
        m_out->write(f.num, m_num_bits);
    }

    /// Passes a raw symbol to the alphabet encoder.
    inline void encode_sym(uint8_t sym) {
        m_alphabet_coder->encode_sym(sym);
    }
    
    /// Notifies the alphabet encoder that the current batch of raw symbols
    /// is finished.
    /// 
    /// This information can be used to encode reoccuring short phrases such
    /// as digrams.
    inline void encode_sym_flush() {
        m_alphabet_coder->encode_sym_flush();
    }
    
    /// Tells whether or not this coder buffers the incoming factors before
    /// encoding.
    /// 
    /// The compressor uses this information to schedule another pass for
    /// the actual encoding.
    inline bool uses_buffer() {
        return false;
    }
    
    /// Sets the factor buffer, which may or may not already contain the
    /// factors coming from the compressor.
    ///
    /// Compressors working with random access on the input may have computed
    /// the factors already. The encoder will then re-use the same buffer.
    template<typename T>
    inline void set_buffer(T& buffer) {
    }
    
    /// Buffers the given factor and allows the encoder to analyze it.
    inline void buffer_fact(const LZSSFactor& f) {
    }
};

}}

#endif
