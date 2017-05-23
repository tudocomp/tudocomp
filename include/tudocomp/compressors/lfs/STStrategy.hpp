#pragma once

#include <vector>
#include <tuple>

#include <unordered_map>


#include <tudocomp/util.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/SuffixTree.hpp>




namespace tdc {
namespace lfs {

class STStrategy : public Algorithm {
private:

    //(position in text, non_terminal_symbol_number, length_of_symbol);
    typedef std::tuple<uint,uint,uint> non_term;
    typedef std::vector<non_term> non_terminal_symbols;
    typedef std::vector<std::pair<uint,uint>> rules;


    SuffixTree stree;

    BitVector dead_positions;

    std::vector<std::vector<SuffixTree::STNode*> > bins;
    uint node_count;

    std::vector<uint> child_sizes;


   // typedef  std::vector<std::pair<uint, SuffixTree::STNode*> > string_depth_vector;


    inline virtual void compute_string_depth(SuffixTree::STNode* node, uint str_depth){




        SuffixTree::STInnerNode * inner = dynamic_cast<SuffixTree::STInnerNode *>(node);
        if(inner){
            node_count++;
            if(str_depth>0){

                bins[str_depth].push_back(node);
            }

            child_sizes.push_back(inner->child_nodes.size());

            auto it = inner->child_nodes.begin();
            while (it != inner->child_nodes.end()){
                auto child = *it;
                uint child_depth = (str_depth+stree.edge_length(child.second));
                compute_string_depth( child.second, child_depth);
                it++;
            }

        }
    }





    inline virtual std::vector<uint> select_starting_positions(std::vector<uint> & starting_positions, uint length){
        std::vector<uint> selected_starting_positions;
        //select occurences greedily non-overlapping:
        selected_starting_positions.reserve(starting_positions.size());

        long last =  0-length;
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

      /*


    inline virtual void update_tree(uint length, std::vector<uint> selected_positions){
        //foreach occpos \in gso do
        for(auto it = selected_positions.begin();it!= selected_positions.end();it++){
            uint occpos = *it;
            uint text_length = stree.get_size();
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
            uint min=stree.get_size();
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
    } */

public:

    using Algorithm::Algorithm; //import constructor

    inline static Meta meta() {
        Meta m("lfs_comp", "st");
        m.option("min_lrf").dynamic(2);
        return m;
    }


    inline void compute_rules(io::InputView & input, rules & dictionary, non_terminal_symbols & nts_symbols){
        uint min_lrf = env().option("min_lrf").as_integer();

        StatPhase::wrap("Constructing ST", [&]{
        stree = SuffixTree(input);
        });


        StatPhase::wrap("Computing String Depth", [&]{
            bins.resize(stree.get_size());
            node_count=0;
            compute_string_depth(stree.get_root(),0);
        });

        StatPhase::log("Number of inner Nodes", node_count);

        std::sort(child_sizes.begin(), child_sizes.end());
        if(child_sizes.size()>0){

            uint max_depth = child_sizes[child_sizes.size()-1];

            DLOG(INFO)<<"Max Child size: "<< max_depth;

            if(child_sizes.size()>=4){
                uint quarter = child_sizes.size() /4;

           // nts_depth[quarter -1];
                StatPhase::log("25 \% quantil Child size", child_sizes[quarter -1]);
                StatPhase::log("50 \% quantil Child size", child_sizes[(2*quarter) -1]);
                StatPhase::log("75 \% quantil Child size", child_sizes[(3*quarter) -1]);

            }
            if(child_sizes.size()>=10){
                StatPhase::log("90 \% Child size", child_sizes[(9 * (child_sizes.size()/10) ) -1]);
            }
            StatPhase::log("Max Child size", max_depth);
        }


        StatPhase::wrap("Computing LRF Occs", [&]{
            dead_positions = BitVector(stree.get_size(), 0);
            DLOG(INFO)<<"input size: "<<stree.get_size();
            std::unordered_map<SuffixTree::STNode *, std::vector<uint> > beginning_positions;

            beginning_positions.reserve(node_count);



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

                        beginning_positions[node]=positions;




                    }
                    std::vector<uint> & begin_pos = beginning_positions[node];
                   // DLOG(INFO)<<"found pos size: "<< begin_pos.size();

                    //check if repeating factor:
                    if(begin_pos.size() >= 2 && ( (begin_pos.back() ) - (begin_pos.front()) >= i)){

                        //check dead positions:
                        if(!(
                                dead_positions[(begin_pos.back())]              ||
                                dead_positions[(begin_pos.back()) + i -1]    ||
                                dead_positions[(begin_pos.front())]            ||
                                dead_positions[(begin_pos.front()) + i -1]
                                )


                                ){
                            DLOG(INFO)<<"is lrf!, length: " << i;


                            std::vector<uint> sel_pos = select_starting_positions(begin_pos, i);

                        }

                          //  DLOG(INFO)<<"selected pos: "<<sel_pos.size();




                      //  DLOG(INFO)<<stree.get_text().substr(*begin_pos.begin(),i);
                        //vector of text position, length
                        //std::pair<uint,uint> rule = std::make_pair(begin_pos.begin(), pair.first);

                       // dictionary.push_back(rule);
                    } else {
                        DLOG(INFO)<<"not a lrf!";
                    }


                    bin_it++;

                }
            }


            /*
        auto bin_it = bins.end();
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

            }*/



        });

        DLOG(INFO) << "sorting occurences";
        std::sort(nts_symbols.begin(), nts_symbols.end());
    }
};
}

}
