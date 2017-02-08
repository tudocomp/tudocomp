#ifndef _INCLUDED_ST_LFS_COMPRESSOR_HPP_
#define _INCLUDED_ST_LFS_COMPRESSOR_HPP_

#include <tudocomp/ds/SuffixTree.hpp>



#include <tudocomp/Compressor.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/util.hpp>

#include <tudocomp/io.hpp>

//#include <iostream>

#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/io/BitIStream.hpp>
#include <tudocomp/io/BitOStream.hpp>


#include <tudocomp/ds/IntVector.hpp>

#include <utility>

#include <set>
#include <string>
#include <vector>
#include <tuple>

//#include <tudocomp/tudocomp.hpp>


namespace tdc {

template<typename literal_coder_t, typename len_coder_t>
class STLFSCompressor : public Compressor {
private:
    SuffixTree stree;
    uint min_lrf;

    BitVector dead_positions;

    typedef  std::vector<std::pair<uint, SuffixTree::STNode*> > string_depth_vector;

    //
    //(position in text, non_terminal_symbol_number, length_of_symbol);
    typedef std::tuple<uint,uint,uint> non_term;
    typedef std::vector<non_term> non_terminal_symbols;
    //


    inline virtual std::vector<uint> select_starting_positions(std::set<uint> starting_positions, uint length){
        std::vector<uint> selected_starting_positions;
        //select occurences greedily non-overlapping:
        selected_starting_positions.reserve(starting_positions.size());

        int last =  0-length;
        uint current;
        for (auto it=starting_positions.begin(); it!=starting_positions.end(); ++it){

            current = *it;
            //DLOG(INFO) << "checking starting position: " << current << " length: " << top.first << "last " << last;
            if(last+length <= current){
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
            uint pos = std::max((uint)1, occpos-length);
            uint end = std::min(text_length, occpos + length);//maybe -1
                DLOG(INFO)<<"occpos: "<<occpos;

            //line 2!
            //uint pos = pos
            for(; pos<end;pos++){
                //3: find first node such that v.pathlen > length
                DLOG(INFO)<<"fixiung suffix: "<<pos;

                uint walked_length = 0;
                //current node = v
                SuffixTree::STNode * current_node = stree.get_root();
                while (walked_length <= length){
                    char c = stree.get_text()[pos+walked_length];
                   // DLOG(INFO) << c;
                    current_node = current_node->child_nodes[c];
                    walked_length += stree.edge_length(current_node);
                   // DLOG(INFO) << "walked length: "<< walked_length;

                }
                //4: delete leaf pos[i], maintain  triple
                if(current_node->min_bp == pos){
                    current_node->min_bp = current_node->max_bp;
                    current_node->card_bp = current_node->card_bp-1;
                }
                if(current_node->max_bp == pos){
                    current_node->max_bp = current_node->min_bp;
                    current_node->card_bp = current_node->card_bp-1;
                }

                //5:
                if(pos < occpos  && !dead_positions[pos]){
                    DLOG(INFO)<<"find/create node s, recreate leaf (TODO)";
                    //6-8 here
                }
                //9:
                if(pos>occpos) {
                    //w[i] = dot?
                    // mark dead pos
                    dead_positions[pos] = 1;
                    DLOG(INFO)<<"positions marked dead: "<<pos;
                }
            }
            //10: w[occpos] = non_term

            dead_positions[occpos] = 1;

            DLOG(INFO)<<"positions marked dead: "<<occpos;
        }

    }

    //returns all bp of corresponding factor
    inline virtual std::set<uint> compute_triple(SuffixTree::STNode* node){

        std::set<uint> beggining_positions;
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
                if(child.second->card_bp>0){

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

        }
        return beggining_positions;
    }



public:
    inline static Meta meta() {
        Meta m("compressor", "suffixtree_longest_first_substitution_compressor",
            "This is an implementation of the longest first substitution compression scheme using suffixtree.");
        m.option("lit_coder").templated<literal_coder_t>();
        m.option("len_coder").templated<len_coder_t>();
        m.needs_sentinel_terminator();
        return m;
    }


    inline STLFSCompressor(Env&& env):
        Compressor(std::move(env))
    {
        stree=SuffixTree();
        min_lrf=2;
        DLOG(INFO) << "Compressor instantiated";

    }
    inline virtual void compress(Input& input, Output& output) override {

        //BitVector
        dead_positions = BitVector(input.size(), 0);
        DLOG(INFO)<< "dead_positions.size(): "<<dead_positions.size();

        //build suffixtree
        DLOG(INFO)<<"build suffixtree";


        //auto in = input.as_view();


        //vector of text position, length
        std::vector<std::pair<uint,uint>> rules;



        //
        //std::tuple<uint,uint,uint> symbol(*it, non_terminal_symbol_number, length_of_symbol);
        //

        std::vector<std::tuple<uint,uint,uint>> non_terminal_symbols;



        stree.append_input(input);


        std::string text = stree.get_text();

        DLOG(INFO)<< text << std::endl;
        //compute string depth of st:
        string_depth_vector nl;
        compute_string_depth(stree.get_root(),0, &nl);

        std::sort(nl.begin(), nl.end());

        auto it = nl.end();
        while (it != nl.begin()){
            it--;
            auto pair = *it;
            if(pair.first<min_lrf){
                break;
            }
            std::set<uint> begining_pos = compute_triple(pair.second);
            if(pair.second->card_bp>=2){
                //compute if overlapping:
                if(pair.second->min_bp+pair.first <= pair.second->max_bp){

                    //its a reapting factor, compute
                    DLOG(INFO)<<"reapting factor:  \"" << text.substr( pair.second->min_bp, pair.first)<<"\"" ;

                    DLOG(INFO)<<"length: "<<pair.first;
                    //min and mac of all children are all BPs of LRF
                    auto it = begining_pos.begin();
                    DLOG(INFO) << "beginning positions: " << std::endl;
                    while(it!= begining_pos.end()){
                        DLOG(INFO) << *it;
                        it++;
                    }

                    std::vector<uint> selected_pos = select_starting_positions(begining_pos, pair.first);
                   // DLOG(INFO) << "selected beginning positions: " << std::endl;

                    update_tree(pair.first, selected_pos);

                    //


                }

            }


        }


    }

    inline virtual void decompress(Input& input, Output& output) override {
        return;
        DLOG(INFO) << "decompress lfs";
        std::shared_ptr<BitIStream> bitin = std::make_shared<BitIStream>(input);

        typename literal_coder_t::Decoder lit_decoder(
            env().env_for_option("lit_coder"),
            bitin
        );
        typename len_coder_t::Decoder len_decoder(
            env().env_for_option("len_coder"),
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

        std::vector<std::string> dictionary;
        uint dictionary_size = dict_lengths.size();

        Range dictionary_r (0, dictionary_size);


        uint length_of_symbol;
        std::string non_terminal_symbol;
        DLOG(INFO) << "reading dictionary";
        for(uint i = 0; i< dict_lengths.size();i++){
            non_terminal_symbol ="";
            char c1;
            length_of_symbol=dict_lengths[i];
            for(uint i =0; i< length_of_symbol;i++){
                c1 = lit_decoder.template decode<char>(literal_r);
                non_terminal_symbol += c1;
            }
            dictionary.push_back(non_terminal_symbol);
        }
        auto ostream = output.as_stream();

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

                if(symbol_number < dictionary.size()){

                    ostream << dictionary.at(symbol_number);
                } else {
                    DLOG(INFO)<< "too large symbol: " << symbol_number;
                }

            }
        }
    }

};

}


#endif
