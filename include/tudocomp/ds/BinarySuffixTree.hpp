#pragma once

//Original Implementation::
// https://gist.github.com/makagonov/f7ed8ce729da72621b321f0ab547debb

//from: http://stackoverflow.com/questions/9452701/ukkonens-suffix-tree-algorithm-in-plain-english/9513423#9513423
#include <string>
#include <map>
#include <vector>
#include <tuple>

#include <unordered_map>


#include <tudocomp/io.hpp>


namespace tdc {

class BinarySuffixTree{
private:


    //binary property of tree:
    std::vector<uint> first_child;
    std::vector<uint> next_sibling;


    //information of nodes, stored in array

    std::vector<char> edge;

    //represents the edge leading to this node
    std::vector<uint> start;
    std::vector<uint> end;

    //suffix link of node
    std::vector<uint> suffix_link;

   // std::vector<uint> suffix;


    //text added to st
    std::string Text;
    int pos;

    uint suffix;

    //number of suffixes to be added;
    uint remainder;

    //active point, from where to start inserting new suffixes
    uint active_node;
    char active_edge;
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

        //init empty node
        start.push_back(s);
        end.push_back(e);
        first_child.push_back(0);
        next_sibling.push_back(0);
        edge.push_back(c);
        suffix_link.push_back(0);
      //  suffix.push_back(0);

        return start.size()-1;

    }


    void reserve(uint size){
        //init empty node
        start.reserve(size);
        end.reserve(size);
        first_child.reserve(size);
        next_sibling.reserve(size);
        edge.reserve(size);
        suffix_link.reserve(size);
     //   suffix.reserve(size);
    }



public:


    //computes edge length:
    uint edge_length(uint node){
        if(node==0){
            return 0;
        }

        if(end[node] == 0){
            return pos - start[node]+1;
        } else {
            return end[node]- start[node];
        }
    }

    //constructor
    BinarySuffixTree(){
        //no Text is read
        pos=-1;
        Text="";
        remainder=0;
        suffix=0;


        //init root
        start.push_back(0);
        end.push_back(0);
        first_child.push_back(0);
        next_sibling.push_back(0);
        edge.push_back(0);
        suffix_link.push_back(0);
        //suffix corresponds to start
 //       suffix.push_back(0);


        //active start node is root
        active_node=0;
        active_length=0;

        last_added_sl=0;

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
                do{
                    if(edge[child] == active_edge){
                        found = true;
                        break;
                    } else {
                        previous_sibling = child;
                        child=next_sibling[child];
                    }
                }
                while (next_sibling[child] != 0);
            }

            //if not found
            if(!found){
                //insert new leaf
                create_node(pos, 0, active_edge);
         //       leaves.push_back(new_leaf);
           //     new_leaf->suffix=suffix++;
                add_sl(active_node);

            } else {
                uint next =  child ;
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
                if(Text[start[next]+active_length] == c){
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
                first_child[split] = next;
                if(previous_sibling == next){
                    first_child[active_node]= split;
                } else {
                    next_sibling[previous_sibling] = split;
                }
                if(next_sibling[next] != 0){
                    next_sibling[split] = next_sibling[next];

                }
                next_sibling[next] = leaf;


                start[next]=start[next]+ active_length;
               // split->child_nodes[Text[next->start]] = next;
                add_sl(split);
       //         leaves.push_back(leaf);
            }
            remainder--;
            if(active_node==0 && active_length>0){
                active_length--;
                active_edge = Text[pos-remainder+1];
            }else {
                if(suffix_link[active_node] != 0){
                    active_node = suffix_link[active_node];
                } else {
                    active_node = 0;
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
     //   leaves.reserve(leaves.size()+input.size());
        for(uint i = 0; i<input.length();i++){
            add_char(input[i]);
        }
    }

    inline void append_input(Input& input){
        auto iview  = input.as_view();
        append_input(iview);
    }

    inline void append_input(io::InputView & input){
      //  leaves.reserve(leaves.size()+input.size());
        Text += input;
        for (uint i = 0; i < input.size(); i++) {
            uint8_t c = input[i];
            add_char(c);
        }
    }

    inline std::string get_text(){
        return Text;
    }

    inline uint get_size(){
        return Text.size();
    }


    inline uint get_root(){
        return 0;
    }

    inline std::string get_string_of_edge(uint node){
        return Text.substr(start[node], edge_length(node));
    }

   // inline std::vector<STNode*> get_leaves(){
   //     return leaves;
  //  }

   // inline STNode* get_leaf(uint position){
   //     return leaves.at(position);
   // }

    /*
    inline void print_tree(std::ostream& out, STNode* node, std::string depth){
        auto it = node->child_nodes.begin();
        while (it != node->child_nodes.end()){
            std::pair<char, STNode*> child = *it;
            out<< depth << "edge: " << get_string_of_edge(child.second) << std::endl;
            print_tree(out, child.second, depth+"  ");
            it++;
        }
    }*/

    BinarySuffixTree(Input& input) : BinarySuffixTree(){
        append_input(input);
    }

    BinarySuffixTree(io::InputView & input) : BinarySuffixTree(){
        append_input(input);
    }

    BinarySuffixTree(std::string input) :  BinarySuffixTree(){
        append_string(input);
    }


};

}

