#pragma once

#include <vector>
#include <tuple>
#include <array>

#include <tudocomp/util.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/Algorithm.hpp>


#include <tudocomp/ds/IntVector.hpp>

#include <sdsl/cst_sct3.hpp>




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
        uint current;
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



    typedef sdsl::cst_sct3<> cst_t;
    cst_t stree;


    BitVector dead_positions;


public:

    using Algorithm::Algorithm; //import constructor

    inline static Meta meta() {
        Meta m("lfs_comp", "sim_st");
        return m;
    }


    inline void compute_rules(io::InputView & input, rules & dictionary, non_terminal_symbols & nts_symbols){
        //BitVector
       // dead_positions = BitVector(input.size(), 0);
        //DLOG(INFO)<< "dead_positions.size(): "<<dead_positions.size();

        //build suffixtree
        DLOG(INFO)<<"build suffixtree";


        dead_positions = BitVector(input.size(), 0);


        StatPhase::wrap("Constructing ST", [&]{
             sdsl::construct_im(stree, (const char*) input.data(), 1);
        });

       // stree.append_input(in);

        DLOG(INFO)<<"computing string depth";


        //array of vectors for bins of nodes with string depth
        std::vector<std::vector<int> > bins;
        bins.resize(stree.size()+1);

        uint node_counter = 0;

        typedef sdsl::cst_bfs_iterator<cst_t> iterator;
            iterator begin = iterator(&stree, stree.root());
            iterator end   = iterator(&stree, stree.root(), true, true);

            for (iterator it = begin; it != end; ++it) {
                //std::cout << stree.depth(*it) << "-[" << stree.lb(*it) << "," << stree.rb(*it) << "]  id:" << stree.id(*it) <<std::endl;
                //std::cout << "si leaf:"<< stree.is_leaf(*it) << std::endl;
                bins[stree.depth(*it)].push_back(stree.id(*it));
                node_counter++;
            }


        //min_lrf=2;
        for(int i = 0; i< stree.size(); i++){
            std::cout << i << "; ";
        }
        std::cout<<std::endl;
        for(int i = 0; i< stree.size(); i++){
            std::cout << stree.csa[i] << "; ";
        }
        std::cout<<std::endl;


        //std::fill(bins.begin(), bins.end(), std::vector<int>(5));
        std::cout << "size of tree: "<< stree.size()<<  " nodes: " << node_counter << std::endl;



        for(int i = bins.size()-1; i>=min_lrf; i--){
            auto bin_it = bins[i].begin();
            std::cout<< "string depth: "<<i<<std::endl;
            while (bin_it!= bins[i].end()){
               // std::cout<< *bin_it << std::endl;

                auto node = stree.inv_id(*bin_it);
               // std::cout << stree.depth(node) << "-[" << stree.lb(node) << "," << stree.rb(node) << "]  sa lb:" << stree.csa[stree.lb(node)] <<std::endl;
                std::cout<< "factor: ";
                uint offset = stree.csa[stree.lb(node)];
                for(uint c = 0; c<i; c++){
                    std::cout << input[c+offset];
                }
                std::cout<<std::endl;

                std::cout<<"left leaf: " << stree.csa[stree.lb(stree.leftmost_leaf(node))]<<
                           " right leaf: " << stree.csa[stree.lb(stree.rightmost_leaf(node))] <<
                           " size: "<< stree.size(node)<<
                           std::endl;

                bin_it++;

                if(stree.size(node)>=2){

                    //iterate over corresponding sa and find min and max
                    offset = stree.lb(node);
                    int min = stree.csa[offset];
                    int max = stree.csa[offset];
                    int min_pos = offset;
                    int max_pos = offset;
                    std::vector<int> beginning_positions;
                    for(int c = 0;c<stree.size(node); c++){
                        int val = stree.csa[c+offset];
                        beginning_positions.push_back(val);
                        if(min > val){
                            min = val;
                            min_pos = offset+c;
                        }
                        if(max < val){
                            max = val;
                            max_pos = offset+c;
                        }


                    }
                    int dif = max -min;
                    std::cout<< "first: " << min<< " last: "<<max<<" dif: " << dif << std::endl;


                    //Add new rule

                    //and add new non-terminal symbols
                    std::vector<int> selected_bp = select_starting_positions(beginning_positions, stree.depth(node));
                }

            }

        }




    }
};
}

}
