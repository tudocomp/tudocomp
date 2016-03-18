#ifndef _INCLUDED_DUMMYCODER_HPP_
#define _INCLUDED_DUMMYCODER_HPP_

#include <tudocomp/Coder.hpp>
#include <tudocomp/util/DecodeBuffer.hpp>

namespace tudocomp {

/**
 * Encodes factors as simple strings.
 */
template<typename F>
class DummyCoder : public Coder<F> {

    const char init_sep = ':';

    static char fact_start;
    static char fact_end;

public:
    /// The default constructor is not supported.
    DummyCoder() = delete;

    /// Constructor for an environment.
    DummyCoder(Env& env) : Coder<F>(env) {
    }

    /// Destructor
    ~DummyCoder() {
    }

    /// Initiates the encoding with no total length information.
    inline virtual void encode_init(Output& out) override {
        encode_init(out, 0); //TODO throw exception!
    }

    /// Initiates the encoding with total length information.
    inline virtual void encode_init(Output& out, size_t len) override {
        *(out.as_stream()) << len << init_sep;
    }

    /// Encodes a raw symbol.
    inline virtual void encode_sym(Output& out, char32_t sym) override {
        *(out.as_stream()) << char(sym);
    }

    /// Encodes a factor of the supported type.
    virtual void encode_fact(Output& out, const F& fact) override;

    /// Finalizes the encoding.
    inline virtual void encode_finalize(Output& out) override {
    }

private:
    /// Decodes and defactorizes a factor represented by a string.
    void decode_fact(DecodeBuffer& decbuf, const std::string& str);

public:
    /// Decodes and defactorizes the input
    inline virtual void decode(Input& in, Output& out) override {
        auto in_guard = in.as_stream();
        std::istream& ins = *in_guard;

        //read original text length
        size_t len = 0;
        {
            std::stringbuf buf;
            ins.get(buf, init_sep); //TODO fail safety
            ins.get(); //pop delimeter

            std::stringstream(buf.str()) >> len;
        }

        //decode and defactorize
        DecodeBuffer decbuf(len);

        char c;
        while(ins.get(c)) {
            if(c == fact_start) {
                //Houston, we got a factor!
                //parse until end and decode
                std::stringbuf buf;
                ins.get(buf, fact_end); //TODO fail safety
                ins.get(); //pop delimeter

                decode_fact(decbuf, buf.str());
            } else {
                //raw symbol
                decbuf.push_decoded_byte(c);
            }
        }

        decbuf.write_to(*(out.as_stream()));
    }
};

}

#endif
