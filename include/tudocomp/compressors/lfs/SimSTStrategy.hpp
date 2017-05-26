#pragma once

#include <vector>
#include <tuple>
#include <array>

#include <tudocomp/util.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/Algorithm.hpp>


#include <tudocomp/ds/IntVector.hpp>


//#include <sdsl/suffixarrays.hpp>

#include <sdsl/suffix_trees.hpp>
//#include <sdsl/suffix_arrays.hpp>
//#include <sdsl/csa_bitcompressed.hpp>
//#include <sdsl/csa_uncompressed.hpp>




namespace tdc {
namespace lfs {

template<uint min_lrf = 2 >
class SimSTStrategy : public Algorithm {
private:

    typedef sdsl::bp_interval<long unsigned int> node_type;

    //sdsl::t_csa sa =sdsl::csa_uncompressed<>
    typedef sdsl::cst_sct3< sdsl::csa_bitcompressed<> > cst_t;
    cst_t stree;




    // greedily select starting positions and delete them from corresponding vector
    inline virtual std::vector<uint> select_starting_positions(int node_id, int length){

        std::vector<uint> selected_starting_positions;
        std::vector<uint> not_selected_starting_positions;

        //select occurences greedily non-overlapping:
        selected_starting_positions.reserve(node_begins[node_id].size());


        not_selected_starting_positions.reserve(node_begins[node_id].size());

        long last =  0-length-1;
        uint current;

        long min_shorter = 1;
        for (auto it=node_begins[node_id].begin(); it!=node_begins[node_id].end(); it++){

            current = *it;
            if((last + length <= current )&& !dead_positions[current] && !dead_positions[current+length-1]){
                selected_starting_positions.push_back(current);
                last = current;

            } else {
                not_selected_starting_positions.push_back(current);
            }


            if(current < dead_positions.size() && !dead_positions[current] && dead_positions[current+length-1]){


                while((current+min_shorter < (long) dead_positions.size()) && !dead_positions[current+min_shorter]){
                    min_shorter++;
                }

            }

        }


        if(min_shorter < length){
            node_type node = stree.inv_id(node_id + stree.size());

            if(min_shorter >= (int) min_lrf){
                //check if parent node is shorter

                node_type parent = stree.parent(node);
                uint depth = stree.depth(parent);
                if(depth < (uint)(min_shorter)){

                    //just re-add node, if the possible replaceable lrf is longer than dpeth of parent node
                    bins[min_shorter].push_back(node_id + stree.size());
                }
            }
        }
        if(selected_starting_positions.size()>=2){
            node_begins[node_id].assign(not_selected_starting_positions.begin(), not_selected_starting_positions.end());
            return selected_starting_positions;
        } else {
            return std::vector<uint>();
        }
    }

    //(position in text, non_terminal_symbol_number, length_of_symbol);
    typedef std::tuple<uint,uint,uint> non_term;
    typedef std::vector<non_term> non_terminal_symbols;
    typedef std::vector<std::pair<uint,uint>> rules;





    BitVector dead_positions;

    //could be node_type
    std::vector<std::vector<uint> > bins;


    std::vector<std::vector<uint> > node_begins;

public:

    using Algorithm::Algorithm; //import constructor

    inline static Meta meta() {
        Meta m("lfs_comp", "sim_st");
       // m.needs_sentinel_terminator();
        return m;
    }


    inline void compute_rules(io::InputView & input, rules & dictionary, non_terminal_symbols & nts_symbols){

        //build suffixtree
        DLOG(INFO)<<"build suffixtree";



        dead_positions = BitVector(input.size(), 0);


        StatPhase::wrap("Constructing ST", [&]{
            uint size =  input.size();
            //remove sentinel because sdsl cant handle that (at last pos)
            //this is just to handle test cases
            if(input[size-1] == 0){
                size--;
            }
            std::string in_string ((const char*) input.data(), size);
            sdsl::construct_im(stree, in_string , 1);


        });




        StatPhase::wrap("Computing LRF", [&]{
            DLOG(INFO)<<"computing lrf";

            //array of vectors for bins of nodes with string depth


            bins.resize(200);
            uint node_counter = 0;

            typedef sdsl::cst_bfs_iterator<cst_t> iterator;
            iterator begin = iterator(&stree, stree.root());
            iterator end   = iterator(&stree, stree.root(), true, true);

            StatPhase::wrap("Iterate over ST", [&]{
                DLOG(INFO)<<"iterate st";

                for (iterator it = begin; it != end; ++it) {

                    if(!stree.is_leaf(*it)){
                        if(bins.size() <= stree.depth(*it) ){
                            bins.resize(bins.size()*2);
                        }
                        bins[stree.depth(*it)].push_back(stree.id(*it));
                        node_counter++;
                    }
                }
            });

            node_begins.resize(node_counter);


            //node_tuples.resize(node_counter);

            uint nts_number =0;
            StatPhase::wrap("Iterate over Node Bins", [&]{
                for(uint i = bins.size()-1; i>=min_lrf; i--){
                    auto bin_it = bins[i].begin();
                    while (bin_it!= bins[i].end()){
                        node_type node = stree.inv_id(*bin_it);
                        uint no_leaf_id = *bin_it - stree.size();


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

                                    }
                                }
                            }
                            std::sort(leaf_bps.begin(), leaf_bps.end());
                            offsets.push_back(node_begins[no_leaf_id].size());
                            node_begins[no_leaf_id].insert(node_begins[no_leaf_id].end(),leaf_bps.begin(), leaf_bps.end());
                            //offsets.push_back(node_begins[no_leaf_id].size());
                            //inplace merge with offset
                            for(uint k = 0; k < offsets.size()-1; k++){
                                std::inplace_merge(node_begins[no_leaf_id].begin(), node_begins[no_leaf_id].begin()+ offsets[k], node_begins[no_leaf_id].begin()+ offsets[k+1]);

                            }
                            //now inplace merge to end
                            std::inplace_merge(node_begins[no_leaf_id].begin(), node_begins[no_leaf_id].begin()+ offsets[offsets.size()-1], node_begins[no_leaf_id].end());

                            //sort bps of leaves
                          //  std::sort(node_begins[no_leaf_id].begin(), node_begins[no_leaf_id].end());


                        }
                        if(node_begins[no_leaf_id].empty()){

                            bin_it++;
                            continue;
                        }
                        //check tuple
                        if(!(node_begins[no_leaf_id].size()>=2) && !( (  (uint)( node_begins[no_leaf_id].back()   - node_begins[no_leaf_id].front() )) >= i )){
                            uint min_shorter = node_begins[no_leaf_id].back()   - node_begins[no_leaf_id].front();
                            //check if parent subs this lrf
                            node_type parent = stree.parent(node);
                            uint depth = stree.depth(parent);
                            if(depth < (min_shorter)){
                                //just re-add node, if the possible replaceable lrf is longer than dpeth of parent node
                                bins[min_shorter].push_back(stree.id(node));
                            }

                            bin_it++;
                            continue;
                        }
                        //do this
                        //Add new rule
                        //and add new non-terminal symbols
                        std::vector<uint> selected_bp = select_starting_positions(no_leaf_id, i);
                        if(! (selected_bp.size() >=2) ){
                            bin_it++;
                            continue;
                        }
                        //vector of text position, length
                        std::pair<uint,uint> rule = std::make_pair(selected_bp.at(0), i);
                        dictionary.push_back(rule);

                        //iterate over selected pos, add non terminal symbols
                        for(auto bp_it = selected_bp.begin(); bp_it != selected_bp.end(); bp_it++){
                            //(position in text, non_terminal_symbol_number, length_of_symbol);
                            non_term nts = std::make_tuple(*bp_it, nts_number, i);
                            nts_symbols.push_back(nts);
                            //mark as used
                            for(uint pos = 0; pos<i;pos++){
                                dead_positions[pos+ *bp_it] = 1;
                            }
                        }
                        nts_number++;

                        bin_it++;

                    }
                }

            });
        });

        StatPhase::wrap("Sorting occurences", [&]{

            DLOG(INFO) << "sorting occurences";
            std::sort(nts_symbols.begin(), nts_symbols.end());

        });
    }
};
}
}
