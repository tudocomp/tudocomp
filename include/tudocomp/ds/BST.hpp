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

template<typename size_type = uint>
class BST : STInterface<uint>{
private:

    typedef  uint node_type;


   // STInterface<size_type> stree;

    typedef IntVector<uint> vectortype ;

    //binary property of tree:  DynamicIntVector
    vectortype first_child;
    vectortype next_sibling;
    vectortype parent;


    //information of nodes, stored in array

   // std::vector<char> edge;

    //represents the edge leading to this node
    //DynamicIntVector start_dyn;
    vectortype start;
    vectortype end;

    //suffix link of node
    vectortype suffix_link;

    vectortype suffix;


    //text added to st
   // const io::InputView& Text;

    size_type new_node;


    node_type create_node(uint s, uint e){
        new_node++;
        start[new_node] = s;
        end[new_node] = e;


        return new_node;

    }


    /* uint previous_sibling = child;
            if(child != 0){
                do
                {
                    if(  Text[(start[child])] == active_edge){
                        found = true;
                        break;
                    } else {
                        previous_sibling = child;
                        child=next_sibling[child];
                    }
                }
                while (child != 0);

            }
            */

    void reserve(){



        auto size = Text.size() * 2 -1;
         DLOG(INFO)<<"allocating: "<<size;

        start= vectortype(size, 0);
        end =  vectortype(size, 0);
        first_child= vectortype(size, 0);
        next_sibling= vectortype(size, 0);
        suffix_link= vectortype(size, 0);
        suffix= vectortype(size, 0);
        parent = vectortype(size, 0);




    }

    void resize(){

        DLOG(INFO)<<"resizing: "<<new_node;
        new_node++;
        start.resize(new_node);
        end.resize(new_node);
        first_child.resize(new_node);
        next_sibling.resize(new_node);
        suffix_link.resize(new_node);
        suffix.resize(new_node);
        parent.resize(new_node);


        start.shrink_to_fit();
        end.shrink_to_fit();
        first_child.shrink_to_fit();
        next_sibling.shrink_to_fit();
        suffix_link.shrink_to_fit();
        suffix.shrink_to_fit();
        parent.shrink_to_fit();

    }

public:
    inline void add_child(node_type node, size_type start, size_type suffix_beg){
        node_type child = create_node(start,0);
        suffix[child]=suffix_beg;

        //add to childs of node:
        size_type temp = first_child[node];
        first_child[node]=child;
        next_sibling[child]=temp;

      //  return child;

    }

    //virtual auto set_start(node_type node, size_type start) -> void;
    //virtual auto set_end(node_type node, size_type end) -> void;
    //virtual auto set_suffix(node_type node, size_type suffix) -> void;

    inline node_type split_edge(node_type parent, node_type child, size_type edge_len){


        node_type split = create_node(start[child]  , start[child] + edge_len);
        start[child]= start[child] + edge_len;

        node_type child_prev = first_child[parent];


        first_child[split] = child;
        next_sibling[split] = next_sibling[child];
        next_sibling[child]=0;

            //if(child_prev == (uint) 0){
              //  first_child[parent]= split;
           // }

            if(child_prev == child){


                //size_type temp = first_child[parent];
                first_child[parent]= split;



                return split;

            }
            do
            {
                if(  next_sibling[child_prev] == child){
                    break;
                } else {
                    child_prev=next_sibling[child_prev];
                }
            }
            while (child_prev != 0);
            next_sibling[child_prev] = split;

            return split;

    }


    inline size_type get_edge_length(node_type node){
            if(node==0){
                return 0;
            }

            if(end[node] == (uint)0){
                return pos - start[node]+1;
            } else {
                return end[node]- start[node];
            }


    }

    inline size_type get_start(node_type node){ return start[node];}
    inline size_type get_end(node_type node){ return end[node];}



    inline size_type get_suffix(node_type node){
        return suffix[node];
    }


    inline bool is_leaf(node_type node){
        if(first_child[node]>0){
            return false;
        }
        else {
            return true;
        }
    }

    inline node_type get_child(node_type node, char c){
        size_type child = first_child[node];
        if(child != 0){
            do
            {
                if(  Text[(start[child])] == c){
                    return child;
                } else {
                    child=next_sibling[child];
                }
            }
            while (child != 0);

        }
        return node;


    }


    inline std::vector<node_type> get_child(node_type node){
        std::vector<node_type> children;
        size_type child = first_child[node];
        if(child != 0){
            do
            {
                children.push_back(child);
                child=next_sibling[child];

            }
            while (child != 0);

        }
        return children;
    }

    inline node_type get_suffix_link(node_type node){
        return suffix_link[node];
    }

    inline void set_suffix_link(node_type from_node, node_type to_node){
        suffix_link[from_node]= to_node;
    }


    inline node_type get_root(){
        return (uint) 0;
    }

    inline size_type get_tree_size(){
        return new_node;
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

    BST(io::InputView & input) : STInterface(input){

        reserve();


      //  stree(input);
        construct();

        resize();


    }


};

}

