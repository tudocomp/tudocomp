#ifndef _INCLUDED_ONLINE_LZSS_CODER_HPP
#define _INCLUDED_ONLINE_LZSS_CODER_HPP

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>
#include <tudocomp/util/DecodeBuffer.hpp>

#include <tudocomp/lzss/LZSSCoderOpts.hpp>
#include <tudocomp/lzss/LZSSFactor.hpp>
#include <tudocomp/Algorithm.hpp>

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
class OnlineLZSSCoder: Algorithm {
//TODO more unique name (there may be more online coders in the future...)

private:
    std::shared_ptr<BitOStream> m_out;
    std::shared_ptr<A> m_alphabet_coder;

    size_t m_len;

    bool m_src_use_delta;
    size_t m_src_bits;
    size_t m_num_bits;

public:
    inline static Meta meta() {
        Meta m("lzss_coder", "online",
            "Direct encoding of factors"
        );
        m.option("alphabet_coder").templated<A>();
        return m;
    }

    /// Constructor.
    ///
    /// \param env The environment.
    /// \param in The input text.
    /// \param out The (bitwise) output stream.
    /// \param opts Coder options determined by the compressor.
    inline OnlineLZSSCoder(Env&& env, Input& in, io::OutputStream& out, LZSSCoderOpts opts): Algorithm(std::move(env)) {

        m_out = std::make_shared<BitOStream>(out);
        m_len = in.size();
        m_alphabet_coder = std::make_shared<A>(
            this->env().env_for_option("alphabet_coder"), in, *m_out);

        m_src_bits = std::min(bits_for(m_len), opts.src_bits);
        m_num_bits = bits_for(m_len);
        m_src_use_delta = opts.use_src_delta;
    }

    /// Destructor
    ~OnlineLZSSCoder() {
        m_out->flush();
    }

    /// Initializes the encoding by writing information to the output that
    /// will be needed for decoding.
    inline void encode_init() {
        //TODO write magic
        m_out->write_compressed_int(m_len);
        m_out->write_compressed_int(m_src_bits);
        m_out->write_bit(m_src_use_delta);

        m_alphabet_coder->encode_init();
    }

    /// Encodes a LZSS factor to the output.
    inline void encode_fact(const LZSSFactor& f) {
        m_alphabet_coder->encode_sym_flush();

        m_out->write_bit(1);
        m_out->write_int(m_src_use_delta ? (f.pos - f.src) : f.src, m_src_bits);
        m_out->write_int(f.num, m_num_bits);
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

    static void decode(Env&&, Input&, Output&);
};

template<typename A>
inline void OnlineLZSSCoder<A>::decode(Env&& env, Input& input, Output& out) {

    bool done; //GRRR
    auto in_guard = input.as_stream();
    BitIStream in(in_guard, done);

    //Init
    size_t len = in.read_compressed_int();
    size_t num_bits = bits_for(len);
    size_t src_bits = in.read_compressed_int();
    bool src_use_delta = in.read_bit();

    //TODO: use DCBStrategyNone if src_use_delta is true
    DecodeBuffer<DCBStrategyRetargetArray> buffer(len);

    //TODO: wtf
    typename A::template Decoder<DecodeBuffer<DCBStrategyRetargetArray>> alphabet_decoder(env, in, buffer);

    //Decode
    size_t pos = 0;
    while(pos < len) {
        if(in.read_bit()) {
            //decode factor
            size_t src = in.read_int<size_t>(src_bits);
            size_t num = in.read_int<size_t>(num_bits);

            buffer.defact(src_use_delta ? (pos - src) : src, num);
            pos += num;
        } else {
            //decode raw symbol
            pos += alphabet_decoder.decode_sym();
        }
    }

    //Write
    auto out_guard = out.as_stream();
    buffer.write_to(out_guard);
}

}}

#endif
