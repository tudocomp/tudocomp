#pragma once

//std includes:
#include <vector>
#include <tuple>

//general includes:
#include <tudocomp/util.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/Error.hpp>
#include <tudocomp/Tags.hpp>

#include <tudocomp_stat/StatPhase.hpp>

//sdsl include stree:
#include <sdsl/suffix_trees.hpp>

//includes encoding:
#include <tudocomp/io/BitOStream.hpp>

#include <tudocomp/coders/EliasGammaCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>

//decompressor:
#include <tudocomp/decompressors/LFS2Decompressor.hpp>

namespace tdc {
namespace lfs {

template<
    typename literal_coder_t = HuffmanCoder,
    typename len_coder_t = EliasGammaCoder
>
class LFS2Compressor : public Compressor {
private:
    //Suffix Tree type + st
    typedef sdsl::cst_sct3< sdsl::csa_bitcompressed<> > cst_t;
    cst_t stree;

    using node_type = typename cst_t::node_type;

    //Stores nts_symbols of first layer
    IntVector<uint> first_layer_nts;
    // offset to begin of last nts +1. if ==0 no substitution
    IntVector<uint> fl_offsets;
    // stores subs in first layer symbols
    IntVector<uint> second_layer_nts;
    // dead pos in first layer
    BitVector second_layer_dead;

    //pair contains begin pos, length
    std::vector<std::pair<uint, uint> > non_terminal_symbols;

    //stores node_ids of corresponding factor length
    std::vector<std::vector<uint> > bins;

    //stores beginning positions corresponding to node_ids
    std::vector<std::vector<uint> > node_begins;

    bool exact;
    uint size;

public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "lfs2", "lfs2 with simst");
        m.param("min_lrf").primitive(5);
        m.param("exact").primitive(0);
        m.param("lfs2_lit_coder").strategy<literal_coder_t>(
            Coder::type_desc(), Meta::Default<HuffmanCoder>());
        m.param("lfs2_len_coder").strategy<len_coder_t>(
            Coder::type_desc(), Meta::Default<EliasGammaCoder>());
        m.add_tag(tags::require_sentinel);
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        uint min_lrf = config().param("min_lrf").as_uint();
        exact = config().param("exact").as_bool();

        auto in = input.as_view();
        MissingSentinelError::check(in);

        //create vectors:
        first_layer_nts = IntVector<uint>(input.size(), 0);
        fl_offsets = IntVector<uint>(input.size(), 0);
        second_layer_nts = IntVector<uint>(input.size(), 0);
        second_layer_dead = BitVector(input.size(), 0);

        if(in.size() >= min_lrf){

        StatPhase::wrap("Constructing ST", [&]{
            size =  in.size();
            //remove sentinel because sdsl cant handle that
            while(in[size-1] == 0){
                size--;
            }



            std::string in_string ((const char*) in.data(), size);
            sdsl::construct_im(stree, in_string , 1);
        });

        StatPhase::wrap("Computing LRF", [&]{
            bins.resize(200);
            uint node_counter = 0;

            typedef sdsl::cst_bfs_iterator<cst_t> iterator;
            iterator begin = iterator(&stree, stree.root());
            iterator end   = iterator(&stree, stree.root(), true, true);

            StatPhase::wrap("Iterate over ST", [&]{
                DLOG(INFO)<<"iterate st";

                for (iterator it = begin; it != end; ++it) {

                    if(!stree.is_leaf(*it)){

                        if(bins.size() <= stree.depth(*it)) {

                            uint resize = bins.size()*2;
                            while (resize<= stree.depth(*it)) {
                                resize*=2;
                            }
                            bins.resize(resize);
                        }
                        bins[stree.depth(*it)].push_back(stree.id(*it));
                        node_counter++;
                    }
                }
            });
            node_begins.resize(node_counter);

            uint nts_number = 1 ;
            StatPhase::wrap("Iterate over Node Bins", [&]{
                //iterate node bins top down
                DLOG(INFO)<<"iterate over Node Bins";
                for(uint i = bins.size()-1; i>=min_lrf; i--){

                    //iterate over ids in bin:
                    while(!bins[i].empty()){
                        uint id = bins[i].back();
                        bins[i].pop_back();
                        node_type node = stree.inv_id(id);
                        uint no_leaf_id = id - stree.size();

                        //get bps of node

                        if(node_begins[no_leaf_id].empty()){
                            //get leaves or merge child vectors
                            std::vector<uint> offsets;
                            std::vector<uint> leaf_bps;

                            for (auto & child : stree.children(node)) {
                                if(stree.is_leaf(child)){


                                        leaf_bps.push_back(stree.csa[stree.lb(child)]);


                                } else {
                                    int child_id = stree.id(child) - stree.size();
                                    if(!node_begins[child_id ].empty()){
                                        //append child list, remember offset
                                        offsets.push_back(node_begins[no_leaf_id].size());


                                        node_begins[no_leaf_id].insert(node_begins[no_leaf_id].end(),node_begins[child_id ].begin(), node_begins[child_id].end());

                                        node_begins[child_id] = std::vector<uint>();

                                    }
                                }
                            }
                            std::sort(leaf_bps.begin(), leaf_bps.end());

                            offsets.push_back(node_begins[no_leaf_id].size());
                            node_begins[no_leaf_id].insert(node_begins[no_leaf_id].end(),leaf_bps.begin(), leaf_bps.end());

                            //inplace merge with offset
                            for(uint k = 0; k < offsets.size()-1; k++){
                                std::inplace_merge(node_begins[no_leaf_id].begin(), node_begins[no_leaf_id].begin()+ offsets[k], node_begins[no_leaf_id].begin()+ offsets[k+1]);

                            }
                            //now inplace merge to end
                            std::inplace_merge(node_begins[no_leaf_id].begin(), node_begins[no_leaf_id].begin()+ offsets.back(), node_begins[no_leaf_id].end());



                        }
                        //if still empty, because everything is substituted...
                        if(node_begins[no_leaf_id].empty()){
                            continue;
                        }
                        //check if viable lrf, else sort higher!
                        if((node_begins[no_leaf_id].size()>=2)){

                            if (( (uint)( node_begins[no_leaf_id].back() - node_begins[no_leaf_id].front() )) >= i ){

                                //greedily iterate over occurences
                                signed long last =  0 - (long) i;
                                std::vector<uint> first_layer_viable;
                                std::vector<uint> second_layer_viable;
                                for(uint occurence : node_begins[no_leaf_id]){
                                    //check for viability
                                    if( (last+i <= (long) occurence)){
                                        if(fl_offsets[occurence] == 0){
                                            if(fl_offsets[occurence + i -1] == 0){
                                                //Position is firs layer viable
                                                first_layer_viable.push_back(occurence);
                                                last= occurence;
                                            }
                                        } else {
                                            //find nts number of symbol that corresponds to substitued occ
                                            uint parent_nts= first_layer_nts[ occurence - (fl_offsets[occurence] -1) ];
                                            auto nts = non_terminal_symbols[parent_nts-1];
                                            //if length of parent nts is greater than current len + offset
                                            if(nts.second >=fl_offsets[occurence]-1 + i ){
                                                second_layer_viable.push_back(occurence);
                                            }
                                        }

                                    }

                                }


                                //and substitute

                                //if at least 2 first level layer occs viable:
                                if(first_layer_viable.size() >=1 &&(first_layer_viable.size() + second_layer_viable.size() >= 2) ) {
                                    std::pair<uint,uint> nts = std::make_pair(first_layer_viable.front(), i);
                                    non_terminal_symbols.push_back(nts);

                                    //iterate over vector, make first layer unviable:
                                    for(uint occ : first_layer_viable){
                                        first_layer_nts[occ]= nts_number;

                                        for(uint nts_length =0; nts_length < i; nts_length++){
                                            fl_offsets[occ + nts_length] = nts_length+1;
                                        }
                                    }



                                    for(uint sl_occ :second_layer_viable){
                                        uint parent_nts= first_layer_nts[ sl_occ - (fl_offsets[sl_occ] -1) ];

                                        auto parent_sym = non_terminal_symbols[parent_nts-1];
                                        uint parent_start= parent_sym.first;
                                        uint sl_start = (parent_start + fl_offsets[sl_occ] -1);
                                        uint sl_end = sl_start+i-1;
                                        if(second_layer_dead[sl_start] == (uint)0 && second_layer_dead[sl_end] == (uint)0){

                                            second_layer_nts[sl_start]=nts_number;

                                            for(uint dead = sl_start; dead<=sl_end;dead++){
                                                second_layer_dead[dead]=1;
                                            }
                                        }
                                    }

                                    //raise nts number:
                                    nts_number++;

                                }



                            } else {
                                if(exact){
                                    //readd node if lrf shorter
                                    uint min_shorter = node_begins[no_leaf_id].back()   - node_begins[no_leaf_id].front();
                                    //check if parent subs this lrf
                                    node_type parent = stree.parent(node);
                                    uint depth = stree.depth(parent);
                                    if(depth < (min_shorter)){
                                        //just re-add node, if the possible replaceable lrf is longer than dpeth of parent node
                                        bins[min_shorter].push_back(stree.id(node));
                                    }


                                }
                                continue;

                            }
                        }
                    }
                }
            });

        });

        DLOG(INFO)<<"Computing symbol depth";

        IntVector<uint> nts_depth(non_terminal_symbols.size(), 0);

        for(uint nts_num =0; nts_num<non_terminal_symbols.size(); nts_num++){
            auto symbol = non_terminal_symbols[nts_num];
            uint cur_depth = nts_depth[nts_num];

            for(uint pos = symbol.first; pos < symbol.second + symbol.first ; pos++){
                if(second_layer_nts[pos]>0){

                    uint symbol_num = second_layer_nts[pos] -1;
                    if(nts_depth[symbol_num]< cur_depth+1){
                        nts_depth[symbol_num]= cur_depth+1;
                    }
                }
            }
        }

        DLOG(INFO)<<"Computing done";

        std::sort(nts_depth.begin(), nts_depth.end());
        if(nts_depth.size()>0){

            uint max_depth = nts_depth[nts_depth.size()-1];

            DLOG(INFO)<<"Max CFG Depth: "<< max_depth;
            DLOG(INFO)<<"Number of CFG rules: "<< non_terminal_symbols.size();

            if(nts_depth.size()>=4){
                uint quarter = nts_depth.size() /4;

                StatPhase::log("25 \% quantil CFG Depth", nts_depth[quarter -1]);
                StatPhase::log("50 \% quantil CFG Depth", nts_depth[(2*quarter) -1]);
                StatPhase::log("75 \% quantil CFG Depth", nts_depth[(3*quarter) -1]);

            }
            StatPhase::log("Max CFG Depth", max_depth);
        }

        //input size end
        }

        StatPhase::log("Number of CFG rules", non_terminal_symbols.size());

        std::stringstream literals;

        for(uint position = 0; position< in.size(); position++){
            if(fl_offsets[position]==0){
                literals << in[position];
            }
        }
        for(uint nts_num = 0; nts_num<non_terminal_symbols.size(); nts_num++){

            auto symbol = non_terminal_symbols[nts_num];

            for(uint pos = symbol.first; pos < symbol.second + symbol.first; pos++){
                if(second_layer_nts[pos] == 0 && pos < in.size()){
                    literals<< in[pos];

                }
            }
        }


        StatPhase::wrap("Encoding Comp", [&]{
            // encode dictionary:
            DLOG(INFO) << "encoding dictionary symbol sizes ";

            std::shared_ptr<BitOStream> bitout = std::make_shared<BitOStream>(output);
            typename literal_coder_t::Encoder lit_coder(
                config().sub_config("lfs2_lit_coder"),
                bitout,
                ViewLiterals(literals.str())
            );
            typename len_coder_t::Encoder len_coder(
                config().sub_config("lfs2_len_coder"),
                bitout,
                NoLiterals()
            );

            //encode lengths:
            DLOG(INFO)<<"number nts: " << non_terminal_symbols.size();
            Range intrange (0, UINT_MAX);
            //encode first length:
            if(non_terminal_symbols.size()>=1){
                auto symbol = non_terminal_symbols[0];
                uint last_length=symbol.second;
                //Range for encoding nts number
                Range s_length_r (0,last_length);
                len_coder.encode(last_length,intrange);
                //encode delta length  of following symbols
                for(uint nts_num = 1; nts_num < non_terminal_symbols.size(); nts_num++){
                    symbol = non_terminal_symbols[nts_num];
                    len_coder.encode(last_length-symbol.second,s_length_r);
                    last_length=symbol.second;

                }
                //encode last length, to have zero length last
                len_coder.encode(symbol.second,s_length_r);
            }else {
                len_coder.encode(0ULL,intrange);

            }
            Range dict_r(0, non_terminal_symbols.size());


            long buf_size = bitout->stream().tellp();

            StatPhase::log("Bytes Length Encoding", buf_size);
           DLOG(INFO)<<"Bytes Length Encoding: "<< buf_size;


            DLOG(INFO) << "encoding dictionary symbols";
            uint dict_literals=0;

            // encode dictionary strings, backwards, to directly decode strings:
            if(non_terminal_symbols.size()>=1){
                std::pair<uint,uint> symbol;
                for(long nts_num =non_terminal_symbols.size()-1; nts_num >= 0; nts_num--){

                    symbol = non_terminal_symbols[nts_num];

                    for(uint pos = symbol.first; pos < symbol.second + symbol.first ; pos++){
                        if(second_layer_nts[pos] > 0){
                            lit_coder.encode(1, bit_r);
                            lit_coder.encode(second_layer_nts[pos], dict_r);
                            auto symbol = non_terminal_symbols[second_layer_nts[pos] -1];



                            pos += symbol.second - 1;

                        } else {
                            lit_coder.encode(0, bit_r);
                            lit_coder.encode(in[pos],literal_r);
                            dict_literals++;

                        }
                    }

                }
            }

             uint literals=0;

            buf_size = long(bitout->stream().tellp()) - buf_size;
            StatPhase::log("Bytes Non-Terminal Symbol Encoding", buf_size);


            DLOG(INFO)<<"Bytes Non-Terminal Symbol Encoding: "<< buf_size;

            //encode start symbol

            DLOG(INFO)<<"encode start symbol";
            for(uint pos = 0; pos < in.size(); pos++){
                if(first_layer_nts[pos]>0){
                    lit_coder.encode(1, bit_r);
                    lit_coder.encode(first_layer_nts[pos], dict_r);
                    auto symbol = non_terminal_symbols[first_layer_nts[pos] -1];

                    pos += symbol.second - 1;
                } else {
                    lit_coder.encode(0, bit_r);
                    lit_coder.encode(in[pos],literal_r);
                    literals++;
                }
            }

            buf_size = long(bitout->stream().tellp()) - buf_size;
            StatPhase::log("Bytes Start Symbol Encoding", buf_size);

            DLOG(INFO)<<"Bytes Start Symbol Encoding: "<< buf_size;

            StatPhase::log("Literals in Dictionary", dict_literals);

             StatPhase::log("Literals in Start Symbol", literals);
             StatPhase::log("Literals in Input", in.size());

             double literal_percent = ((double)dict_literals + (double)literals)/ (double)in.size();
             StatPhase::log("Literals Encoding / Literals Input", literal_percent);


            DLOG(INFO)<<"encoding done";

        });
    }

    inline std::unique_ptr<Decompressor> decompressor() const override {
        return Algorithm::instance<
            LFS2Decompressor<literal_coder_t, len_coder_t>>();
    }
};

//namespaces closing
}}
