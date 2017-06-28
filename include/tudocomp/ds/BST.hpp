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


    STInterface<size_type> stree;

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
    const io::InputView& Text;

    size_type new_node;


    uint create_node(uint s, uint e, char c){
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
        start= vectortype(size, 0);
        end =  vectortype(size, 0);
        first_child= vectortype(size, 0);
        next_sibling= vectortype(size, 0);
        suffix_link= vectortype(size, 0);
        suffix= vectortype(size, 0);
        parent = vectortype(size, 0);




    }

    void resize(){
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
    void add_child(node_type node, char c, size_type start, size_type suffix){
        node_type child = create_node(start,0,c);
        suffix[child]=suffix;

        //add to childs of node:
        size_type temp = first_child[node];
        first_child[node]=child;
        next_sibling[child]=temp;

      //  return child;

    }

    //virtual auto set_start(node_type node, size_type start) -> void;
    //virtual auto set_end(node_type node, size_type end) -> void;
    //virtual auto set_suffix(node_type node, size_type suffix) -> void;

    node_type split_edge(node_type node, size_type edge_len, char c){

        uint split = create_node(start[node] + edge_len, end[node], c);
        end[node]= start[node] + edge_len;
        size_type temp = first_child[node];
        first_child[node]= split;
        first_child[split]=temp;

        //update relations of nodes

        size_type temp = first_child[node];
        first_child[node]=split;
        first_child[child]=temp;


    }


    size_type get_edge_length(node_type node){
            if(node==0){
                return 0;
            }

            if(end[node] == (uint)0){
                return new_node - start[node]+1;
            } else {
                return end[node]- start[node];
            }


    }

  //  virtual auto get_edge_label(node_type node, size_type pos) -> char;

    size_type get_suffix(node_type node){
        return suffix[node];
    }


    bool is_leaf(node_type node){
        if(first_child[node]>0){
            return false;
        }
        else {
            return true;
        }
    }

    node_type get_child(node_type node, char c){
        size_type child = first_child[node];
        if(child != 0){
            do
            {
                if(  Text[(start[child])] == c){
                    break;
                } else {
                    child=next_sibling[child];
                }
            }
            while (child != 0);

        }
        if(child >0){
            return child;
        } else {
            return node;
        }

    }


    std::vector<node_type> get_child(node_type node){
        std::vector<node_type> child;
        size_type child = first_child[node];
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
    }

    node_type get_suffix_link(node_type node){
        return suffix_link[node];
    }

    void set_suffix_link(node_type from_node, node_type to_node){
        suffix_link[from_node]= to_node;
    }


    node_type get_root(){
        return 0;
    }

    size_type get_tree_size(){
        return new_node;
    }




    inline uint get_first_child(uint node){
        return first_child[node];
    }

    inline uint get_next_sibling(uint node){
        return next_sibling[node];
    }
    inline uint get_suffix(uint node){
        return suffix[node];
    }


    inline uint get_root(){
        return 0;
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

    inline uint get_tree_size(){
        return start.size();
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

    BST(Input& input) : Text(input.as_view()){
     //   auto in = input.as_view();

        stree(input.as_view());

   }

    BST(io::InputView & input) : Text(input){


        stree(input);


    }


};

}

