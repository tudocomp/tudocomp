#ifndef _INCLUDED_DS_SUFFIX_TREE_EDGE_HPP
#define _INCLUDED_DS_SUFFIX_TREE_EDGE_HPP

#include "SuffixTreeNode.hpp"


class STEdge {
private:
    uint start;
    uint end;
    STNode child;
public:
    STEdge(uint start, uint end, STNode child){
        this->start=start;
        this->end=end;
        this->child=child;
    }

    inline uint get_start(){
        return start;
    }

    inline uint get_end(){
        return end;
    }
    inline STNode get_child(){
        return child;
    }

    inline void set_start(uint start){
        this->start=start;
    }
    inline void set_end(uint end){
        this->end=end;
    }
    inline void set_child(STNode child){
        this->child = child;
    }
};

#endif // SUFFIXTREEEDGE

