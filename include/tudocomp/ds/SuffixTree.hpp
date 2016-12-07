#ifndef _INCLUDED_DS_SUFFIX_TREE_HPP
#define _INCLUDED_DS_SUFFIX_TREE_HPP

#include <string>

//#include <tudocomp/ds/SuffixTreeNode.hpp>
//#include <tudocomp/ds/SuffixTreeEdge.hpp>


#include "SuffixTreeEdge.hpp"
#include "SuffixTreeNode.hpp"


class SuffixTree{

private:
    STNode root;
    std::string Text;
    uint last_symbol;
public:
    SuffixTree(){
        root = new STNode();
    }


};


//represents Edges between Nodes
//start and end end are the corresponding start and end of the factor of this edge


#endif // _INCLUDED_DS_SUFFIX_TREE_HPP

