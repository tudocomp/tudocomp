#ifndef BWTCOMPRESSOR_HPP
#define BWTCOMPRESSOR_HPP


#include <tudocomp/util.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/bwt.hpp>
#include <tudocomp/ds/TextDS.hpp>

namespace tdc {

template<typename len_t = uint32_t>
class BWTCompressor : public Compressor {

private:
//    const TypeRange<len_t> len_r = TypeRange<len_t>();

public:
    inline static Meta meta() {
        Meta m("compressor", "bwt", "BWT Compressor");
        return m;
    }

    inline BWTCompressor(Env&& env) : Compressor(std::move(env)) {
    }

    inline virtual void compress(Input& input, Output& output) override {
        auto ostream = output.as_stream();
        auto in = input.as_view();

        TextDS<> t(in);
		const len_t input_size = t.size()+1;

        env().begin_stat_phase("Construct Text DS");
        t.require(TextDS<>::SA);
        env().end_stat_phase();
        auto& sa = t.require_sa();
        for(size_t i = 0; i < input_size; ++i) {
            ostream << bwt::bwt(t,sa,i);
        }
    }

    inline virtual void decompress(Input& input, Output& output) override {
        auto in = input.as_view();
        auto ostream = output.as_stream();

		bwt::char_t* decoded_string = bwt::decode_bwt(in);
		if(decoded_string == nullptr) {
			return;
		}
		ostream << decoded_string;
		delete [] decoded_string;
    }
};

}//ns
#endif /* BWTCOMPRESSOR_HPP */
