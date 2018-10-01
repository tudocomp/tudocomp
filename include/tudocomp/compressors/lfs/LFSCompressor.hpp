#pragma once

#include <vector>
#include <tuple>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/io.hpp>

#include <tudocomp/compressors/lfs/EncodeStrategy.hpp>
#include <tudocomp/coders/BitCoder.hpp>
#include <tudocomp/coders/EliasGammaCoder.hpp>


#include <tudocomp/ds/IntVector.hpp>



#include <tudocomp/coders/HuffmanCoder.hpp>


#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lfs {

template<typename comp_strategy_t , typename coding_strat_t = EncodeStrategy<HuffmanCoder, EliasGammaCoder> >
class LFSCompressor : public Compressor {
private:

    typedef std::tuple<uint,uint,uint> non_term;
    typedef std::vector<non_term> non_terminal_symbols;
    typedef std::vector<std::pair<uint,uint>> rules;


public:

    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "lfs_comp", "LFS compression scheme");
        m.param("computing_strat").strategy<comp_strategy_t>(TypeDesc("lfs_comp"));
        m.param("coding_strat").strategy<coding_strat_t>(
            TypeDesc("lfs_comp_enc"),
            Meta::Default<EncodeStrategy<HuffmanCoder, EliasGammaCoder>>());
        m.needs_sentinel_terminator();
        return m;
    }


    inline LFSCompressor(Config&& cfg):
        Compressor(std::move(cfg))
    {
        DLOG(INFO) << "Compressor instantiated";
    }
    inline virtual void compress(Input& input, Output& output) override {

        StatPhase::wrap("lfs compressor", [&]{


            non_terminal_symbols nts_symbols = non_terminal_symbols();
            rules dictionary = rules();
            auto in = input.as_view();
            if(in.size()>1){
                comp_strategy_t strategy(config().sub_config("computing_strat"));

                StatPhase::wrap("computing lrfs", [&]{
                //compute dictionary and nts.
                    strategy.compute_rules( in, dictionary, nts_symbols);

                    DLOG(INFO)<<"dict size: "<<dictionary.size() << std::endl;
                    DLOG(INFO)<<"symbols:"<<nts_symbols.size()<< std::endl;
                });
                 StatPhase::log("Number of CFG rules", dictionary.size());
            }





            // if(dictionary.size()==0){
           //      return;
           //  }


            StatPhase::wrap("encoding input", [&]{


                //StatPhase encode("encoding input");
                coding_strat_t coding_strategy(config().sub_config("coding_strat"));

                coding_strategy.encode(in, output, dictionary, nts_symbols);

            });

        });


    }

    inline virtual void decompress(Input& input, Output& output) override {

        coding_strat_t strategy(config().sub_config("coding_strat"));

        strategy.decode(input,output);
    }

};

}


}
