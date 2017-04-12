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


    inline virtual std::vector<int> select_starting_positions(int node_id, int length){

        node_type node = stree.inv_id(node_id);

        uint offset = stree.csa[stree.lb(node)];
        //uint fac_length = stree.depth(node);


        std::vector<int> beginning_positions;

        if(stree.size(node)>=2){

            //iterate over corresponding sa and find min and max
            offset = stree.lb(node);
            int min = stree.csa[offset];
            int max = stree.csa[offset];
            //int min_pos = offset;
            //int max_pos = offset;
            for(uint c = 0;c<stree.size(node); c++){
                int val = stree.csa[c+offset];
                beginning_positions.push_back(val);

                if(min > val){
                    min = val;
                    //   min_pos = offset+c;
                }
                if(max < val){
                    max = val;
                    //  max_pos = offset+c;
                }


            }
            int dif = max -min;
            if(dif < length){

                return std::vector<int>();
            }

            std::sort(beginning_positions.begin(), beginning_positions.end());
        }

        std::vector<int> selected_starting_positions;
        //select occurences greedily non-overlapping:
        selected_starting_positions.reserve(beginning_positions.size());

        int last =  0-length;
        int current;
        int shorter_count = 0;
        int min_shorter = length;
        for (auto it=beginning_positions.begin(); it!=beginning_positions.end(); ++it){

            current = *it;
            if(last+length <= current && !dead_positions[current] && !dead_positions[current+length-1]){



                selected_starting_positions.push_back(current);
                last = current;

            }

            if(!dead_positions[current] && dead_positions[current+length-1]){
                //Some replaceable lrf at beginning
                for(int i =1; i < length; i++){
                    if( ! (dead_positions[current+length-i-1]) ){
                        min_shorter = std::min(min_shorter, i);
                        shorter_count++;
                        break;

                    }
                }
            }

        }

        if(shorter_count>0){



            if(length-min_shorter >= (int)min_lrf){
                bins[length-min_shorter].push_back(node_id);
            }
        }
        return selected_starting_positions;
    }

    //(position in text, non_terminal_symbol_number, length_of_symbol);
    typedef std::tuple<uint,uint,uint> non_term;
    typedef std::vector<non_term> non_terminal_symbols;
    typedef std::vector<std::pair<uint,uint>> rules;





    BitVector dead_positions;

    //could be node_type
    std::vector<std::vector<uint> > bins;

    //vector of tuples, associates id to tuple
    //tuple:: min_bp, max_bp, card
    typedef std::tuple<int,int,int> n_tpl;
    typedef std::vector< n_tpl > tuple_vector;
    tuple_vector node_tuples;

public:

    using Algorithm::Algorithm; //import constructor

    inline static Meta meta() {
        Meta m("lfs_comp", "sim_st");
        return m;
    }


    inline void compute_rules(io::InputView & input, rules & dictionary, non_terminal_symbols & nts_symbols){

        //build suffixtree
        DLOG(INFO)<<"build suffixtree";


        dead_positions = BitVector(input.size(), 0);




        StatPhase::wrap("Constructing ST", [&]{
             sdsl::construct_im(stree, (const char*) input.data(), 1);
        });



        StatPhase::wrap("Computing LRF", [&]{
            DLOG(INFO)<<"computing lrf";

            //array of vectors for bins of nodes with string depth


            bins.resize(stree.size()+1);



            uint node_counter = 0;

            typedef sdsl::cst_bfs_iterator<cst_t> iterator;
            iterator begin = iterator(&stree, stree.root());
            iterator end   = iterator(&stree, stree.root(), true, true);

            StatPhase::wrap("Iterate over ST", [&]{
                DLOG(INFO)<<"iterate st";

                for (iterator it = begin; it != end; ++it) {
                    bins[stree.depth(*it)].push_back(stree.id(*it));
                    node_counter++;
                }
            });



            node_tuples.resize(node_counter);

            uint nts_number =0;
            StatPhase::wrap("Iterate over Node Bins", [&]{
                DLOG(INFO)<<"iterate node bins";

                for(uint i = bins.size()-1; i>=min_lrf; i--){
                    auto bin_it = bins[i].begin();
                    while (bin_it!= bins[i].end()){
                        node_type node = stree.inv_id(*bin_it);
                        n_tpl cur_tpl;
                        if(stree.is_leaf(node)){
                            int bp = stree.csa[stree.lb(node)];
                            cur_tpl = std::make_tuple(bp,bp,1);
                            node_tuples[*bin_it] = cur_tpl;

                            //DLOG(INFO)<<"new tuple for id: "<<*bin_it<< " <"<<bp<<","<<bp<<",1>";

                        }
                        else {
                            int min = stree.size();
                            int max = 0;
                            int card = 0;
                            for (auto& child: stree.children(node)) {
                                int child_id = stree.id(child);
                                n_tpl child_tpl = node_tuples[child_id];
                                min = std::min(min, std::get<0>(child_tpl));
                                max = std::max(max, std::get<1>(child_tpl));
                                card += std::get<2>(child_tpl);
                            }

                            cur_tpl = std::make_tuple(min,max,card);
                            //DLOG(INFO)<<"new tuple for id: "<<*bin_it<< " <"<<min<<","<<max<<","<<card<<">";
                            node_tuples[*bin_it] = cur_tpl;

                        }
                        //check tuple
                        if(!(std::get<2>(cur_tpl)>=2) && !( (std::get<1>(cur_tpl) - std::get<0>(cur_tpl)) >= i )){

                            bin_it++;
                            continue;
                        }
                            //do this


                           //Add new rule
                            //and add new non-terminal symbols
                            std::vector<int> selected_bp = select_starting_positions(*bin_it, i);
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
                                //typedef std::tuple<uint,uint,uint> non_term;
                                non_term nts = std::make_tuple(*bp_it, nts_number, i);
                                nts_symbols.push_back(nts);
                                //typedef std::vector<non_term> non_terminal_symbols;
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
