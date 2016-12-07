#ifndef _INCLUDED_DS_SUFFIX_TREE_HPP
#define _INCLUDED_DS_SUFFIX_TREE_HPP

#include <string>
#include <map>

//#include <tudocomp/ds/SuffixTreeNode.hpp>
//#include <tudocomp/ds/SuffixTreeEdge.hpp>


//#include "SuffixTreeEdge.hpp"
//#include "SuffixTreeNode.hpp"

namespace tdc {
using namespace tdc;
class SuffixTree{
public:
struct STNode;
struct STEdge;


private:
    STNode* root;

    std::string Text;

    inline STNode* add_child(STNode* parent, uint from_suffix, uint to_suffix, char symbol){
        auto it = parent->child_nodes.find(symbol);
        if(it == parent->child_nodes.end()){
            STNode* child = new STNode();
            STEdge* edge_to_child = new STEdge();
            edge_to_child->start=from_suffix;
            edge_to_child->end = to_suffix;
            edge_to_child->child= child;
            parent->child_nodes[symbol]=edge_to_child;
            return child;
        }else {

            return parent;
        }

    }





public:
    struct STEdge {
        uint start;
        uint end;
        STNode* child;
    };

    struct STNode{
        std::map<char, STEdge*> child_nodes;
        struct STNode* suffix_link;
        uint suffix;
    };

    SuffixTree(){
        root = new STNode();
    }

    inline void add_char(char c){
        Text += c;
        add_child(root,0,Text.length(),c);

    }
    inline void add_string(std::string input){
        for(int i = 0; i<input.length();i++){
            add_char(input[i]);
        }
    }

    inline std::string get_text(){
        return Text;
    }


    inline SuffixTree::STNode* get_root(){
        return root;
    }



};

}


//represents Edges between Nodes
//start and end end are the corresponding start and end of the factor of this edge


#endif // _INCLUDED_DS_SUFFIX_TREE_HPP

