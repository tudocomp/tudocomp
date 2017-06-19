#pragma once

#include <vector>
#include <tuple>

#include <unordered_map>


#include <tudocomp/util.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/BinarySuffixTree.hpp>




namespace tdc {
namespace lfs {

class BSTStrategy : public Algorithm {
private:

    //(position in text, non_terminal_symbol_number, length_of_symbol);
    typedef std::tuple<uint,uint,uint> non_term;
    typedef std::vector<non_term> non_terminal_symbols;
    typedef std::vector<std::pair<uint,uint>> rules;


    BinarySuffixTree * stree;
    uint min_lrf;

    BitVector dead_positions;

    std::vector<std::vector<uint> > bins;

    //    DLOG(INFO)<<"input size: "<<stree.get_size();
      //  std::unordered_map<SuffixTree::STNode *, std::vector<uint> > beginning_positions;

    //stats
    uint node_count;
    uint max_depth;

   // std::vector<uint> child_sizes;


   // typedef  std::vector<std::pair<uint, SuffixTree::STNode*> > string_depth_vector;

/*
    inline virtual void compute_string_depth(uint node, uint str_depth){
        //resize if str depth grater than bins size
        uint child = stree.get_first_child(node);


        if(child != 0){
            while(str_depth>= bins.size()){
                bins.resize(bins.size()*2);
                std::cerr<<"resizing: "<<bins.size();
            }



            if(str_depth>max_depth){
                max_depth=str_depth;
            }
            node_count++;
            if(str_depth>0){

                bins[str_depth].push_back(node);
            }
            while (child != 0){
                compute_string_depth(child, str_depth);
                child=stree.get_next_sibling(child);
            }

        }

    }


*/



/*

    inline virtual std::vector<uint> select_starting_positions(SuffixTree::STNode * node, uint length){


        std::vector<uint> starting_positions = beginning_positions[node];
        std::vector<uint> selected_starting_positions;

        long min_shorter = 1;

        //select occurences greedily non-overlapping:
        selected_starting_positions.reserve(starting_positions.size());

        long last =  0- (long) length - 1;
        long current;
        for (auto it=starting_positions.begin(); it!=starting_positions.end(); ++it){

            current = (long) *it;
          //  DLOG(INFO) << "checking starting position: " << current << " length: " << length << "last " << last;
            if(last+length <= current && !dead_positions[current] && !dead_positions[current+length-1]){

                selected_starting_positions.push_back(current);
                last = current;

            }

            if(current < (long) dead_positions.size() && !dead_positions[current] && dead_positions[current+length-1]){


                while((current+min_shorter < (long) dead_positions.size()) && !dead_positions[current+min_shorter]){
                    min_shorter++;
                }

            }

        }
     //   DLOG(INFO)<<"min shorter: "<<min_shorter;

        if(min_shorter < length){

            if(min_shorter >= (int) min_lrf){
                //check if parent node is shorter
                SuffixTree::STInnerNode * inner = dynamic_cast<SuffixTree::STInnerNode *>(node);

                SuffixTree::STInnerNode * parent = inner->parent;
                uint depth = parent->string_depth;
             //   DLOG(INFO)<<"check shorter node";
                if(depth < (uint)(min_shorter)){
                  //  DLOG(INFO)<<"pushing back shorter node";

                    //just re-add node, if the possible replaceable lrf is longer than dpeth of parent node
                    bins[min_shorter].push_back(node);
                }
            }
        }


        return selected_starting_positions;
    }
    */

public:

    using Algorithm::Algorithm; //import constructor

    inline static Meta meta() {
        Meta m("lfs_comp", "bst");
        m.option("min_lrf").dynamic(2);
        return m;
    }


    inline void compute_rules(io::InputView & input, rules & dictionary, non_terminal_symbols & nts_symbols){
     //   BinarySuffixTree stree;
        min_lrf = env().option("min_lrf").as_integer();

        StatPhase::wrap("Constructing ST", [&]{
            stree = new BinarySuffixTree(input);
            StatPhase::log("Number of Nodes", stree->get_tree_size());
        });



        StatPhase::wrap("Computing String Depth", [&]{
            bins.resize(200);
            node_count=0;
            max_depth=0;
         //   compute_string_depth(0,0);
        });

        StatPhase::log("Number of inner Nodes", node_count);
        StatPhase::log("Max Depth inner Nodes", max_depth);
        DLOG(INFO)<<"max depth: "<<max_depth;



        /*

        StatPhase::wrap("Computing LRF Substitution", [&]{
            dead_positions = BitVector(stree.get_size(), 0);
            uint nts_number =0;

            beginning_positions.reserve(node_count);

            uint rd_counter =0;



            for(uint i = bins.size()-1; i>=min_lrf; i--){
                auto bin_it = bins[i].begin();
                while (bin_it!= bins[i].end()){

                    SuffixTree::STNode * node = *bin_it;

                    auto bp = beginning_positions.find(node);

                    //no begin poss found, get from children

                    if(bp == beginning_positions.end()){

                        std::vector<uint> positions;

                        SuffixTree::STInnerNode * inner = dynamic_cast<SuffixTree::STInnerNode *>(node);
                        auto it = inner->child_nodes.begin();
                        while (it != inner->child_nodes.end()){
                            auto child = *it;

                            SuffixTree::STNode * child_node = child.second;

                            if(SuffixTree::STInnerNode * inner_child = dynamic_cast<SuffixTree::STInnerNode *>(child_node)){
                                auto child_bp = beginning_positions.find(inner_child);
                                if(child_bp != beginning_positions.end()){

                                    positions.insert(positions.end(), (*child_bp).second.begin(), (*child_bp).second.end());

                                    beginning_positions.erase((*child_bp).first);
                                    //(*child_bp).second.clear();

                                }


                            }
                            if(SuffixTree::STLeaf * leaf_child = dynamic_cast<SuffixTree::STLeaf *>(child_node)){
                                positions.push_back(leaf_child->suffix);

                            }




                            it++;
                        }
                      //  DLOG(INFO)<<"begin pos size: "<< positions.size();
                        std::sort(positions.begin(), positions.end());

                        uint real_depth = positions.back() - positions.front();

                        if(real_depth<i){
                            rd_counter++;
                         //   DLOG(INFO)<<"reald depth of node: "<<real_depth;
                        }

                        beginning_positions[node]=positions;




                    }
                    std::vector<uint> & begin_pos = beginning_positions[node];
                   // DLOG(INFO)<<"found pos size: "<< begin_pos.size();

                    //check if repeating factor:
                    if(begin_pos.size() >= 2 && ( (begin_pos.back() ) - (begin_pos.front()) >= i)){

                        //check dead positions:
                        if(!(
                                dead_positions[(begin_pos.back())]              ||
                            //    dead_positions[(begin_pos.back()) + i -1]    ||
                                dead_positions[(begin_pos.front())]
                            //    dead_positions[(begin_pos.front()) + i -1]
                                )


                                ){


                            std::vector<uint> sel_pos = select_starting_positions(node, i);

                             // DLOG(INFO)<<"selected pos: "<<sel_pos.size();



                            if(! (sel_pos.size() >=2) ){
                                bin_it++;
                                continue;
                            }
                            //vector of text position, length
                            std::pair<uint,uint> rule = std::make_pair(sel_pos.at(0), i);
                            dictionary.push_back(rule);

                        //iterate over selected pos, add non terminal symbols
                            for(auto bp_it = sel_pos.begin(); bp_it != sel_pos.end(); bp_it++){
                                //(position in text, non_terminal_symbol_number, length_of_symbol);
                                non_term nts = std::make_tuple(*bp_it, nts_number, i);
                                nts_symbols.push_back(nts);
                                //mark as used
                                for(uint pos = 0; pos<i;pos++){
                                    dead_positions[pos+ *bp_it] = 1;
                                }
                            }
                            nts_number++;
                         }






                      //  DLOG(INFO)<<stree.get_text().substr(*begin_pos.begin(),i);
                        //vector of text position, length
                        //std::pair<uint,uint> rule = std::make_pair(begin_pos.begin(), pair.first);

                       // dictionary.push_back(rule);
                    }



                    bin_it++;

                }
            }

            StatPhase::log("real depth counter",rd_counter);




        });
        StatPhase::wrap("Sorting occurences", [&]{
        DLOG(INFO) << "sorting occurences";
        std::sort(nts_symbols.begin(), nts_symbols.end());
        });

    */
    }
};
}

}
