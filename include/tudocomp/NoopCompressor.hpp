#ifndef _INCLUDED_NOOP_COMPRESSOR_HPP_
#define _INCLUDED_NOOP_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>

namespace tdc {

class NoopCompressor: public Compressor {

public:
    inline static Meta meta() {
        Meta m("compressor", "noop");
        return m;
    }

    inline NoopCompressor(Env&& env):
        Compressor(std::move(env)) {}

    inline virtual void compress(Input& input, Output& output) override final {
        auto i = input.as_stream();
        auto o = output.as_stream();
        o << i.rdbuf();
    }

    inline virtual void decompress(Input& input, Output& output) override final {
        auto i = input.as_stream();
        auto o = output.as_stream();
        o << i.rdbuf();
    }
};

}

#endif
