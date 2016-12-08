#ifndef _INCLUDED_DS_SUFFIX_TREE_HPP
#define _INCLUDED_DS_SUFFIX_TREE_HPP

//Original Implementation::
// https://gist.github.com/makagonov/f7ed8ce729da72621b321f0ab547debb

//from: http://stackoverflow.com/questions/9452701/ukkonens-suffix-tree-algorithm-in-plain-english/9513423#9513423
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
    uint pos;

    //number of suffixes to be added;
    uint remainder;

    //active point, from where to start inserting new suffixes
    STNode* active_node;
    char active_edge;
    uint active_length;


    //saves last added node
    STNode* last_added_sl;


    //computes edge length:
    uint edge_length(STNode* node){

        if(node->end == 0){
            return pos - node->start;
        } else {
            return node->end - node->start +1;
        }
    }
    void add_sl(STNode* node){

        if(last_added_sl != root) {
            last_added_sl->suffix_link=node;
        }
        last_added_sl=node;
    }

public:
    struct STNode{

        //represents the edge leading to this node
        uint start;
        uint end;
        // child nodes
        std::map<char, STNode*> child_nodes;
        //suffix link
        STNode* suffix_link;
        //suffix represented by this node, if leaf
        uint suffix;

        //constructor. e=0
        STNode(uint s, uint e = 0) : start(s), end (e){ suffix_link=NULL;}


    };
    //constructor
    SuffixTree(){
        //no Text is read
        pos=0;
        Text="";
        remainder=0;


        root = new STNode(0);

        //active start node is root
        active_node=root;
        active_length=0;

        last_added_sl=root;

    }
    ~SuffixTree(){
        root=NULL;

        active_node=root;
        last_added_sl=root;
    }

    inline void add_char(char c){
        Text += c;
        pos++;
        remainder++;
        last_added_sl=root;

        while(remainder > 0){
            if(active_length==0){
                active_edge = c;
            }



            //if the active node doesnt have the corresponding edge:
            auto next_it = active_node->child_nodes.find(active_edge);
            if(next_it==active_node->child_nodes.end()){
                //insert new leaf

                STNode* new_leaf = new STNode(pos-1);
                active_node->child_nodes[active_edge] = new_leaf;
                new_leaf->suffix=pos-1;

                add_sl(active_node);

            } else {
                STNode* next = active_node->child_nodes[active_edge];
                //if the active length is greater than the edge length:
                //switch active node to that
                if(active_length>= edge_length(next)){
                    active_node = next;
                    active_length -= edge_length(next);
                    active_edge = Text[pos-active_length];
                    continue;
                }

                //if that suffix is already in the tree::
                if(Text[next->start +active_length] == c){
                    active_length++;
                    add_sl(active_node);
                    break;
                }

                //now split edge if the edge is found

                STNode* split = new STNode(next->start, next->start+active_length-1);
                active_node->child_nodes[active_edge] = split;
                STNode* leaf = new STNode(pos-1);
                leaf->suffix=pos-1;
                //leaf->suffix_link=active_node;
                //last_added_sl->suffix_link = leaf;
                split->child_nodes[c] = leaf;

                next->start=next->start + active_length;
                split->child_nodes[Text[next->start]] = next;
                //split->suffix_link=next;
                add_sl(split);
            }
            remainder--;
            if(active_node==root && active_length>0){
                active_length--;
                active_edge = Text[pos-remainder];
            }else {
                if(active_node->suffix_link != NULL){
                    active_node = active_node->suffix_link;
                } else {
                    active_node = root;
                }

            }
        }

    }
    inline void add_string(std::string input){
        for(uint i = 0; i<input.length();i++){
            add_char(input[i]);
        }
    }

    inline std::string get_text(){
        return Text;
    }


    inline SuffixTree::STNode* get_root(){
        return root;
    }

    inline std::string get_string_of_edge(STNode* node){
        std::string output ="";
        uint e;
        if(node->end==0){
            e=pos;
        } else {
            e = node->end;
        }
        for(uint i = node->start;i<=e;i++){
            output+=Text[i];
        }
        return output;
    }



};

}


#endif // _INCLUDED_DS_SUFFIX_TREE_HPP

