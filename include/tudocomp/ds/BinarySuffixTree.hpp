#pragma once

//Original Implementation::
// https://gist.github.com/makagonov/f7ed8ce729da72621b321f0ab547debb
//from: http://stackoverflow.com/questions/9452701/ukkonens-suffix-tree-algorithm-in-plain-english/9513423#9513423

#include <string>
#include <map>
#include <vector>
#include <tuple>
#include <memory>

#include <sstream>


#include <math.h>

#include <tudocomp/ds/IntVector.hpp>


#include <tudocomp/io.hpp>


namespace tdc {


class BinarySuffixTree{
private:


    typedef IntVector<uint> vectortype ;

    //binary property of tree:  DynamicIntVector
    vectortype first_child;
    vectortype next_sibling;


    //information of nodes, stored in array


    //represents the edge leading to this node
    vectortype start;
    vectortype end;

    //suffix link of node
    vectortype suffix_link;

    vectortype suffix;


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

        return new_node;

    }


    void reserve(){

        auto size = Text.size() * 2 -1;
        start= vectortype(size + 1, 0);
        end =  vectortype(size + 1, 0);
        first_child= vectortype(size, 0);
        next_sibling= vectortype(size, 0);
        suffix_link= vectortype(size, 0);
        suffix= vectortype(size + 1, 0);




    }

    void resize(){
        new_node++;
        start.resize(new_node);
        end.resize(new_node);
        first_child.resize(new_node);
        next_sibling.resize(new_node);

        suffix.resize(new_node);


        start.shrink_to_fit();
        end.shrink_to_fit();
        first_child.shrink_to_fit();
        next_sibling.shrink_to_fit();
        suffix.shrink_to_fit();

        //deallocate sl, because not necessary, save 20% space
        suffix_link=vectortype(0, 0);

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


        //active start node is root
        active_node=0;
        active_length=0;

        last_added_sl=0;
        DLOG(INFO)<<"Text size: " << Text.size();

        for (uint i = 0; i < Text.size(); i++) {
            uint8_t c = Text[i];
            add_char(c);
        }

        resize();
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


                start[next]=start[next]+ active_length;


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

                uint leaf = create_node(pos, 0, c);
                next_sibling[next] = leaf;
                suffix[leaf] = current_suffix++;

                add_sl(split);
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

    inline uint get_edge_length(uint node){
        return edge_length(node);
    }

    inline std::string get_string_of_edge(uint node){
        std::stringstream ss ;
        ss << start[node]<< " " <<start[node] + edge_length(node);
        return ss.str();
    }

    inline uint get_tree_size(){
        return start.size();
    }



    inline void print_tree(uint node, std::string depth){

        uint child = first_child[node];
        while (child != 0){
            DLOG(INFO)<< depth << "edge: " << get_string_of_edge(child) << std::endl;
            print_tree(child, depth+"  ");
            child=next_sibling[child];
        }
    }

    BinarySuffixTree(io::InputView & input) : Text(input){
        compute();
    }

};

}

