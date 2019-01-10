#pragma once

#include <tudocomp/compressors/lfs/EncodeStrategy.hpp>
#include <tudocomp/Decompressor.hpp>

#include <tudocomp/coders/EliasGammaCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>

namespace tdc {
namespace lfs {

template<
    typename coding_strat_t = EncodeStrategy<HuffmanCoder, EliasGammaCoder>
>
class LFSDecompressor : public Decompressor {
public:
    inline static Meta meta() {
        Meta m(Decompressor::type_desc(), "lfs", "LFS decompression");
        m.param("coding_strat").strategy<coding_strat_t>(
            TypeDesc("lfs_comp_enc"),
            Meta::Default<EncodeStrategy<HuffmanCoder, EliasGammaCoder>>());
        return m;
    }

    using Decompressor::Decompressor;

    inline virtual void decompress(Input& input, Output& output) override {
        coding_strat_t strategy(config().sub_config("coding_strat"));
        strategy.decode(input, output);
    }
};

}} //ns

