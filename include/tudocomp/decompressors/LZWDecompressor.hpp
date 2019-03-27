#pragma once

#include <tudocomp/compressors/lzw/LZWFactor.hpp>
#include <tudocomp/compressors/lzw/LZWDecoding.hpp>

#include <tudocomp/coders/BinaryCoder.hpp>
#include <tudocomp/Decompressor.hpp>

namespace tdc {

template <typename coder_t>
class LZWDecompressor : public Decompressor {
private:
    std::vector<lz_trie::factorid_t> indices;
    std::vector<uliteral_t> literals;

    /// Max dictionary size before reset
    const lz_trie::factorid_t m_dict_max_size {0}; //! Maximum dictionary size before reset, 0 == unlimited

public:
    inline static Meta meta() {
        Meta m(Decompressor::type_desc(), "lzw", "LZW decompressor.");
        m.param("coder", "The input decoder.")
            .strategy<coder_t>(TypeDesc("coder"), Meta::Default<BinaryCoder>());
        m.param("dict_size",
            "the maximum size of the dictionary's backing storage before it "
            "gets reset (0 = unlimited)"
        ).primitive(0);
        return m;
    }

    inline LZWDecompressor(Config&& cfg):
        Decompressor(std::move(cfg)),
        m_dict_max_size(this->config().param("dict_size").as_uint())
    {}

    virtual void decompress(Input& input, Output& output) override final {
        const size_t reserved_size = input.size();
        const size_t dict_max_size = config().param("dict_size").as_uint();

        //TODO C::decode(in, out, dms, reserved_size);
        auto out = output.as_stream();
        typename coder_t::Decoder decoder(config().sub_config("coder"), input);

        size_t counter = 0;

        //TODO file_corrupted not used!
        lzw::decode_step([&](lz_trie::factorid_t& entry, bool reset, bool &file_corrupted) -> bool {
            if (reset) {
                counter = 0;
            }

            if(decoder.eof()) {
                return false;
            }

            lzw::Factor factor(decoder.template decode<len_t>(Range(counter + ULITERAL_MAX + 1)));
            counter++;
            entry = factor;
            return true;
        }, out, dict_max_size == 0 ? lz_trie::DMS_MAX : dict_max_size, reserved_size);
    }
};

}

