#pragma once

#include <tudocomp/util.hpp>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/decompressors/WrapDecompressor.hpp>
#include <tudocomp/Tags.hpp>

#include <tudocomp/compressors/lzss/FactorBuffer.hpp>
#include <tudocomp/compressors/lzss/FactorizationStats.hpp>
#include <tudocomp/compressors/lzss/UnreplacedLiterals.hpp>
#include <tudocomp/compressors/lcpcomp/lcpcomp.hpp>
#include <tudocomp/compressors/lcpcomp/decompress/MultiMapBuffer.hpp>

#include <tudocomp/ds/DSManager.hpp>
#include <tudocomp/ds/providers/DivSufSort.hpp>
#include <tudocomp/ds/providers/ISAFromSA.hpp>
#include <tudocomp/ds/providers/PhiAlgorithm.hpp>
#include <tudocomp/ds/providers/PhiFromSA.hpp>
#include <tudocomp/ds/providers/LCPFromPLCP.hpp>

#include <tudocomp_stat/StatPhase.hpp>

#include <tudocomp/compressors/lzss/LZSSCoder.hpp>
#include <tudocomp/compressors/lzss/LZSSCoding.hpp>
#include <tudocomp/decompressors/LZSSDecompressor.hpp>
#include <tudocomp/decompressors/LCPDecompressor.hpp>

namespace tdc {

template<
    typename lzss_coder_t,
    typename ds_t = DSManager<DivSufSort, PhiFromSA, PhiAlgorithm, LCPFromPLCP, ISAFromSA>>
class LexParseCompressor : public Compressor
{
public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "lexparse",
            "Computes the lexparse factorization of the input.");
        m.param("coder", "The output encoder.")
            .strategy<lzss_coder_t>(lzss_bidirectional_coder_type());
        m.param("ds", "The text data structure provider.")
            .strategy<ds_t>(ds::type(), Meta::Default<DSManager<DivSufSort, PhiFromSA, PhiAlgorithm, LCPFromPLCP, ISAFromSA>>());
        m.inherit_tag<ds_t>(tags::require_sentinel);
        m.inherit_tag<lzss_coder_t>(tags::lossy);
        return m;
    }
    
    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        auto in = input.as_view();

        ds_t text(config().sub_config("ds"), in);
        text.template construct<ds::SUFFIX_ARRAY, ds::LCP_ARRAY, ds::INVERSE_SUFFIX_ARRAY>();
        auto &sa = text.template get<ds::SUFFIX_ARRAY>();
        auto &isa = text.template get<ds::INVERSE_SUFFIX_ARRAY>(); 
        auto &lcp = text.template get<ds::LCP_ARRAY>();
        
        lzss::FactorBuffer<> factors;

        for (size_t pos = 0; pos < in.size();) {
            size_t factor_len = lcp[isa[pos]];
            if (factor_len == 0) {
                pos++;
                continue;
            }
            size_t source_pos = sa[isa[pos] - 1];
            factors.emplace_back(pos, source_pos, factor_len);
            pos += factor_len;
        }

        auto coder = lzss_coder_t(config().sub_config("coder")).encoder(
            output,
            lzss::UnreplacedLiterals<decltype(in), decltype(factors)>(in, factors)
        );
        coder.encode_text(in, factors);
    }

    inline std::unique_ptr<Decompressor> decompressor() const override {
        return Algorithm::instance<LCPDecompressor<lzss_coder_t>>();
    }
};

}