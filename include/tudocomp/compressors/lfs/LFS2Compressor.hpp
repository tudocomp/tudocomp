#pragma once

#include <vector>
#include <tuple>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/io.hpp>


#include <tudocomp/compressors/lfs/EncodeStrategy.hpp>
#include <tudocomp/coders/BitCoder.hpp>


#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp_stat/StatPhase.hpp>


#include <sdsl/suffix_trees.hpp>

namespace tdc {
namespace lfs {


template<uint min_lrf = 2 >
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
        Meta m("compressor", "lfs2_comp",
            "This is an implementation of the longest first substitution compression scheme, type 2.");
        return m;
    }


    inline LFS2Compressor(Env&& env):
        Compressor(std::move(env))
    {
        DLOG(INFO) << "Compressor lfs2 instantiated";
    }
    inline virtual void compress(Input& input, Output& output) override {

        auto in = input.as_view();

        //create vectors:
        first_layer_nts = IntVector<uint>(input.size(), 0);
        fl_offsets = IntVector<uint>(input.size(), 0);
        second_layer_nts = IntVector<uint>(input.size(), 0);
        second_layer_dead = BitVector(input.size(), 0);





        StatPhase::wrap("Constructing ST", [&]{
            uint size =  in.size();
            //remove sentinel because sdsl cant handle that
            if(in[size-1] == 0){
                size--;
            }
            std::string in_string ((const char*) in.data(), size);
            sdsl::construct_im(stree, in_string , 1);
        });



        StatPhase::wrap("Computing LRF", [&]{
            bins.resize(stree.size());
            uint node_counter = 0;

            typedef sdsl::cst_bfs_iterator<cst_t> iterator;
            iterator begin = iterator(&stree, stree.root());
            iterator end   = iterator(&stree, stree.root(), true, true);

            StatPhase::wrap("Iterate over ST", [&]{
                DLOG(INFO)<<"iterate st";

                for (iterator it = begin; it != end; ++it) {

                    if(!stree.is_leaf(*it)){
                        bins[stree.depth(*it)].push_back(stree.id(*it));
                        node_counter++;
                    }
                }
            });
            node_begins.resize(node_counter);

            uint nts_number = 1 ;
            StatPhase::wrap("Iterate over Node Bins", [&]{
                //iterate node bins top down
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
                                DLOG(INFO)<<"iterating occs, lrf_len: " << i;
                                for(uint occurence : node_begins[no_leaf_id]){
                                    //check for viability
                                    DLOG(INFO)<<"checking occ: " << occurence;
                                    DLOG(INFO)<<"last chosen occ: " << last;
                                    if( (last+i <= (long) occurence)){
                                        DLOG(INFO)<<"last no collide";
                                        if(fl_offsets[occurence] == 0){
                                            DLOG(INFO)<<"first pos of occ ok";
                                            if(fl_offsets[occurence + i -1] == 0){
                                                //Position is firs layer viable
                                                DLOG(INFO)<<"occ viable: " << occurence;
                                                first_layer_viable.push_back(occurence);
                                                last= occurence;
                                            }
                                        } else {
                                            //find nts number of symbol that corresponds to substitued occ
                                            uint parent_nts= first_layer_nts[ occurence - (fl_offsets[occurence] -1) ];
                                            DLOG(INFO)<<"sl maybe viable: " << occurence << " parent nts: " << parent_nts;
                                            auto nts = non_terminal_symbols[parent_nts-1];
                                            //if length of parent nts is greater than current len + offset
                                            DLOG(INFO)<<"offset: "<<fl_offsets[occurence] <<  " len: " << nts.second;
                                            if(nts.second >fl_offsets[occurence]-1 + i ){
                                                second_layer_viable.push_back(occurence);
                                                DLOG(INFO)<<"sl added";
                                            }
                                        }


                                        //
                                    }

                                }


                                //and substitute

                                //if at least 2 first level layer occs viable:
                                if(first_layer_viable.size()>=2){
                                    DLOG(INFO)<<"adding new nts";
                                    std::pair<uint,uint> nts = std::make_pair(first_layer_viable.front(), i);
                                    non_terminal_symbols.push_back(nts);

                                    //iterate over vector, make first layer unviable:
                                    for(uint occ : first_layer_viable){
                                        DLOG(INFO)<<"fl note nts";
                                        first_layer_nts[occ]= nts_number;

                                        DLOG(INFO)<<"fl offset to nts";
                                        for(uint nts_length =0; nts_length < i; nts_length++){
                                            fl_offsets[occ + nts_length] = nts_length+1;
                                        }
                                    }

                                    //raise nts number:
                                    nts_number++;

                                    for(uint sl_occ :second_layer_viable){
                                        DLOG(INFO)<<"second layer viable: " << sl_occ;
                                    }

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
                        DLOG(INFO)<<"state of arrays: ";
                        std::stringstream ss;
                        for(uint x = 0; x < in.size(); x++){


                            ss << in[x] ;
                            ss << " " ;
                        }
                        DLOG(INFO)<< ss.str();

                        ss.str("");
                        ss.clear();
                        for(uint x = 0; x < first_layer_nts.size(); x++){


                            ss << first_layer_nts[x] ;
                            ss << " " ;
                        }
                        DLOG(INFO)<< ss.str();

                        ss.str("");
                        ss.clear();

                        for(uint x = 0; x < fl_offsets.size(); x++){


                            ss << fl_offsets[x] ;
                            ss << " " ;
                        }
                        DLOG(INFO)<< ss.str();
                    }

                }

            });

        });

        StatPhase::wrap("Encoding Comp", [&]{

        });



    }

    inline virtual void decompress(Input& input, Output& output) override {

    }

};

//namespaces closing
}}
