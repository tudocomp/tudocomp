#ifndef _INCLUDED_ESP_COMPRESSOR_HPP
#define _INCLUDED_ESP_COMPRESSOR_HPP

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>

namespace tdc {

class EspCompressor : public Compressor {


public:
    inline static Meta meta() {
        Meta m("compressor", "esp", "ESP based grammar compression");
        //m.option("coder").templated<coder_t>();
        //m.option("min_run").dynamic("3");
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        auto in = input.as_view();


    }

    inline virtual void decompress(Input& input, Output& output) override {

    }
};

}

#endif
