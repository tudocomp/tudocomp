#ifndef _INCLUDED_OFFLINE_LZSS_CODER_HPP
#define _INCLUDED_OFFLINE_LZSS_CODER_HPP

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>

#include <tudocomp/lzss/LZSSCoderOpts.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>

namespace tudocomp {
namespace lzss {

/// Encoder for LZSS factors using a static - but minimum - amount of bits to
/// encode factor components.
///
/// The coder analyzes the factors given by the compressor and computes the
/// minimum amount of bits required to encode factor source positions and
/// lengths.
///
/// This requires the factors to be buffered, yielding a higher memory load
/// than online coders.
///
/// \tparam A the alphabet coder to use for encoding raw symbols.
template<typename A>
class OfflineLZSSCoder {
//TODO more unique name (there may be more offline coders in the future...)

private:
    BitOStream* m_out;
    Input* m_in;
    std::shared_ptr<A> m_alphabet_coder;

    std::vector<LZSSFactor>* m_factors;
    
    bool m_src_use_delta;

    size_t m_num_min = SIZE_MAX;
    size_t m_num_max = 0;

    size_t m_src_max = 0;

    size_t m_num_bits = 0;
    size_t m_src_bits = 0;
    
public:
    /// Constructor.
    ///
    /// \param env The environment.
    /// \param in The input text.
    /// \param out The (bitwise) output stream.
    /// \param opts Coder options determined by the compressor.
    inline OfflineLZSSCoder(Env& env, Input& in, BitOStream& out, LZSSCoderOpts opts)
            : m_out(&out), m_in(&in), m_src_use_delta(opts.use_src_delta) {

        //TODO write magic
        out.write_compressed_int(in.size());
        out.writeBit(m_src_use_delta);
        
        m_alphabet_coder = std::shared_ptr<A>(new A(env, in, out));
    }

    /// Initializes the encoding by writing information to the output that
    /// will be needed for decoding.
    inline void encode_init() {
        m_num_bits = bitsFor(m_num_max - m_num_min);
        m_src_bits = bitsFor(m_src_max);
        
        m_out->write_compressed_int(m_num_min, 4);
        m_out->write_compressed_int(m_num_bits, 5);
        m_out->write_compressed_int(m_src_bits, 5);
    }
    
    /// Encodes a LZSS factor to the output.
    inline void encode_fact(const LZSSFactor& f) {
        m_out->writeBit(1);
        m_out->write(m_src_use_delta ? (f.pos - f.src) : f.src, m_src_bits);
        m_out->write(f.num - m_num_min, m_num_bits);
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
        return true;
    }
    
    /// Sets the factor buffer, which may or may not already contain the
    /// factors coming from the compressor.
    ///
    /// Compressors working with random access on the input may have computed
    /// the factors already. The encoder will then re-use the same buffer.
    inline void set_buffer(std::vector<LZSSFactor>& buffer) {
        m_factors = &buffer;
        
        for(auto f : buffer) {
            analyze_fact(f);
        }
    }
    
    /// Buffers the given factor and allows the encoder to analyze it.
    inline void buffer_fact(const LZSSFactor& f) {
        analyze_fact(f);
        m_factors->push_back(f);
    }
    
private:
    inline void analyze_fact(const LZSSFactor& f) {
        if(f.num < m_num_min) {
            m_num_min = f.num;
        }
        
        if(f.num > m_num_max) {
            m_num_max = f.num;
        }
        
        size_t src = m_src_use_delta ? (f.pos - f.src) : f.src;
        if(src > m_src_max) {
            m_src_max = src;
        }
    }
};

}}

#endif
