#ifndef _INCLUDED_DS_SUFFIX_TREE_HPP
#define _INCLUDED_DS_SUFFIX_TREE_HPP

//Original Implementation::
// https://gist.github.com/makagonov/f7ed8ce729da72621b321f0ab547debb
#include <string>
#include <map>
namespace tdc {
using namespace tdc;

class SuffixTree{
public:
    struct STNode;
private:
    //root node
    STNode* root;

    //text added to st
    std::string Text;
    uint text_length;

    //number of suffixes to be added;
    uint remainder;

    //active point, from where to start inserting new suffixes
    STNode* active_node;
    char active_edge;
    uint active_length;


    //saves last added node
    STNode* last_added;


    //computes edge length:
    uint edge_length(STNode* node){

        if(node->end == 0){
            return text_length+1 - node->start;
        } else {
            return node->end - node->start;
        }
    }

public:
    struct STNode{

        //represents the edge leading to this node
        uint start;
        uint end;
        // child nodes
        std::map<char, STNode*> child_nodes;
        //suffix link
        struct STNode* suffix_link;
        //suffix represented by this node, if leaf
        uint suffix;

        //constructor. e=0
        STNode(uint s, uint e = 0) : start(s), end (e){ suffix_link=0;}


    };
    //constructor
    SuffixTree(){
        //no Text is read
        text_length=0;
        Text="";
        remainder=0;


        root = new STNode(0);

        //active start node is root
        active_node=root;
        active_length=0;

    }

    inline void add_char(char c){
        Text += c;
        text_length++;
        remainder++;
        while(remainder > 0){
            if(active_length==0){
                active_edge = c;
            }
            //if the active node doesnt have the corresponding edge:
            auto next_it = active_node->child_nodes.find(active_edge);
            if(next_it==active_node->child_nodes.end()){
                //insert new leaf
                //DLOG(INFO)<<"inserting new leaf rule 1: " << c;

                STNode* new_leaf = new STNode(text_length);
                active_node->child_nodes[active_edge] = new_leaf;
                new_leaf->suffix_link = active_node;
                new_leaf->suffix=text_length;
            } else {
                DLOG(INFO)<<"already " << c;
                STNode* next = active_node->child_nodes[active_edge];
                //if the active length is greater than the edge length:
                //switch active node to that

                if(active_length>= edge_length(next)){
                    active_node = next;
                    active_length -= edge_length(next);
                    active_edge = Text[text_length-active_length];
                    continue;
                }

                //if that suffix is already in the tree::
                if(Text[next->start +active_length] == c){
                    active_length++;
                    next->suffix_link = active_node;
                    break;
                }

                //now split edge if the correct edge is found
                STNode* split = new STNode(next->start, next->start+active_length);
                active_node->child_nodes[active_edge] = split;
                STNode* leaf = new STNode(text_length);
                DLOG(INFO)<<"inserting new leaf rule 2: " << c;
                leaf->suffix=text_length;
                split->child_nodes[c] = leaf;
                next->start+=active_length;
                split->child_nodes[Text[next->start]] = next;
                split->suffix_link=next;
            }
            remainder--;
            if(active_node==root && active_length>0){
                //DLOG(INFO)<<"setting new active edge";
                active_length--;
                active_edge = Text[text_length-remainder];
            }else {
                if(active_node->suffix_link != 0){

                    //DLOG(INFO)<<"setting active node sl";
                    active_node = active_node->suffix_link;
                } else {

                    //DLOG(INFO)<<"setting active node root";
                    active_node = root;
                }

            }
        }

    }
    inline void add_string(std::string input){
        for(int i = 0; i<input.length();i++){
            add_char(input[i]);
        }
    }

    inline std::string get_text(){
        return Text;
    }


    inline SuffixTree::STNode* get_root(){
        return root;
    }



};

}


//represents Edges between Nodes
//start and end end are the corresponding start and end of the factor of this edge


#endif // _INCLUDED_DS_SUFFIX_TREE_HPP

