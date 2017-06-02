#pragma once

//std includes:
#include <vector>
#include <tuple>

//general includes:
#include <tudocomp/Compressor.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp_stat/StatPhase.hpp>

//sdsl include stree:
#include <sdsl/suffix_trees.hpp>


//includes encoding:
#include <tudocomp/io/BitIStream.hpp>
#include <tudocomp/io/BitOStream.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/coders/BitCoder.hpp>

#include <tudocomp/coders/EliasGammaCoder.hpp>
#include <tudocomp/coders/ASCIICoder.hpp>


namespace tdc {
namespace lfs {

//uint min_lrf = 2,
template<typename literal_coder_t = BitCoder, typename len_coder_t = EliasGammaCoder >
class LFS2Compressor : public Compressor {
private:




    //sdsl::t_csa sa =sdsl::csa_uncompressed<>
    //Suffix Tree type + st
    typedef sdsl::cst_sct3< sdsl::csa_bitcompressed<> > cst_t;
    cst_t stree;

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


    //
    //stores node_ids of corresponding factor length
    std::vector<std::vector<uint> > bins;


    //stores beginning positions corresponding to node_ids
    std::vector<std::vector<uint> > node_begins;


    typedef sdsl::bp_interval<long unsigned int> node_type;




public:

    inline static Meta meta() {
        Meta m("compressor", "lfs2",
            "This is an implementation of the longest first substitution compression scheme, type 2.");
        m.option("min_lrf").dynamic(5);
        m.option("lfs2_lit_coder").templated<literal_coder_t, BitCoder>("lfs2_lit_coder");
        m.option("lfs2_len_coder").templated<len_coder_t, EliasGammaCoder>("lfs2_len_coder");

        return m;
    }


    inline LFS2Compressor(Env&& env):
        Compressor(std::move(env))
    {
        DLOG(INFO) << "Compressor lfs2 instantiated";
    }
    inline virtual void compress(Input& input, Output& output) override {
        uint min_lrf = env().option("min_lrf").as_integer();

        auto in = input.as_view();

        //create vectors:
        first_layer_nts = IntVector<uint>(input.size(), 0);
        fl_offsets = IntVector<uint>(input.size(), 0);
        second_layer_nts = IntVector<uint>(input.size(), 0);
        second_layer_dead = BitVector(input.size(), 0);

        std::cerr<<"building stree"<<std::endl;





        StatPhase::wrap("Constructing ST", [&]{
            uint size =  in.size();
            //remove sentinel because sdsl cant handle that
            if(in[size-1] == 0){
                size--;
            }
            std::string in_string ((const char*) in.data(), size);
            sdsl::construct_im(stree, in_string , 1);
        });



        std::cerr<<"computing lrf"<<std::endl;
        StatPhase::wrap("Computing LRF", [&]{
            bins.resize(200);
            uint node_counter = 0;

            typedef sdsl::cst_bfs_iterator<cst_t> iterator;
            iterator begin = iterator(&stree, stree.root());
            iterator end   = iterator(&stree, stree.root(), true, true);

            StatPhase::wrap("Iterate over ST", [&]{
                DLOG(INFO)<<"iterate st";
                std::cerr<<"iterate st"<<std::endl;

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

            std::cerr<<"iterate st done"<<std::endl;
            uint nts_number = 1 ;
            StatPhase::wrap("Iterate over Node Bins", [&]{
                //iterate node bins top down
                DLOG(INFO)<<"iterate over Node Bins";
                for(uint i = bins.size()-1; i>=min_lrf; i--){

                    //iterate over ids in bin:
                    for(auto bin_it = bins[i].begin(); bin_it!= bins[i].end() ; bin_it++){
                        node_type node = stree.inv_id(*bin_it);
                        uint no_leaf_id = *bin_it - stree.size();

                        //get bps of node

                        if(node_begins[no_leaf_id].empty()){
                            //get leaves or merge child vectors
                            for (auto & child : stree.children(node)) {
                                if(stree.is_leaf(child)){

                                    node_begins[no_leaf_id].push_back(stree.csa[stree.lb(child)]);

                                } else {
                                    int child_id = stree.id(child) - stree.size();
                                    if(!node_begins[child_id ].empty()){
                                        node_begins[no_leaf_id].insert(node_begins[no_leaf_id].end(),node_begins[child_id ].begin(), node_begins[child_id].end());
                                        node_begins[child_id ].clear();
                                    }
                                }
                            }
                            std::sort(node_begins[no_leaf_id].begin(), node_begins[no_leaf_id].end());


                        }
                        //if still empty, because everything is substituted...
                        if(node_begins[no_leaf_id].empty()){
                           // bin_it++;
                            continue;
                        }
                        //check if viable lrf, else sort higher!
                        if((node_begins[no_leaf_id].size()>=2)){

                            if (( (uint)( node_begins[no_leaf_id].back() - node_begins[no_leaf_id].front() )) >= i ){

                                //greedily iterate over occurences
                                signed long last =  0 - (long) i;
                                std::vector<uint> first_layer_viable;
                                std::vector<uint> second_layer_viable;
                              //  DLOG(INFO)<<"iterating occs, lrf_len: " << i;
                                for(uint occurence : node_begins[no_leaf_id]){
                                    //check for viability
                                    if( (last+i <= (long) occurence)){
                                        if(fl_offsets[occurence] == 0){
                                       //     DLOG(INFO)<<"first pos of occ ok";
                                            if(fl_offsets[occurence + i -1] == 0){
                                                //Position is firs layer viable
                                        //        DLOG(INFO)<<"occ viable: " << occurence;
                                                first_layer_viable.push_back(occurence);
                                                last= occurence;
                                            }
                                        } else {
                                            //find nts number of symbol that corresponds to substitued occ
                                            uint parent_nts= first_layer_nts[ occurence - (fl_offsets[occurence] -1) ];
                                         //   DLOG(INFO)<<"sl maybe viable: " << occurence << " parent nts: " << parent_nts;
                                            auto nts = non_terminal_symbols[parent_nts-1];
                                            //if length of parent nts is greater than current len + offset
                                       //     DLOG(INFO)<<"offset: "<<fl_offsets[occurence] <<  " len: " << nts.second;
                                            if(nts.second >=fl_offsets[occurence]-1 + i ){
                                                second_layer_viable.push_back(occurence);
                                 //               DLOG(INFO)<<"sl viable, parent length: " << nts.second << ">= " <<fl_offsets[occurence]-1 + i;
                                            }
                                        }

                                    }

                                }


                                //and substitute

                                //if at least 2 first level layer occs viable:
                                // || (first_layer_viable.size() >=1 &&(first_layer_viable.size() + second_layer_viable.size() >= 2))
                                if(first_layer_viable.size() >=1 &&(first_layer_viable.size() + second_layer_viable.size() >= 2) ) {
                                 //   DLOG(INFO)<<"adding new nts";
                                    std::pair<uint,uint> nts = std::make_pair(first_layer_viable.front(), i);
                                    non_terminal_symbols.push_back(nts);

                                    //iterate over vector, make first layer unviable:
                                    for(uint occ : first_layer_viable){
                                      //  DLOG(INFO)<<"fl note nts";
                                        first_layer_nts[occ]= nts_number;

                                      //  DLOG(INFO)<<"fl offset to nts";
                                        for(uint nts_length =0; nts_length < i; nts_length++){
                                            fl_offsets[occ + nts_length] = nts_length+1;
                                        }
                                    }



                                    for(uint sl_occ :second_layer_viable){
                                        uint parent_nts= first_layer_nts[ sl_occ - (fl_offsets[sl_occ] -1) ];
                                        //parten start
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
                                //readd node if lrf shorter
                                uint min_shorter = node_begins[no_leaf_id].back()   - node_begins[no_leaf_id].front();
                                //check if parent subs this lrf
                                node_type parent = stree.parent(node);
                                uint depth = stree.depth(parent);
                                if(depth < (min_shorter)){
                                    //just re-add node, if the possible replaceable lrf is longer than dpeth of parent node
                                    bins[min_shorter].push_back(stree.id(node));
                                }
                             //   bin_it++;
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

           // DLOG(INFO)<<"encoding from "<<symbol.first<<" to "<<symbol.second + symbol.first;
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

           // nts_depth[quarter -1];
                StatPhase::log("25 \% quantil CFG Depth", nts_depth[quarter -1]);
                StatPhase::log("50 \% quantil CFG Depth", nts_depth[(2*quarter) -1]);
                StatPhase::log("75 \% quantil CFG Depth", nts_depth[(3*quarter) -1]);

            }
            StatPhase::log("Max CFG Depth", max_depth);
        }





        StatPhase::log("Number of CFG rules", non_terminal_symbols.size());


        std::cerr<<"encoding text"<<std::endl;

       // std::vector<uint8_t> byte_buffer;

        //Output out_with_buf = output.from_memory(byte_buffer);

        StatPhase::wrap("Encoding Comp", [&]{
            // encode dictionary:
            DLOG(INFO) << "encoding dictionary symbol sizes ";

            std::shared_ptr<BitOStream> bitout = std::make_shared<BitOStream>(output);
            typename literal_coder_t::Encoder lit_coder(
                env().env_for_option("lfs2_lit_coder"),
                bitout,
                NoLiterals()
            );
            typename len_coder_t::Encoder len_coder(
                env().env_for_option("lfs2_len_coder"),
                bitout,
                NoLiterals()
            );

            //encode lengths:
            DLOG(INFO)<<"number nts: " << non_terminal_symbols.size();
            Range intrange (0, UINT_MAX);//uint32_r
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
                len_coder.encode(0,intrange);

            }
            Range dict_r(0, non_terminal_symbols.size());


            long buf_size = bitout->tellp();

           // long alt_size = ;
            StatPhase::log("Bytes Length Encoding", buf_size);
           DLOG(INFO)<<"Bytes Length Encoding: "<< buf_size;
           // DLOG(INFO)<<"Bytes Length Encoding: "<< buf_size;


            DLOG(INFO) << "encoding dictionary symbols";
            uint literals=0;

            // encode dictionary strings, backwards, to directly decode strings:
            if(non_terminal_symbols.size()>=1){
                std::pair<uint,uint> symbol;
                for(long nts_num =non_terminal_symbols.size()-1; nts_num >= 0; nts_num--){
               //     DLOG(INFO)<<"nts_num: " << nts_num;
                    symbol = non_terminal_symbols[nts_num];

                   // DLOG(INFO)<<"encoding from "<<symbol.first<<" to "<<symbol.second + symbol.first;
                    for(uint pos = symbol.first; pos < symbol.second + symbol.first ; pos++){
                     //   DLOG(INFO)<<"pos: " <<pos;
                        if(second_layer_nts[pos] > 0){
                            lit_coder.encode(1, bit_r);
                            lit_coder.encode(second_layer_nts[pos], dict_r);
                            auto symbol = non_terminal_symbols[second_layer_nts[pos] -1];



                            pos += symbol.second - 1;
                        //    DLOG(INFO)<<"new pos "<< pos;

                        } else {
                        //    DLOG(INFO)<<"encoding literal: "<< in[pos];
                            lit_coder.encode(0, bit_r);
                            lit_coder.encode(in[pos],literal_r);
                            literals++;

                        }
                    }
                  //  DLOG(INFO)<<"symbol done";

                }
            }

             StatPhase::log("Literals in Dictionary", literals);
             literals=0;




            buf_size = bitout->tellp() - buf_size;
            StatPhase::log("Bytes Non-Terminal Symbol Encoding", buf_size);


            DLOG(INFO)<<"Bytes Non-Terminal Symbol Encoding: "<< buf_size;

            //encode start symbol

            DLOG(INFO)<<"encode start symbol";
            for(uint pos = 0; pos < in.size(); pos++){
                if(first_layer_nts[pos]>0){
                    lit_coder.encode(1, bit_r);
                    lit_coder.encode(first_layer_nts[pos], dict_r);
                    auto symbol = non_terminal_symbols[first_layer_nts[pos] -1];
                  //  DLOG(INFO)<<"old pos: "<<pos<<" len: " << symbol.second  <<" sl: " << first_layer_nts[pos];

                    pos += symbol.second - 1;
                   // DLOG(INFO)<<"new pos "<< pos;

                } else {
                   // DLOG(INFO)<<"encoding literal: "<< in[pos];
                    lit_coder.encode(0, bit_r);
                    lit_coder.encode(in[pos],literal_r);
                    literals++;


                }
            }

            buf_size = bitout->tellp() - buf_size;
            StatPhase::log("Bytes Start Symbol Encoding", buf_size);


            DLOG(INFO)<<"Bytes Start Symbol Encoding: "<< buf_size;

             StatPhase::log("Literals in Start Symbol", literals);
             StatPhase::log("Literals in Input", in.size());

            std::cerr<<"encoding done"<<std::endl;

            DLOG(INFO)<<"encoding done";

        });
    }

    inline virtual void decompress(Input& input, Output& output) override {

        DLOG(INFO) << "decompress lfs";
        std::shared_ptr<BitIStream> bitin = std::make_shared<BitIStream>(input);

        typename literal_coder_t::Decoder lit_decoder(
            env().env_for_option("lfs2_lit_coder"),
            bitin
        );
        typename len_coder_t::Decoder len_decoder(
            env().env_for_option("lfs2_len_coder"),
            bitin
        );
        Range int_r (0,UINT_MAX);

        uint symbol_length = len_decoder.template decode<uint>(int_r);
        Range slength_r (0, symbol_length);
        std::vector<uint> dict_lengths;
        dict_lengths.reserve(symbol_length);
        dict_lengths.push_back(symbol_length);
        while(symbol_length>0){

            uint current_delta = len_decoder.template decode<uint>(slength_r);
            symbol_length-=current_delta;
            dict_lengths.push_back(symbol_length);
        }
        dict_lengths.pop_back();

        DLOG(INFO)<<"decoded number of nts: "<< dict_lengths.size();



        std::vector<std::string> dictionary;
        uint dictionary_size = dict_lengths.size();

        Range dictionary_r (0, dictionary_size);


        dictionary.resize(dict_lengths.size());

        //uint length_of_symbol;
        std::stringstream ss;
        uint symbol_number;
        char c1;
        //c1 = lit_decoder.template decode<char>(literal_r);
       // DLOG(INFO)<<"sync symbol:: "<< c1;

        DLOG(INFO) << "reading dictionary";
        for(long i = dict_lengths.size() -1; i>=0 ;i--){

            ss.str("");
            ss.clear();
            long size_cur = (long) dict_lengths[i];
          //  DLOG(INFO)<<"decoding symbol: "<<i << "length: "  << size_cur;
            while(size_cur > 0){
                bool bit1 = lit_decoder.template decode<bool>(bit_r);

             //   DLOG(INFO)<<"bit indicator: "<<bit1<<" rem len: " << size_cur;

                if(bit1){
                    //bit = 1, is nts, decode nts num and copy
                    symbol_number = lit_decoder.template decode<uint>(dictionary_r); // Dekodiere Literal

              //      DLOG(INFO)<<"read symbol number: "<< symbol_number;
                    symbol_number-=1;

                    if(symbol_number < dictionary.size()){

                        ss << dictionary.at(symbol_number);
                        size_cur-= dict_lengths[symbol_number];
                    } else {
                   //     DLOG(INFO)<< "too large symbol: " << symbol_number;
                        break;
                    }

                } else {
                    //bit = 0, decode literal
                    c1 = lit_decoder.template decode<char>(literal_r); // Dekodiere Literal
                    size_cur--;

                    ss << c1;

                }

            }

            dictionary[i]=ss.str();
          //  DLOG(INFO)<<"add symbol: " << i << " str: "<< ss.str();


        }

        auto ostream = output.as_stream();
        //reading start symbol:
        while(!lit_decoder.eof()){
            //decode bit
            bool bit1 = lit_decoder.template decode<bool>(bit_r);
            char c1;
            uint symbol_number;
            // if bit = 0 its a literal
            if(!bit1){
                c1 = lit_decoder.template decode<char>(literal_r); // Dekodiere Literal

                ostream << c1;
            } else {
            //else its a non-terminal
                symbol_number = lit_decoder.template decode<uint>(dictionary_r); // Dekodiere Literal
                symbol_number-=1;

                if(symbol_number < dictionary.size()){

                    ostream << dictionary.at(symbol_number);
                } else {
                    DLOG(INFO)<< "too large symbol: " << symbol_number;
                }

            }
        }
    }

};

//namespaces closing
}}
