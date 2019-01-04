#pragma once

#include <tudocomp/Error.hpp>
#include <tudocomp/Tags.hpp>

#include <tudocomp/compressors/lzcics/st.hpp>
#include <tudocomp/compressors/lzss/DecompBackBuffer.hpp>
#include <tudocomp/compressors/lzss/Factor.hpp>
#include <tudocomp/decompressors/LZSSDecompressor.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

template<typename lzss_coder_t>
class LZ77CicsCompressor : public Compressor {
public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "lz77cics", "LZ77 compression in compressed space.");
        m.param("coder", "The output encoder.")
            .strategy<lzss_coder_t>(TypeDesc("lzss_coder"));
        m.add_tag(tags::require_sentinel);
        return m;
    }

    using Compressor::Compressor;

    /// \copydoc Compressor::compress
    inline virtual void compress(Input& input, Output& output) override {
        auto text = input.as_view();
        MissingSentinelError::check(text);

        const size_t n = text.size();

        // initialize encoder
        auto coder = lzss_coder_t(config().sub_config("coder"))
            .encoder(output, ViewLiterals(text));

        coder.factor_length_range(Range(n));
        coder.encode_header();

        // construct suffix tree
        lzcics::cst_t cst;
        auto st = StatPhase::wrap("Construct ST", [&]{
            return lzcics::suffix_tree(text.data(), cst);
        });
		DCHECK_EQ(st.cst.size(), n);

        // factorize
        StatPhase::wrap("Factorize", [&]{
	        sdsl::bit_vector bV(st.internal_nodes);
	        sdsl::bit_vector bW(st.internal_nodes);
	        auto ell = st.smallest_leaf();
	        DVLOG(2) << "Selecting leaf " << ell;
	        size_t p = 0; // text position of the next (maybe current) factor
	        size_t x = 0; // invariant: x == label(ell)
	        do {
		        //assert(x == st.label(ell));
		        auto v = st.parent(ell);
		        while(v != st.root) {
			        const auto vid = st.nid(v); // internal node id of v
			        if(bV[vid]) {
				        DVLOG(2) << "p = " << p << " <-> " << x;
				        if(x == p) {
					        DVLOG(2) << "found witness " << v << " of leaf " << ell;
					        bW[vid] = true;
					        p += st.str_depth(v);
				        }
				        break;
			        }
			        bV[vid] = true;
			        v = st.parent(v);
		        }
		        if(v == st.root) {
			        DCHECK_EQ(x, p); // it is only possible to find a fresh factor by a leaf that is corresponding
			        ++p;
			        DVLOG(2) << "Found fresh letter";
		        }
		        ell = st.next_leaf(ell);
		        ++x;
				DCHECK_LE(x, n+1); // << "at leaf " << st.cst.sn(ell); // the length x must be at most the text length
		        DVLOG(2) << "Selecting leaf " << ell;
	        } while(ell != st.smallest_leaf());
	        DVLOG(2) << bV;
	        DVLOG(2) << bW;

	        lzcics::reset_bitvector(bV);
	        DVLOG(2) << bV;
	        DVLOG(2) << bW;
        /* TODO
	        Compress bW, drop it and create an enc_vector for bW
	        sdsl::rrr_vector<> bWC(bW);
	        sdsl::sd_vector<> bWC2(bW);
        */
	        sdsl::rank_support_v5<1> rankW(&bW);
	        p = 0;
	        const size_t zw = rankW.rank(bW.size());
	        size_t W[zw];
	        ell = st.smallest_leaf();
	        x = 0; // invariant: x == label(ell)
	        do {
		        auto v = st.parent(ell);
		        while(v != st.root) {
			        const auto vid = st.nid(v); // internal node id of v
			        if(bV[vid]) {
				        if(x == p) {
                            auto src = W[rankW.rank(vid)];
                            auto len = st.str_depth(v);

					        DVLOG(2) << "Pos: " << src << ", Len: " << len;
                            coder.encode_factor(lzss::Factor(p, src, len));
					        p += len;
				        }
				        break;
			        }
			        if(bW[vid]) {
				        W[rankW.rank(vid)] = x;
				        //std::cout << "set W[" << rankW.rank(vid) << "] = " << st.label(ell) << std::endl;
			        }
			        bV[vid] = 1;
			        v = st.parent(v);
		        }
		        DVLOG(2) << "label(ell) = " << x  << " <-> " << p;
		        if(x == p) {
                    auto c = st.head(ell);
			        DVLOG(2) << "Cha: " << c;
                    if(p < n) {
                        // don't encode sentinel
                        coder.encode_literal(c);
                    }
			        ++p;
		        }
		        ell = st.next_leaf(ell);
		        ++x;
		        DVLOG(2) << "Selecting leaf " << ell;
	        } while( ell != st.smallest_leaf());

	        DVLOG(2) << "W array";
	        for(size_t i = 0; i < zw; ++i)
		        DVLOG(2) << W[i];
        });
    }

    inline virtual std::unique_ptr<Decompressor> decompressor() const override {
        return Algorithm::unique_instance<LZSSDecompressor<lzss_coder_t>>();
    }
};

}

