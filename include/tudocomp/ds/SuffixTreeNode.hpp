#ifndef _INCLUDED_DS_SUFFIX_TREE_NODE_HPP
#define _INCLUDED_DS_SUFFIX_TREE_NODE_HPP

#include <unordered_map>
#include <map>

#include "SuffixTreeEdge.hpp"


class STNode{
private:
    std::map<char, STEdge> child_nodes;
    STNode suffix_link;
public:
    STNode(){
        suffix_link = this;
    }
    STNode(STNode suffix_link){
        this->suffix_link = suffix_link;
    }

    inline STEdge get_edge(char symbol){
        return child_nodes[symbol];
    }
    inline void add_child(char c, STEdge edge){

        //if(child_nodes)
    }

};

#endif // SUFFIXTREENODE

