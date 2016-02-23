#ifndef _INCLUDED_DUMMYCODER_HPP_
#define _INCLUDED_DUMMYCODER_HPP_

#include <tudocomp/util/decode_buffer.h>

namespace tudocomp {

/**
 * Encodes factors as simple strings.
 */
template<typename F>
class DummyCoder {
    
    const char init_sep   = ':';
    
    static char fact_start;
    static char fact_end;

public:
    DummyCoder() = delete;
    
    /// Constructor for the given output, accepting an environment.
    DummyCoder(Env& env) {
        //
    }
    
    /// Destructor
    ~DummyCoder() {
        //
    }
    
    /// Initiates the encoding with no total length information.
    inline void encode_init(Output& out) {
        encode_init(out, 0); //TODO throw exception!
    }

    /// Initiates the encoding with total length information.
    inline void encode_init(Output& out, size_t len) {
        *(out.as_stream()) << len << init_sep;
    }
    
    /// Encodes the given symbol.
    inline void encode_sym(Output& out, char32_t sym) {
        *(out.as_stream()) << char(sym);
    }
    
    /// Encodes the given factor.
    void encode_fact(Output& out, const F& fact);
    
    /// Finalizes the encoding.
    inline void encode_finalize(Output& out) {
        //
    }
    
    /// Decodes and defactorizes a factor represented by a string.
    inline void decode_fact(DecodeBuffer& decbuf, const std::string& str);
    
    /// Decodes the input
    inline void decode(Input& in, Output& out) {
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
