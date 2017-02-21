#pragma once

#include <vector>
#include <tuple>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/io.hpp>

#include <tudocomp/compressors/lfs/EncodeStrategy.hpp>
#include <tudocomp/coders/BitCoder.hpp>


namespace tdc {
namespace lfs {

template<typename comp_strategy_t , typename coding_strat_t = EncodeStrategy<BitCoder, BitCoder> >
class LFSCompressor : public Compressor {
private:

    typedef std::tuple<uint,uint,uint> non_term;
    typedef std::vector<non_term> non_terminal_symbols;
    typedef std::vector<std::pair<uint,uint>> rules;


public:

    inline static Meta meta() {
        Meta m("compressor", "lfs_comp",
            "This is an implementation of the longest first substitution compression scheme.");
        m.option("computing_strat").templated<comp_strategy_t>();
        m.option("coding_strat").templated<coding_strat_t, EncodeStrategy<BitCoder, BitCoder> >();
        m.needs_sentinel_terminator();
        return m;
    }


    inline LFSCompressor(Env&& env):
        Compressor(std::move(env))
    {
        DLOG(INFO) << "Compressor instantiated";
    }
    inline virtual void compress(Input& input, Output& output) override {

        non_terminal_symbols nts_symbols = non_terminal_symbols();
        rules dictionary = rules();
        auto in = input.as_view();

        comp_strategy_t strategy(env().env_for_option("computing_strat"));


        //compute dictionary and nts.
        strategy.compute_rules(in, dictionary, nts_symbols);

        DLOG(INFO)<<"dict size: "<<dictionary.size();
        DLOG(INFO)<<"symbols:"<<nts_symbols.size();


        coding_strat_t coding_strategy(env().env_for_option("coding_strat"));

        coding_strategy.encode(in,output, dictionary, nts_symbols);


    }

    inline virtual void decompress(Input& input, Output& output) override {

        coding_strat_t strategy(env().env_for_option("coding_strat"));

        strategy.decode(input,output);
    }

};

}


}
