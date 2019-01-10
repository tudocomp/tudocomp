#pragma once

#include <algorithm>
#include <functional>
#include <vector>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/Tags.hpp>
#include <tudocomp/util.hpp>

#include <tudocomp/compressors/lzss/FactorBuffer.hpp>
#include <tudocomp/compressors/lzss/FactorizationStats.hpp>
#include <tudocomp/compressors/lzss/UnreplacedLiterals.hpp>

#include <tudocomp/decompressors/LZSSDecompressor.hpp>

#include <tudocomp/ds/DSManager.hpp>
#include <tudocomp/ds/providers/DivSufSort.hpp>
#include <tudocomp/ds/providers/ISAFromSA.hpp>
#include <tudocomp/ds/providers/PhiAlgorithm.hpp>
#include <tudocomp/ds/providers/PhiFromSA.hpp>
#include <tudocomp/ds/providers/LCPFromPLCP.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Computes the LZ77 factorization of the input using its suffix array and
/// LCP table.
template<typename lzss_coder_t, typename ds_t = DSManager<DivSufSort, PhiFromSA, PhiAlgorithm, LCPFromPLCP, ISAFromSA>>
class LZSSLCPCompressor : public Compressor {
public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "lzss_lcp",
            "Computes the LZSS factorization of the input using the "
            "suffix and LCP array.");
        m.param("coder", "The output encoder.")
            .strategy<lzss_coder_t>(TypeDesc("lzss_coder"));
        m.param("ds", "The text data structure provider.")
            .strategy<ds_t>(ds::type(), Meta::Default<DSManager<DivSufSort, PhiFromSA, PhiAlgorithm, LCPFromPLCP, ISAFromSA>>());
        m.param("threshold", "The minimum factor length.").primitive(2);
        m.inherit_tag<ds_t>(tags::require_sentinel);
        m.inherit_tag<lzss_coder_t>(tags::lossy);
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        auto view = input.as_view();

        // Construct text data structures
        ds_t ds(config().sub_config("ds"), view);
        StatPhase::wrap("Construct Text DS", [&]{
            ds.template construct<
                ds::SUFFIX_ARRAY,
                ds::LCP_ARRAY,
                ds::INVERSE_SUFFIX_ARRAY>();
        });

        auto& sa = ds.template get<ds::SUFFIX_ARRAY>();
        auto& isa = ds.template get<ds::INVERSE_SUFFIX_ARRAY>();
        auto& lcp = ds.template get<ds::LCP_ARRAY>();

        // Factorize
        const len_t text_length = view.size();
        lzss::FactorBufferRAM factors;

        StatPhase::wrap("Factorize", [&]{
            const len_t threshold = config().param("threshold").as_uint();

            for(len_t i = 0; i+1 < text_length;) { // we omit T[text_length-1] since we assume that it is the \0 byte!
                //get SA position for suffix i
                const size_t& cur_pos = isa[i];
			    DCHECK_NE(cur_pos,0); // isa[i] == 0 <=> T[i] = 0

			    //compute naively PSV
                //search "upwards" in LCP array
                //include current, exclude last
                size_t psv_lcp = lcp[cur_pos];
                ssize_t psv_pos = cur_pos - 1;
                if (psv_lcp > 0) {
                    while (psv_pos >= 0 && sa[psv_pos] > sa[cur_pos]) {
                        psv_lcp = std::min<size_t>(psv_lcp, lcp[psv_pos--]);
                    }
                }

			    //compute naively NSV, TODO: use NSV data structure
                //search "downwards" in LCP array
                //exclude current, include last
                size_t nsv_lcp = 0;
                size_t nsv_pos = cur_pos + 1;
                if (nsv_pos < text_length) {
                    nsv_lcp = SSIZE_MAX;
                    do {
                        nsv_lcp = std::min<size_t>(nsv_lcp, lcp[nsv_pos]);
                        if (sa[nsv_pos] < sa[cur_pos]) {
                            break;
                        }
                    } while (++nsv_pos < text_length);

                    if (nsv_pos >= text_length) {
                        nsv_lcp = 0;
                    }
                }

                //select maximum
                const size_t& max_lcp = std::max(psv_lcp, nsv_lcp);
                if(max_lcp >= threshold) {
				    const ssize_t& max_pos = max_lcp == psv_lcp ? psv_pos : nsv_pos;
				    DCHECK_LT(max_pos, text_length);
				    DCHECK_GE(max_pos, 0);
                    // new factor
                    factors.emplace_back(i, sa[max_pos], max_lcp);

                    i += max_lcp; //advance
                } else {
                    ++i; //advance
                }
            }
        });

        // statistics
        IF_STATS({
            lzss::FactorizationStats stats(factors, view.size());
            stats.log();
        })

        // encode
        StatPhase::wrap("Encode", [&]{
            auto coder = lzss_coder_t(config().sub_config("coder")).encoder(
                output, lzss::UnreplacedLiterals<decltype(view), decltype(factors)>(view, factors));

            coder.encode_text(view, factors);
        });
    }

    inline virtual std::unique_ptr<Decompressor> decompressor() const override {
        return Algorithm::instance<LZSSDecompressor<lzss_coder_t>>();
    }
};

} //ns
