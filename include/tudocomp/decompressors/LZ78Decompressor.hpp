#pragma once

#include <tudocomp/compressors/lz78/LZ78Coding.hpp>

#include <tudocomp/coders/BinaryCoder.hpp>
#include <tudocomp/Decompressor.hpp>

namespace tdc {

template <typename coder_t>
class LZ78Decompressor : public Decompressor {
private:
    std::vector<lz_common::factorid_t> indices;
    std::vector<uliteral_t> literals;

    /// Max dictionary size before reset, 0 == unlimited
    const lz_common::factorid_t m_dict_max_size {0};

public:
    inline static Meta meta() {
        Meta m(Decompressor::type_desc(), "lz78", "LZ78 decompressor.");
        m.param("coder", "The output decoder.")
            .strategy<coder_t>(TypeDesc("coder"), Meta::Default<BinaryCoder>());
        m.param("dict_size",
            "the maximum size of the dictionary's backing storage before it "
            "gets reset (0 = unlimited)"
        ).primitive(0);
        return m;
    }

    inline LZ78Decompressor(Config&& cfg):
        Decompressor(std::move(cfg)),
        m_dict_max_size(this->config().param("dict_size").as_uint())
    {}

    virtual void decompress(Input& input, Output& output) override final {
        auto out = output.as_stream();
        typename coder_t::Decoder decoder(config().sub_config("coder"), input);

        lz78::Decompressor decomp;
        uint64_t factor_count = 0;

        while (!decoder.eof()) {
            const lz_common::factorid_t index = decoder.template decode<lz_common::factorid_t>(Range(factor_count));
            const uliteral_t chr = decoder.template decode<uliteral_t>(literal_r);
            decomp.decompress(index, chr, out);
            factor_count++;
        }

        out.flush();
    }
};

}

