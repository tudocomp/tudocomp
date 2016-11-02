#ifndef _INCLUDED_BYTE_CODE_COMPRESSOR_HPP_
#define _INCLUDED_BYTE_CODE_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/io.hpp>

namespace tdc {

template<typename C>
class ByteCodeCompressor : public Compressor {

public:
    inline static Meta meta() {
        Meta m("compressor", "bytecode", "Compressions by encoding individual bytes.");
        m.option("coder").templated<C>();
        return m;
    }

    inline ByteCodeCompressor(Env&& env) : Compressor(std::move(env)) {}

    inline virtual void compress(Input& input, Output& output) override final {
        auto i = input.as_stream();
        auto o = output.as_stream();

        BitOStream bitostream(o);
        C coder(env().env_for_option("coder"), bitostream);
        ByteRange byte_r;

        for(uint8_t c : i) {
            coder.encode(c, byte_r);
        }

        coder.finalize();
    }

    inline virtual void decompress(Input& input, Output& output) override final {
        auto i = input.as_stream();
        auto o = output.as_stream();
        o << i.rdbuf();
    }
};

}

#endif
