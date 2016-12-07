#ifndef _INCLUDED_DS_SUFFIX_TREE_HPP
#define _INCLUDED_DS_SUFFIX_TREE_HPP

#include <string>
#include <map>

//#include <tudocomp/ds/SuffixTreeNode.hpp>
//#include <tudocomp/ds/SuffixTreeEdge.hpp>


//#include "SuffixTreeEdge.hpp"
#include "SuffixTreeNode.hpp"

namespace tdc {
using namespace tdc;
class SuffixTree{

private:
    STNode* root;
    STNode* last_node;
    std::string Text;
    uint last_symbol;
public:
    SuffixTree(){
        root = new STNode();
    }

    inline void add_char(char c){
        Text += c;

    }
    inline void add_string(std::string input){
        for(int i = 0; i<input.length();i++){
            add_char(input[i]);
        }
    }

    inline std::string get_text(){
        return Text;
    }


};

}


//represents Edges between Nodes
//start and end end are the corresponding start and end of the factor of this edge


#endif // _INCLUDED_DS_SUFFIX_TREE_HPP

