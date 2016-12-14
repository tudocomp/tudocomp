#ifndef _INCLUDED_DS_SUFFIX_TREE_HPP
#define _INCLUDED_DS_SUFFIX_TREE_HPP

//Original Implementation::
// https://gist.github.com/makagonov/f7ed8ce729da72621b321f0ab547debb

//from: http://stackoverflow.com/questions/9452701/ukkonens-suffix-tree-algorithm-in-plain-english/9513423#9513423
#include <string>
#include <map>
#include <vector>
#include <tuple>


#include <tudocomp/io.hpp>
namespace tdc {
using namespace tdc;

class SuffixTree{
public:
    struct STNode;
private:

    std::vector<STNode*> leaves;

    //root node
    STNode* root;

    //text added to st
    std::string Text;
    int pos;

    //number of suffixes to be added;
    uint remainder;

    //active point, from where to start inserting new suffixes
    STNode* active_node;
    char active_edge;
    uint active_length;


    //saves last added node
    STNode* last_added_sl;

    void add_sl(STNode* node){

        if(last_added_sl != root) {
            last_added_sl->suffix_link=node;
        }
        last_added_sl=node;
    }

public:


    //computes edge length:
    uint edge_length(STNode* node){
        if(node->start==-1){
            return 0;
        }

        if(node->end == 0){
            return pos - node->start+1;
        } else {
            return node->end - node->start;
        }
    }
    struct STNode{

        //represents the edge leading to this node
        int start;
        int end;
        // child nodes
        std::map<char, STNode*> child_nodes;
        //suffix link
        STNode* suffix_link;

        //constructor. e=0
        STNode(int s, int e = 0) : start(s), end (e){ suffix_link=NULL; card_bp=0;}

        // needed for lfs, corresponds to triple
        uint min_bp;
        uint max_bp;
        uint card_bp;


    };
    //constructor
    SuffixTree(){
        //no Text is read
        pos=-1;
        Text="";
        remainder=0;


        root = new STNode(-1);

        //active start node is root
        active_node=root;
        active_length=0;

        last_added_sl=root;

    }
    ~SuffixTree(){
        root=NULL;

        active_node=root;
        last_added_sl=root;
    }

private:
    inline void add_char(char c){
        //Text += c;
        pos++;
        remainder++;
        last_added_sl=root;

        while(remainder > 0){
            if(active_length==0){
                active_edge = c;
            }



            //if the active node doesnt have the corresponding edge:
            auto next_it = active_node->child_nodes.find(active_edge);
            if(next_it==active_node->child_nodes.end()){
                //insert new leaf
                STNode* new_leaf = new STNode(pos);
                active_node->child_nodes[active_edge] = new_leaf;
                leaves.push_back(new_leaf);
                add_sl(active_node);

            } else {
                STNode* next = active_node->child_nodes[active_edge];
                //if the active length is greater than the edge length:
                //switch active node to that
                //walk down
                if(active_length>= edge_length(next)){
                    active_node = next;
                    active_length -= edge_length(next);
                    active_edge = Text[pos-active_length];
                    continue;
                }

                //if that suffix is already in the tree::
                if(Text[next->start +active_length] == c){
                    active_length++;
                    add_sl(active_node);
                    break;
                }

                //now split edge if the edge is found

                STNode* split = new STNode(next->start, next->start+active_length);
                active_node->child_nodes[active_edge] = split;
                STNode* leaf = new STNode(pos);
                split->child_nodes[c] = leaf;

                next->start=next->start + active_length;
                split->child_nodes[Text[next->start]] = next;
                add_sl(split);
                leaves.push_back(leaf);
            }
            remainder--;
            if(active_node==root && active_length>0){
                active_length--;
                active_edge = Text[pos-remainder+1];
            }else {
                if(active_node->suffix_link != NULL){
                    active_node = active_node->suffix_link;
                } else {
                    active_node = root;
                }

            }
        }

    }
public:
    inline void append_char(char c){

        Text +=c;
        add_char(c);
    }

    inline void append_string(std::string input){
        Text += input;
        leaves.reserve(leaves.size()+input.size());
        for(uint i = 0; i<input.length();i++){
            add_char(input[i]);
        }
    }

    inline void append_input(Input& input){
        auto iview  = input.as_view();
        leaves.reserve(leaves.size()+iview.size());
        Text += iview;
        for (uint i = 0; i < iview.size(); i++) {
            uint8_t c = iview[i];
            add_char(c);
        }
    }

    inline std::string get_text(){
        return Text;
    }


    inline SuffixTree::STNode* get_root(){
        return root;
    }

    inline std::string get_string_of_edge(STNode* node){
        return Text.substr(node->start, edge_length(node));
    }

    inline std::vector<STNode*> get_leaves(){
        return leaves;
    }

    inline void print_tree(std::ostream& out, STNode* node, std::string depth){
        auto it = node->child_nodes.begin();
        while (it != node->child_nodes.end()){
            std::pair<char, STNode*> child = *it;
            out<< depth << "edge: " << get_string_of_edge(child.second) << std::endl;
            print_tree(out, child.second, depth+"  ");
            it++;
        }
    }

    SuffixTree(Input& input) : SuffixTree(){
        append_input(input);
    }

    SuffixTree(std::string input) :  SuffixTree(){
        append_string(input);
    }


};

}


#endif // _INCLUDED_DS_SUFFIX_TREE_HPP

