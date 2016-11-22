#ifndef _INCLUDED_LFS_COMPRESSOR_HPP_
#define _INCLUDED_LFS_COMPRESSOR_HPP_

//#include <tudocomp/tudocomp.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Compressor.hpp>

#include <tudocomp/io.hpp>
#include <chrono>
#include <thread>

//#include <iostream>
#include <sdsl/lcp.hpp>
#include <sdsl/suffix_arrays.hpp>
#include <sdsl/bit_vectors.hpp>

#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/ds/SuffixArray.hpp>

#include <tudocomp/tudocomp.hpp>

//#include <sdsl/suffixtrees.hpp>

namespace tdc {

template<typename coder_t>
class LFSCompressor : public Compressor {
private:
    typedef TextDS<> text_t;

public:
    inline static Meta meta() {
        Meta m("compressor", "longest_first_substitution_compressor",
            "This is an implementation of the longest first substitution compression scheme.");
        m.option("coder").templated<coder_t>();
        return m;
    }


    inline LFSCompressor(Env&& env):
        Compressor(std::move(env))
    {
        // ...

        DLOG(INFO) << "Compressor instantiated";

    }
    inline virtual void compress(Input& input, Output& output) override {

        // Kodierer instanziieren
        typename coder_t::Encoder coder(
                    env().env_for_option("coder"), // Environment für den Kodierer //.env_for_option("coder")
                                                   // (könnte z.B. Optionen haben)
                    output,                        // Die zu verwendende Ausgabe
                    NoLiterals());                 // Kein Literal-Iterator
                                                   // (erkläre ich nochmal)

                // [!] Alle Ausgaben sollten nun über den Kodierer laufen

        uint min_lcp_length = 4;
        DLOG(INFO) << "compress lfs";
        //auto ostream = output.as_stream();
        //creating lcp and sa
        DLOG(INFO) << "reading input";
        auto in = input.as_view();
        in.ensure_null_terminator();
        //TextDS<> t(in);
        text_t t(in);
        DLOG(INFO) << "building sa and lcp";
        t.require(text_t::SA | text_t::ISA | text_t::LCP);

        DLOG(INFO) << "done building sa and lcp";
        auto& sa_t = t.require_sa();
        auto& lcp_t = t.require_lcp();

        // iterate over lcp array, add indexes with non overlapping prefix length greater than 2 to pq
        std::priority_queue<std::pair<int,int>> pq;
        //std::vector<std::string> dictionary;

        //vector of position, length
        std::vector<std::pair<int,int>> dictionary;

        DLOG(INFO) << "iterate over lcp";
        int dif ;
        int factor_length;
        for(uint i = 1; i<lcp_t.size(); i++){

            if(lcp_t[i] >= min_lcp_length){
                //compute length of non-overlapping factor:

                if(sa_t[i-1] > sa_t[i]){
                    dif =  sa_t[i-1] - sa_t[i];
                } else {
                    dif =  sa_t[i] - sa_t[i-1];
                }
                factor_length = lcp_t[i];


                if(dif < factor_length) {
                    factor_length = dif;
                } else {

                }
                //second is position in suffix array
                //first is length of common prefix
                std::pair<int,int> pair (factor_length, i);
                if(factor_length>1){

                  //  DLOG(INFO) << "found lrf";
                    pq.push(pair);
                }
            }
        }

        // Pop PQ, Select occurences of suffix, check if contains replaced symbols
        sdsl::bit_vector non_terminals(sa_t.size(), 0);
        //Pq for the non-terminal symbols
        //the first in pair is position, the seconds the number of the non terminal symbol

        DLOG(INFO) << "computing lrfs";
        //std::priority_queue<std::tuple<int,int,int>, std::vector<std::tuple<int,int,int>>, std::greater<std::tuple<int,int,int>> > non_terminal_symbols;

        std::vector<std::tuple<int,int,int>> non_terminal_symbols;
        int non_terminal_symbol_number = 0;
        while(!pq.empty()){
            std::pair<int,int> top = pq.top();
            pq.pop();

            if(non_terminals[sa_t[top.second]] == 1 || non_terminals[sa_t[top.second-1]] == 1){
                continue;
            }

            //detect all starting positions of this string using the sa and lcp:
            std::vector<int> starting_positions;

            starting_positions.push_back(sa_t[top.second]);

            int i = top.second;
            while(i>=0 && ((int) lcp_t[i])>=top.first){
                if(non_terminals[sa_t[i-1]] == 0){
                    starting_positions.push_back(sa_t[i-1]);
                }
                i--;
            }
            i = top.second+1;
            while(i< (int)lcp_t.size() &&((int)  lcp_t[i])>=top.first){
                if(non_terminals[sa_t[i]] == 0){
                    starting_positions.push_back(sa_t[i]);
                }
                i++;
            }
            std::sort(starting_positions.begin(), starting_positions.end());
            //DLOG(INFO) << "starting positions: " << starting_positions.size();

            //select occurences greedily non-overlapping:
            std::vector<int> selected_starting_positions;
            selected_starting_positions.reserve(starting_positions.size());

            int last =  0-top.first-1;
            int current;
            //selected_starting_positions.push_back(last);
            for (std::vector<int>::iterator it=starting_positions.begin(); it!=starting_positions.end(); ++it){

                current = *it;

                //DLOG(INFO) << "checking starting position: " << current << " length: " << top.first << "last " << last;
                if(!(last+top.first>current)){
                    selected_starting_positions.push_back(current);
                }
                last = current;
            }
           // DLOG(INFO) << "selected starting positions: " << selected_starting_positions.size();

            //now ceck in bitvector viable starting positions
            // there is no 1 bit on the corresponding positions

            std::vector<int>::iterator it=selected_starting_positions.begin();

            while(it!=selected_starting_positions.end()){
                bool non_viable = false;
                for(int k = 0; k<top.first; k++){
                    if(non_terminals[*it+k]==1){
                        non_viable=true;
                        break;
                    }
                }
                if(non_viable){
                    it = selected_starting_positions.erase(it);
                } else {
                    it++;
                }
            }
            //if the factor is still repeating, make the corresponding positions unviable

            if(selected_starting_positions.size()>=2){
                //computing substring to be replaced
                int offset = sa_t[top.second-1];
                std::pair<int,int> substring(offset, top.first);
                for (std::vector<int>::iterator it=selected_starting_positions.begin(); it!=selected_starting_positions.end(); ++it){
                    for(int k = 0; k<top.first; k++){
                        non_terminals[*it+k]=1;
                    }

                    int length_of_symbol = top.first;
                    std::tuple<int,int,int> symbol(*it, non_terminal_symbol_number, length_of_symbol);
                    non_terminal_symbols.push_back(symbol);
                }
                dictionary.push_back(substring);
                non_terminal_symbol_number++;
            }
        }

        std::make_heap(non_terminal_symbols.begin(), non_terminal_symbols.end(), std::greater<std::tuple<int,int,int>>());



        DLOG(INFO) << "encoding dictionary";

        DLOG(INFO) << "dictionary size: " << dictionary.size();
        Range dict_r(0, dictionary.size());
        coder.encode(dictionary.size(),uint16_r);


        // encode dictionary:
        if(dictionary.size() >=1 ){
            auto it = dictionary.begin();
            std::pair<int,int> symbol = *it;
            Range slength_r (0, symbol.second);
            coder.encode(symbol.second,uint16_r);

             while(it != dictionary.end()){
            //first length of non terminal symbol
            symbol = *it;
            coder.encode(symbol.second,slength_r);

            for(int k = 0; k<symbol.second; k++){
                coder.encode(t[symbol.first + k],literal_r);
            }
                it++;
            }

             coder.encode(0,slength_r);
        } else {
            coder.encode(1,uint16_r);

            Range min_range (0,1);
            coder.encode(0,min_range);
        }
        DLOG(INFO) << "encoding string";
        //encode string
        int pos = 0;
        int start_position;
        int symbol_number ;
        int symbol_length;
        //int i=0;  &&i<10 i++;
        while(!non_terminal_symbols.empty() ){

            std::tuple<int,int,int> next_position = non_terminal_symbols.front();

            start_position = std::get<0>(next_position);
            symbol_number =std::get<1>(next_position);
            symbol_length = std::get<2>(next_position);

            std::pop_heap(non_terminal_symbols.begin(),non_terminal_symbols.end(), std::greater<std::tuple<int,int,int>>());
            non_terminal_symbols.pop_back();
            while(pos< start_position){
                //get original text, because no symbol...
                //output_string+=t[pos];
                coder.encode(0, bit_r);
                coder.encode(t[pos], literal_r);
                pos++;
            }

            //write symbol number

            coder.encode(1, bit_r);
            coder.encode(symbol_number, dict_r);

            pos += symbol_length;

        }
        //start_position=0;
        //if no more terminals, write rest of text
        //one pos less, because ensure_null_ending adds a symbol
        //TODO!!
        while( pos<(int)t.size()-1){

            coder.encode(0, bit_r);
            coder.encode(t[pos], literal_r);
            pos++;
        }

        DLOG(INFO) << "compression with lfs done";

    }

    inline virtual void decompress(Input& input, Output& output) override {
        DLOG(INFO) << "decompress lfs";



        typename coder_t::Decoder decoder(
                    env().env_for_option("coder"), // Environment
                    input);                        // Eingabe

        std::vector<std::string> dictionary;
        bool reading_dictionary=true;
        int dictionary_size = decoder.template decode<int>(uint16_r);

        int slength = decoder.template decode<int>(uint16_r);

        Range dictionary_r (0, dictionary_size);
        Range slength_r (0, slength);

        int length_of_symbol;
        std::string non_terminal_symbol;
        DLOG(INFO) << "reading dictionary";
        while(reading_dictionary){;
            length_of_symbol = decoder.template decode<int>(slength_r); // Dekodiere Literal
            //length_of_symbol = (int) length;
            non_terminal_symbol ="";

            char c1;
            if(length_of_symbol<=0) {
                reading_dictionary=false;
            } else {
                for(int i =0; i< length_of_symbol;i++){
                    c1 = decoder.template decode<char>(literal_r);
                    non_terminal_symbol += c1;
                }
                dictionary.push_back(non_terminal_symbol);
            }
        }

        DLOG(INFO) << "reading string";
        auto ostream = output.as_stream();
        //std::string output_string;
       // bool end_of_text = false;

        while(!decoder.eof()){
            //decode bit
            bool bit1 = decoder.template decode<bool>(bit_r);
            char c1;
            int symbol_number;
            // if bit = 0 its a literal
            if(!bit1){
                c1 = decoder.template decode<char>(literal_r); // Dekodiere Literal

                ostream << c1;
            } else {
            //else its a non-terminal
                symbol_number = decoder.template decode<int>(dictionary_r); // Dekodiere Literal

                //int symbol_number = (int) c1;

                ostream << dictionary.at(symbol_number);
            }
        }
    }

};

}


#endif
