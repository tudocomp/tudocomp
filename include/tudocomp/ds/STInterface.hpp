#pragma once

namespace tdc {

template<typename node_type, typename size_type = uint>
class STInterface{
private:


    const io::InputView& Text;


    int pos;

    //number of suffixes to be added;
    uint remainder;

    node_type active_node;
    uint8_t active_edge;
    size_type active_length;
    size_type current_suffix;

    node_type last_added_sl;


    void add_sl(uint node){

        if(last_added_sl != get_root()) {

            set_suffix_link(last_added_sl, node);
        }
        last_added_sl=node;
    }


    inline void add_char(char c){
        //Text += c;
        pos++;
        remainder++;


        while(remainder > 0){
            if(active_length==0){
                active_edge = c;
            }



            //if the active node doesnt have the corresponding edge:
            //find edge corresponding to added active edge
            bool found = false;
            node_type child  = get_child(active_node, active_edge);
            if(child != active_node){
                found = true;
            }

            //if not found
            if(!found){
                //insert new leaf
                node_type leaf = add_child(active_node,active_edge,pos,current_suffix++);
                add_sl(active_node);

            } else {
               // uint next =  child ;
                //if the active length is greater than the edge length:
                //switch active node to that
                //walk down
                if(active_length>= get_edge_length(child)){
                    active_node = child;
                    active_length -= get_edge_length(child);
                    active_edge = (char) Text[pos-active_length];
                    continue;
                }

                //if that suffix is already in the tree::
                //Text[start[next]+active_length]
                if( (char)  get_edge_label(child, active_length) == c){
                    active_length++;
                    add_sl(active_node);
                    break;
                }

                //now split edge if the edge is found

                node_type split = split_edge(active_node, active_length, active_edge);

                //        create_node(start[next], start[next]+active_length, active_edge);
                //active_inner->child_nodes[active_edge] = split;

                //node_type leaf =
                add_child(split,active_edge, pos,current_suffix++);


                add_sl(split);
       //         leaves.push_back(leaf);
            }
            remainder--;
            if(active_node==get_root() && active_length>0){
                active_length--;
                active_edge = Text[pos-remainder+1];
            }else {
                if(get_suffix_link(active_node) != get_root()){
                    active_node = get_suffix_link(active_node);
                } else {
                    active_node = get_root();
                }

            }
        }

    }


    void construct(){

        pos=-1;
        remainder=0;
        current_suffix=0;

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

    STInterface(io::InputView& in) : Text(in) {

        construct();
    }

    virtual auto add_child(node_type node, char c, size_type start, size_type suffix) -> void;
    //virtual auto set_start(node_type node, size_type start) -> void;
    //virtual auto set_end(node_type node, size_type end) -> void;
    //virtual auto set_suffix(node_type node, size_type suffix) -> void;

    virtual auto split_edge(node_type node, size_type edge_len, char c) -> node_type;


    virtual auto get_edge_length(node_type node) -> size_type;
    virtual auto get_edge_label(node_type node, size_type pos) -> char;

    virtual auto get_suffix(node_type node) -> size_type;


    virtual auto is_leaf(node_type node) -> bool;
    virtual auto get_child(node_type node, char c) -> node_type;
    virtual auto get_child(node_type node) -> std::vector<node_type>;

    virtual auto get_suffix_link(node_type node) -> node_type;
    virtual auto set_suffix_link(node_type from_node, node_type to_node) -> void;


    virtual auto get_root() -> node_type;
    virtual auto get_tree_size() -> size_type;


};

}
