#pragma once

#include <vector>
#include <tuple>

#include <tudocomp/util.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/SuffixTree.hpp>




namespace tdc {
namespace lfs {

template<uint min_lrf = 2 >
class STStrategy : public Algorithm {
private:

    //(position in text, non_terminal_symbol_number, length_of_symbol);
    typedef std::tuple<uint,uint,uint> non_term;
    typedef std::vector<non_term> non_terminal_symbols;
    typedef std::vector<std::pair<uint,uint>> rules;


    SuffixTree stree;

    BitVector dead_positions;

    typedef  std::vector<std::pair<uint, SuffixTree::STNode*> > string_depth_vector;


    inline virtual std::vector<uint> select_starting_positions(std::set<uint> starting_positions, uint length){
        std::vector<uint> selected_starting_positions;
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

    inline virtual void compute_string_depth(SuffixTree::STNode* node, uint str_depth, string_depth_vector* node_list){

        if(str_depth>0){

            node_list->push_back(std::make_pair(str_depth, node));
        }

        auto it = node->child_nodes.begin();
        while (it != node->child_nodes.end()){
            auto child = *it;
            uint child_depth = (str_depth+stree.edge_length(child.second));
            compute_string_depth( child.second, child_depth, node_list);
            //string_depth_vector child_list =
            //node_list.insert(node_list->end(), child_list.begin(), child_list.end());
            it++;
        }
    }

    inline virtual void update_tree(uint length, std::vector<uint> selected_positions){
        //foreach occpos \in gso do
        for(auto it = selected_positions.begin();it!= selected_positions.end();it++){
            uint occpos = *it;
            uint text_length = stree.get_text().length();
            uint pos = occpos;
            uint end = std::min(text_length, occpos + length);
            for(; pos<end;pos++){
                if(pos>occpos) {
                    dead_positions[pos] = 1;
                }
            }

            dead_positions[occpos] = 1;
        }

    }

    //returns all bp of corresponding factor
    inline virtual std::set<uint> compute_triple(SuffixTree::STNode* node){

        std::set<uint> beggining_positions;
        //already computed
        if(node->deleted){
            return beggining_positions;

        }
        //if no childs, its a leaf
        if(node->child_nodes.size()==0){
            node->min_bp=node->suffix;
            node->max_bp=node->suffix;
            node->card_bp=1;

        } else {
            uint min=stree.get_text().size();
            uint max=0;
            uint card = 0;
            //add all min begins and maxi begins of children to begins
            auto it = node->child_nodes.begin();
            uint min_child;
            uint max_child;
            while (it != node->child_nodes.end()){
                auto child = *it;
                min_child = child.second->min_bp;
                max_child = child.second->max_bp;
                if(child.second->card_bp>0 && !(child.second->deleted)){

                    beggining_positions.insert(min_child);

                    beggining_positions.insert(max_child);

                    if(min > min_child){
                        min = min_child;
                    }
                    if(max < max_child){
                        max = max_child;
                    }
                    card +=child.second->card_bp;

                }


                it++;
            }
            node->card_bp=card;
            node->min_bp = min;
            node->max_bp= max;
            node->computed = true;

        }
        return beggining_positions;
    }

public:

    using Algorithm::Algorithm; //import constructor

    inline static Meta meta() {
        Meta m("lfs_comp", "st");
        return m;
    }


    inline void compute_rules(const io::InputView & input, rules & dictionary, non_terminal_symbols & nts_symbols){
        //BitVector
        dead_positions = BitVector(input.size(), 0);
        //DLOG(INFO)<< "dead_positions.size(): "<<dead_positions.size();

        //build suffixtree
        DLOG(INFO)<<"build suffixtree";

        stree = SuffixTree(input);


       // stree.append_input(in);

        DLOG(INFO)<<"computing string depth";
        //min_lrf=2;

        //std::string t = stree.get_text();

        //DLOG(INFO)<< t << std::endl;
        //compute string depth of st:
        string_depth_vector nl;
        compute_string_depth(stree.get_root(),0, &nl);
        DLOG(INFO)<<"sorting nodes";

        DLOG(INFO)<<"number of nodes: "<<nl.size();

        std::sort(nl.begin(), nl.end());
        uint nts_number =0;

        DLOG(INFO)<<"done. computing lrfs";
        auto it = nl.end();
        while (it != nl.begin()){
            it--;
            auto pair = *it;
            if(pair.first<min_lrf){
                break;
            }
            std::set<uint> begining_pos = compute_triple(pair.second);
           // DLOG(INFO)<<"computing: \"" << t.substr( pair.second->min_bp, pair.first)<<"\"";
            if(pair.second->card_bp>=2){
                //compute if overlapping:
                if(pair.second->min_bp+pair.first <= pair.second->max_bp){

                    //its a reapting factor, compute
                   // DLOG(INFO)<<"reapting factor:  \"" << t.substr( pair.second->min_bp, pair.first)<<"\"" ;

                   // DLOG(INFO)<<"length: "<<pair.first;
                    //min and mac of all children are all BPs of LRF
                   // auto it = begining_pos.begin();
                   /* DLOG(INFO) << "beginning positions: " << std::endl;
                    while(it!= begining_pos.end()){
                        DLOG(INFO) << *it;
                        it++;
                    }*/

                    std::vector<uint> selected_pos = select_starting_positions(begining_pos, pair.first);
                   // DLOG(INFO) << "selected beginning positions: " << std::endl;
                    if(selected_pos.size()>=2){

                        update_tree(pair.first, selected_pos);

                        //vector of text position, length
                        std::pair<uint,uint> rule = std::make_pair(selected_pos.at(0), pair.first);

                        dictionary.push_back(rule);

                        //iterate over selected pos, add non terminal symbols
                        for(auto it = selected_pos.begin(); it != selected_pos.end(); it++){
                            //(position in text, non_terminal_symbol_number, length_of_symbol);
                            //typedef std::tuple<uint,uint,uint> non_term;
                            non_term nts = std::make_tuple(*it, nts_number, pair.first);
                            nts_symbols.push_back(nts);
                            //typedef std::vector<non_term> non_terminal_symbols;
                        }
                        nts_number++;

                    }


                    //


                }

            }


        }

        DLOG(INFO) << "sorting occurences";
        std::sort(nts_symbols.begin(), nts_symbols.end());
    }
};
}

}
