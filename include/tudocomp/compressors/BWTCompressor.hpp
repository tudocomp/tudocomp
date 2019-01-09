#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/ds/bwt.hpp>
#include <tudocomp/ds/DSManager.hpp>
#include <tudocomp/ds/providers/DivSufSort.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/decompressors/BWTDecompressor.hpp>
#include <tudocomp/Tags.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

template<typename ds_t = DSManager<DivSufSort>>
class BWTCompressor : public Compressor {
public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "bwt",
            "Computes the Burrows-Wheeler transform of the input text.");
        m.param("ds", "The text data structure provider.")
            .strategy<ds_t>(ds::type(), Meta::Default<DSManager<DivSufSort>>());
        m.inherit_tag<ds_t>(tags::require_sentinel);
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        auto ostream = output.as_stream();
        auto in = input.as_view();

        ds_t ds(config().sub_config("ds"), in);

		const len_t input_size = in.size();

        StatPhase::wrap("Construct Text DS", [&]{
            ds.template construct<ds::SUFFIX_ARRAY>();
        });

        const auto& sa = ds.template get<ds::SUFFIX_ARRAY>();
        for(size_t i = 0; i < input_size; ++i) {
            ostream << bwt::bwt(in, sa, i);
        }
    }

    inline std::unique_ptr<Decompressor> decompressor() const override {
        return Algorithm::instance<BWTDecompressor>();
    }
};

}//ns

