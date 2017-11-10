#pragma once

//Original Implementation::
// https://gist.github.com/makagonov/f7ed8ce729da72621b321f0ab547debb

//from: http://stackoverflow.com/questions/9452701/ukkonens-suffix-tree-algorithm-in-plain-english/9513423#9513423
#include <string>
#include <map>
#include <vector>
#include <tuple>

#include <sstream>


#include <math.h>

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/STInterface.hpp>


#include <tudocomp/io.hpp>




namespace tdc {


    struct STNode;
    struct STLeaf;
    struct STInnerNode;


template<typename size_type = uint>
class NaivST : STInterface<STNode>{
private:

    typedef  STNode node_type;


    struct STNode{

        //represents the edge leading to this node
        int start;
        int end;
        STNode(int s, int e = 0) :  start(s), end (e) {}
        virtual ~STNode(){}

    };

    struct STLeaf : STNode {
        //suffix if leaf
        uint suffix;
        //constructor. e=0
        STLeaf(int s, int e = 0) : STNode(s,e){}

        virtual ~STLeaf() {}

    };

    struct STInnerNode : STNode {
        //
        uint string_depth;
        STInnerNode * parent;

        // child nodes
        std::unordered_map<char, STNode*> child_nodes;
        //suffix link
        STInnerNode* suffix_link;
        //constructor. e=0

        // start(s), end (e)
        STInnerNode(int s, int e = 0) : STNode(s,e){ suffix_link = NULL; child_nodes.reserve(6);}

        virtual ~STInnerNode(){}

    };





public:
    inline void add_child(node_type node, size_type start, size_type suffix_beg){

        STLeaf child = STLeaf(start,0);
        child.suffix = suffix_beg;




    }


    inline node_type split_edge(node_type parent, node_type child, size_type edge_len){




    }


    inline size_type get_edge_length(node_type node){



    }

    inline size_type get_start(node_type node){ return start[node];}
    inline size_type get_end(node_type node){ return end[node];}



    inline size_type get_suffix(node_type node){

    }


    inline bool is_leaf(node_type node){

    }

    inline node_type get_child(node_type node, char c){



    }


    inline std::vector<node_type> get_child(node_type node){

    }

    inline node_type get_suffix_link(node_type node){

    }

    inline void set_suffix_link(node_type from_node, node_type to_node){

    }


    inline node_type get_root(){

    }

    inline size_type get_tree_size(){

    }





    inline std::string get_string_of_edge(uint node){
       // return Text.substr(start[node], edge_length(node));
        std::stringstream ss ;
        ss << start[node]<< " " <<start[node] + get_edge_length(node);
       // for (uint i = start[node]; i<start[node] + edge_length(node); i++){
        //    DLOG(INFO)<<"works "<< i << " " <<  edge_length(node) ;
          //  ss << Text[i];
        //    DLOG(INFO)<<"works";
      //  }
        return ss.str();
    }


   // inline std::vector<STNode*> get_leaves(){
   //     return leaves;
  //  }

   // inline STNode* get_leaf(uint position){
   //     return leaves.at(position);
   // }


    inline void print_tree(uint node, std::string depth){

        uint child = first_child[node];
        while (child != 0){
            DLOG(INFO)<< depth << "edge: " << get_string_of_edge(child) << std::endl;
            print_tree(child, depth+"  ");
            child=next_sibling[child];
        }
    }

   // BST(Input& input) : STInterface(input){
     //   auto in = input.as_view();

     //   stree(input.as_view());
    //    construct();

  // }

    NaivST(io::InputView & input) : STInterface(input){



      //  stree(input);
        construct();



    }


};

}

