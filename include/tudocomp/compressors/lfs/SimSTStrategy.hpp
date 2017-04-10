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

    inline virtual std::vector<int> select_starting_positions(std::vector<int> starting_positions, int length){
        std::vector<int> selected_starting_positions;
        //select occurences greedily non-overlapping:
        selected_starting_positions.reserve(starting_positions.size());

        int last =  0-length;
        int current;
        for (auto it=starting_positions.begin(); it!=starting_positions.end(); ++it){

            current = *it;
            //DLOG(INFO) << "checking starting position: " << current << " length: " << top.first << "last " << last;
            if(last+length <= current && !dead_positions[current] && !dead_positions[current+length-1]){

                selected_starting_positions.push_back(current);
                last = current;

            }

        }
        return selected_starting_positions;
    }

    //(position in text, non_terminal_symbol_number, length_of_symbol);
    typedef std::tuple<uint,uint,uint> non_term;
    typedef std::vector<non_term> non_terminal_symbols;
    typedef std::vector<std::pair<uint,uint>> rules;



    //sdsl::t_csa sa =sdsl::csa_uncompressed<>
    typedef sdsl::cst_sct3< sdsl::csa_bitcompressed<> > cst_t;
    cst_t stree;


    BitVector dead_positions;


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


        typedef sdsl::bp_interval<long unsigned int> node_type;


        StatPhase::wrap("Constructing ST", [&]{
             sdsl::construct_im(stree, (const char*) input.data(), 1);
        });



        StatPhase::wrap("Computing LRF", [&]{

            //array of vectors for bins of nodes with string depth

            //could be node_type
            std::vector<std::vector<uint> > bins;
            bins.resize(stree.size()+1);

            uint node_counter = 0;

            typedef sdsl::cst_bfs_iterator<cst_t> iterator;
            iterator begin = iterator(&stree, stree.root());
            iterator end   = iterator(&stree, stree.root(), true, true);

            StatPhase::wrap("Iterate over ST", [&]{

                for (iterator it = begin; it != end; ++it) {
                    bins[stree.depth(*it)].push_back(stree.id(*it));
                    node_counter++;
                }
            });




            uint nts_number =0;
            StatPhase::wrap("Iterate over Node Bins", [&]{

                for(uint i = bins.size()-1; i>=min_lrf; i--){
                    auto bin_it = bins[i].begin();
                    while (bin_it!= bins[i].end()){

                        node_type node = stree.inv_id(*bin_it);

                        uint offset = stree.csa[stree.lb(node)];
                        uint fac_length = stree.depth(node);

                        bin_it++;

                        if(stree.size(node)>=2){

                            //iterate over corresponding sa and find min and max
                            offset = stree.lb(node);
                            int min = stree.csa[offset];
                            int max = stree.csa[offset];
                            //int min_pos = offset;
                            //int max_pos = offset;
                            std::vector<int> beginning_positions;
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
                            uint dif = max -min;
                            if(dif < stree.depth(node)){
                                continue;
                            }

                            std::sort(beginning_positions.begin(), beginning_positions.end());


                            //Add new rule



                            //and add new non-terminal symbols
                            std::vector<int> selected_bp = select_starting_positions(beginning_positions, stree.depth(node));
                            if(! (selected_bp.size() >=2) ){
                                continue;
                            }

                            //vector of text position, length
                            std::pair<uint,uint> rule = std::make_pair(selected_bp.at(0), fac_length);


                            dictionary.push_back(rule);

                            //iterate over selected pos, add non terminal symbols
                            for(auto bp_it = selected_bp.begin(); bp_it != selected_bp.end(); bp_it++){
                                //(position in text, non_terminal_symbol_number, length_of_symbol);
                                //typedef std::tuple<uint,uint,uint> non_term;
                                non_term nts = std::make_tuple(*bp_it, nts_number, fac_length);
                                nts_symbols.push_back(nts);
                                //typedef std::vector<non_term> non_terminal_symbols;
                                //mark as used
                                for(uint pos = 0; pos<=fac_length;pos++){
                                    dead_positions[pos+ *bp_it] = 1;
                                }
                            }
                            nts_number++;
                        }

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
