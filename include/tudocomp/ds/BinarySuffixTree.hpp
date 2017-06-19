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


#include <tudocomp/io.hpp>


namespace tdc {

class BinarySuffixTree{
private:


    //binary property of tree:
    DynamicIntVector first_child;
    DynamicIntVector next_sibling;


    //information of nodes, stored in array

   // std::vector<char> edge;

    //represents the edge leading to this node
    //DynamicIntVector start_dyn;
    DynamicIntVector start;
    DynamicIntVector end;

    //suffix link of node
    DynamicIntVector suffix_link;

    DynamicIntVector suffix;


    //text added to st
    const io::InputView& Text;
    int pos;

    uint current_suffix;
    uint new_node;

    //number of suffixes to be added;
    uint remainder;

    //active point, from where to start inserting new suffixes
    uint active_node;
    uint8_t active_edge;
    uint active_length;


    //saves last added node
    uint last_added_sl;

    void add_sl(uint node){

        if(last_added_sl != 0) {
            suffix_link[last_added_sl]=node;
        }
        last_added_sl=node;
    }

    uint create_node(uint s, uint e, char c){
        new_node++;
        start[new_node] = s;
        end[new_node] = e;

        //init empty node
       // start.push_back(s);
       // end.push_back(e);
       // first_child.push_back(0);
       // next_sibling.push_back(0);
       // edge.push_back(c);
       // suffix_link.push_back(0);
      //  suffix.push_back(0);

        return new_node;

    }


    void reserve(){

        auto bits_req_text = bits_for(Text.size() );
        auto size = Text.size() * 2 -1;
        auto bits_req_arr = bits_for(size);
        DLOG(INFO)<<" bits req: " << bits_req_text;
        DLOG(INFO)<<" text size *2 -1: " << size << "bits: " << bits_req_arr;

        start= DynamicIntVector(size, 0, bits_req_text);
        end =  DynamicIntVector(size, 0, bits_req_text);
        //init empty node
     //   start.reserve(size);
     //   end.reserve(size);
        first_child= DynamicIntVector(size, 0, bits_req_arr);
        next_sibling= DynamicIntVector(size, 0, bits_req_arr);
      //  edge.reserve(size);
        suffix_link= DynamicIntVector(size, 0, bits_req_arr);
        suffix= DynamicIntVector(size, 0, bits_req_text);
    }

    void compute(){
        new_node=0;





        reserve();

        //no Text is read
        pos=-1;
        remainder=0;
        current_suffix=0;


        //init root
        create_node(0,0,0);
        //suffix corresponds to start
 //       suffix.push_back(0);


        //active start node is root
        active_node=0;
        active_length=0;

        last_added_sl=0;
        DLOG(INFO)<<"Text size: " << Text.size();

        for (uint i = 0; i < Text.size(); i++) {
            uint8_t c = Text[i];
            add_char(c);
        }
    }



public:


    //computes edge length:
    uint edge_length(uint node){
        if(node==0){
            return 0;
        }

        if(end[node] == (uint)0){
            return pos - start[node]+1;
        } else {
            return end[node]- start[node];
        }
    }


    ~BinarySuffixTree(){

    }

private:
    inline void add_char(char c){
        //Text += c;
        pos++;
        remainder++;
        last_added_sl=0;


        while(remainder > 0){
            if(active_length==0){
                active_edge = c;
            }



            //if the active node doesnt have the corresponding edge:
            //find edge corresponding to added active edge
            bool found = false;
            uint child  = first_child[active_node];
            uint previous_sibling = child;
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

            //if not found
            if(!found){
                //insert new leaf
                uint leaf = create_node(pos, 0, active_edge);
                suffix[leaf] = current_suffix++;
                if(first_child[active_node]== (uint)0){
                    first_child[active_node] = leaf;
                } else {
                    next_sibling[previous_sibling] = leaf;
                }
                add_sl(active_node);

            } else {
                uint next =  child ;
                //if the active length is greater than the edge length:
                //switch active node to that
                //walk down
                if(active_length>= edge_length(next)){
                    active_node = next;
                    active_length -= edge_length(next);
                    active_edge = (char) Text[pos-active_length];
                    continue;
                }

                //if that suffix is already in the tree::
                if( (char) Text[start[next]+active_length] == c){
                    active_length++;
                    add_sl(active_node);
                    break;
                }

                //now split edge if the edge is found

                uint split = create_node(start[next], start[next]+active_length, active_edge);
                //active_inner->child_nodes[active_edge] = split;

                uint leaf = create_node(pos, 0, c);
               // leaf->suffix=suffix++;

                //update relations of nodes
                if(first_child[active_node] == (uint) 0){
                    first_child[active_node]= split;
                }
                if(first_child[active_node] == next){
                    first_child[active_node]= split;
                } else {
                    next_sibling[previous_sibling] = split;
                }
                first_child[split] = next;
                next_sibling[split] = next_sibling[next];
                next_sibling[next] = leaf;
                suffix[leaf] = current_suffix++;


                start[next]=start[next]+ active_length;
              //  edge[next]=Text[start[next]];
               // split->child_nodes[Text[next->start]] = next;
                add_sl(split);
       //         leaves.push_back(leaf);
            }
            remainder--;
            if(active_node==0 && active_length>0){
                active_length--;
                active_edge = Text[pos-remainder+1];
            }else {
                if(suffix_link[active_node] != (uint)0){
                    active_node = suffix_link[active_node];
                } else {
                    active_node = 0;
                }

            }
        }

    }
public:
  //  inline void append_char(char c){

   //     Text +=c;
  //      add_char(c);
  //  }

   // inline void append_string(std::string input){
   //     Text += input;
    //    reserve(input.size() * 3);
     //   leaves.reserve(leaves.size()+input.size());
    //    for(uint i = 0; i<input.length();i++){
   //         add_char(input[i]);
   //     }
  //  }

   // inline void append_input(Input& input){
    //    auto iview  = input.as_view();
  //      append_input(iview);
  //  }

  //  inline void append_input(io::InputView & input){
      //  leaves.reserve(leaves.size()+input.size());

    //}

    //inline std::string get_text(){
    //    return Text;
   // }

    inline uint get_size(){
        return Text.size();
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
        ss << start[node]<< " " <<start[node] + edge_length(node);
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

    BinarySuffixTree(Input& input) : Text(input.as_view()){
     //   auto in = input.as_view();
        compute();

   }

    BinarySuffixTree(io::InputView & input) : Text(input){

        compute();


    }


};

}

