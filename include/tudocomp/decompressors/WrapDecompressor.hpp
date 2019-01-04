#pragma once

#include <memory>
#include <utility>

#include <tudocomp/io.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Decompressor.hpp>
#include <tudocomp/meta/Registry.hpp>

namespace tdc {

class WrapDecompressor : public Decompressor {
private:
    std::unique_ptr<Compressor> m_c;

public:
    inline static Meta meta() {
        Meta m(Decompressor::type_desc(), "wrap",
            "Use a compressor with its own decompress function.");
        m.param("c", "The compressor to wrap.").complex();
        return m;
    }

    inline WrapDecompressor(Config&& cfg)
        : Decompressor(std::move(cfg)) {

        m_c = Registry::of<Compressor>().select(
            meta::ast::convert<meta::ast::Object>(
                config().param("c").ast())).move_instance();
    }

    template<typename C>
    inline WrapDecompressor(const C& c)
        : Decompressor(meta().declare<C>().config(c.config().str()))
    {
        m_c = std::make_unique<C>(Config(c.config()));
    }

    virtual void decompress(Input& input, Output& output) override {
        static_cast<CompressorAndDecompressor*>(m_c.get())->decompress(
            input, output);
    }
};

}
