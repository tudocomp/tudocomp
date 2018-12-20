#pragma once

#include <tudocomp/Decompressor.hpp>

namespace tdc {

class ChainDecompressor : public Decompressor {
public:
    inline static Meta meta() {
        Meta m(Decompressor::type_desc(), "chain",
            "Executes two decompressors consecutively, passing the first "
            "compressor's output to the input of the second.");
        m.param("first", "The first decompressor.").complex();
        m.param("second", "The second decompressor.").complex();
        return m;
    }

private:
    std::unique_ptr<Decompressor> get_decompressor(const std::string& option) {
        return Registry::of<Decompressor>().select(
            meta::ast::convert<meta::ast::Object>( // TODO: shorter syntax for conversion?
                config().param(option).ast())).move_instance();
    }

public:
    using Decompressor::Decompressor;

    virtual void decompress(Input& input, Output& output) override {
        auto first = get_decompressor("first");
        auto second = get_decompressor("second");

        std::vector<uint8_t> between_buf;
        {
            Output between(between_buf);
            first->decompress(input, between);
            DVLOG(1) << "Buffer between chain: "
                     << vec_to_debug_string(between_buf);
        }
        {
            Input between(between_buf);
            second->decompress(between, output);
            // FIXME: between_buf should be freed as soon as possible to avoid
            //        wasting memory in longer chains!
        }
    }
};

}

