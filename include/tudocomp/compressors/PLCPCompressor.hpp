#pragma once

#include <tudocomp/util.hpp>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/compressors/ESACompressor.hpp>

namespace tdc {
namespace plcp {

	template<class phi_t, class sa_t>
		inline phi_t construct_phi_array(const sa_t& sa) {
			auto n = sa.size();
			assert_permutation(sa,n);

			phi_t phi { n, 0, bits_for(n) };
			for(size_t i = 1, prev = sa[0]; i < n; i++) {
				phi[sa[i]] = prev;
				prev = sa[i];
			}
			phi[sa[0]] = sa[n-1];
			assert_permutation(phi,n);
			return phi;
		}
	/*
	 * Constructs PLCP inplace in phi
	 */
	template<typename text_t, typename phi_t>
		inline void phi_algorithm(phi_t& phi, const text_t& text) {
			const size_t n = phi.size();
//			phi_t plcp(std::move(phi.data()));
			for(len_t i = 0, l = 0; i < n - 1; ++i) {
				const len_t phii = phi[i];
				DCHECK_LT(i+l, n);
				DCHECK_LT(phii+l, n);
				DCHECK_NE(i, phii);
				while(text[i+l] == text[phii+l]) { 
					l++;
					DCHECK_LT(i+l, n);
					DCHECK_LT(phii+l, n);
				}
				phi[i] = l;
				if(l) {
					--l;
				}
			}
		}
}//ns

template<typename coder_t,  typename dec_t, typename text_t = TextDS<>>
class PLCPCompressor : public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "plcpcomp");
        m.option("coder").templated<coder_t>();
        m.option("esadec").templated<dec_t, esacomp::SuccinctListBuffer>();
        m.option("textds").templated<text_t, TextDS<>>();
        m.option("threshold").dynamic("3");
        m.needs_sentinel_terminator();
        return m;
    }

    /// Construct the class with an environment.
    inline PLCPCompressor(Env&& env) : Compressor(std::move(env)) {}

    inline virtual void compress(Input& input, Output& output) override {
        auto in = input.as_view();
        DCHECK(in.ends_with(uint8_t(0)));
		TextDS<> text { env().env_for_option("textds"), in };

		const len_t threshold { static_cast<len_t>(env().option("threshold").as_integer()) }; //factor threshold
		env().log_stat("threshold", threshold);


		env().begin_stat_phase("Construct text ds");
		text.require(text_t::SA | text_t::ISA);
		env().end_stat_phase();

		env().begin_stat_phase("Construct PLCP");
		const auto& sa = text.require_sa();
		sdsl::int_vector<> plcp = plcp::construct_phi_array<sdsl::int_vector<>>(sa);
		env().end_stat_phase();

		env().begin_stat_phase("Find POIs");
		IntVector<len_t> pois; // text positions of interest
		plcp::phi_algorithm(plcp,text);
		for(len_t i = 1; i+1 < plcp.size(); ++i) { 
			if(plcp[i-1] < plcp[i] && plcp[i] > threshold) {
				pois.push_back(i);
			}
		}
		std::sort(pois.begin(), pois.end(), [&plcp] (const len_t& a, const len_t& b) { return plcp[a] > plcp[b]; });
		env().end_stat_phase();

		env().begin_stat_phase("Factorize");
		lzss::FactorBuffer factors;
		const auto& isa = text.require_isa();
		//TODO: currently does not strictly replace the longest factors
		for(len_t i = 0; i < pois.size(); ++i) {

			const len_t target_position = pois[i];
			const len_t factor_length = plcp[target_position];
			if(factor_length < threshold) continue;
			DCHECK_NE(isa[target_position],0);
			//create next factor
			const len_t source_position = sa[isa[pois[i]]-1];
			DCHECK_LT(target_position, text.size());
			DCHECK_LT(source_position, text.size());
			DCHECK_LT(factor_length, text.size());
			factors.push_back(lzss::Factor { target_position, source_position, factor_length } );

			for(len_t i = target_position; i < target_position+factor_length; ++i) { // erase
				plcp[i] = 0;
			}

			//trim
			{
				const len_t max_affect = std::min(factor_length, target_position); //if pos_target is at the very beginning, we have less to scan
				//correct intersecting entries
				for(len_t k = 0; k < max_affect; ++k) {
					const len_t affected_position = target_position - k - 1; DCHECK_GE(target_position, k+1);
					plcp[affected_position] = std::min<len_t>(k+1, plcp[affected_position]);
				}
			}

			// add new element, probably resizing and resorting the array of POIs
			const len_t next_target_position = target_position+factor_length;
			if(next_target_position < text.size() && plcp[next_target_position] > threshold) {
				pois.push_back(next_target_position);
				if(pois.capacity() == pois.size()) {
					IntVector<len_t> newpois;
					newpois.reserve(2*pois.size());
					for(len_t j = i+1; j < pois.size(); ++j) {
						if(plcp[pois[j]] > threshold) { newpois.push_back(j); }
					}
					pois.swap(newpois);
					std::sort(pois.begin(), pois.end(), [&plcp] (const len_t& a, const len_t& b) { return plcp[a] > plcp[b]; });
					i=0;
				}
			}
		}
		env().end_stat_phase();
		env().log_stat("factors", factors.size());

		env().begin_stat_phase("Sorting Factors");
		// sort factors
		factors.sort();
		env().end_stat_phase();

		env().begin_stat_phase("Encode Factors");

		// encode
		typename coder_t::Encoder coder(env().env_for_option("coder"), output, lzss::TextLiterals<text_t>(text, factors));

		lzss::encode_text(coder, text, factors); //TODO is this correct?
		env().end_stat_phase();
    }

    inline virtual void decompress(Input& input, Output& output) override {
        //TODO: tell that forward-factors are allowed
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);
        auto outs = output.as_stream();


        //lzss::decode_text_internal<coder_t, dec_t>(decoder, outs);
        // if(lazy == 0)
        // 	lzss::decode_text_internal<coder_t, dec_t>(decoder, outs);
        // else
        esacomp::decode_text_internal<typename coder_t::Decoder, dec_t>(env().env_for_option("esadec"), decoder, outs);
    }
};

}

