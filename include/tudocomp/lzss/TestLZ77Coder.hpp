#ifndef _INCLUDED_DEBUG_LZSS_CODER_HPP
#define _INCLUDED_DEBUG_LZSS_CODER_HPP

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/util.h>
#include <tudocomp/util/DecodeBuffer.hpp>

#include <tudocomp/lzss/LZSSCoderOpts.hpp>
#include <tudocomp/lzss/TestLZ77Factor.hpp>

namespace tudocomp {
namespace lzss {

class TestLZ77Coder {

private:
    size_t m_len;
    io::OutputStream* m_out;

public:
    /// Constructor.
    ///
    /// \param env The environment.
    /// \param in The input text.
    /// \param out The (bitwise) output stream.
    /// \param opts Coder options determined by the compressor.
    inline TestLZ77Coder(Env& env, Input& in, io::OutputStream& out, LZSSCoderOpts opts)
        : m_out(&out) {

        m_len = in.size();
    }

    /// Destructor
    ~TestLZ77Coder() {
    }

    /// Initializes the encoding by writing information to the output that
    /// will be needed for decoding.
    inline void encode_init() {
        **m_out << m_len << ':';
    }

    /// Encodes a LZSS factor to the output.
    inline void encode_fact(const TestLZ77Factor& f) {
        **m_out << "{" << (f.src + 1) << "," << f.num << "}";
    }

    /// Passes a raw symbol to the alphabet encoder.
    inline void encode_sym(uint8_t sym) {
        if(sym == '{' || sym == '\\') {
            **m_out << '\\'; //escape
        }

        **m_out << "{0,0}" << sym;
    }

    /// Notifies the alphabet encoder that the current batch of raw symbols
    /// is finished.
    ///
    /// This information can be used to encode reoccuring short phrases such
    /// as digrams.
    inline void encode_sym_flush() {
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
    inline void buffer_fact(const TestLZ77Factor& f) {
    }

    static void decode(Env&, Input&, Output&);
};

inline void TestLZ77Coder::decode(Env& env, Input& input, Output& out) {

    auto in_guard = input.as_stream();
    std::istream& in = *in_guard;

    char c;

    //Decode length
    size_t len;
    {
        std::stringstream len_str;

        while(in.get(c) && c != ':') {
            len_str << c;
        }

        len_str >> len;
    }

    DecodeBuffer<DCBStrategyRetargetArray> buffer(len);

    //Decode text
    size_t pos = 0;
    bool escape = false;

    while(in.get(c) && pos < len) {
        if(c == '\\' && !escape) {
            escape = true;
        } else {
            size_t src, num;
            {
                std::stringstream src_str, num_str;

                while(in.get(c) && c != ',') {
                    src_str << c;
                }

                while(in.get(c) && c != '}') {
                    num_str << c;
                }

                src_str >> src;
                num_str >> num;
            }
            
            // Here we are decoding a factor
            if(src != 0 || num != 0) {
                buffer.defact(src - 1, num);
                pos += num;
            } else { // Here we are decoding a symbol
                in.get(c);
                buffer.decode(uint8_t(c));
                ++pos;
            }

            escape = false;
        }
    }

    //Write
    auto out_guard = out.as_stream();
    buffer.write_to(*out_guard);
}

}}

#endif
