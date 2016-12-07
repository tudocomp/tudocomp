#ifndef _INCLUDED_LFS_COMPRESSOR_HPP_
#define _INCLUDED_LFS_COMPRESSOR_HPP_



#include <tudocomp/Compressor.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/util.hpp>

#include <tudocomp/io.hpp>

//#include <iostream>

#include <tudocomp/ds/TextDS.hpp>

//#include <tudocomp/tudocomp.hpp>


namespace tdc {

template<typename coder_t>
class LFSCompressor : public Compressor {
private:
    typedef TextDS<> text_t;




    inline virtual std::vector<uint> select_starting_positions(std::vector<uint> starting_positions, uint length){
        std::vector<uint> selected_starting_positions;
        std::sort(starting_positions.begin(), starting_positions.end());
        //select occurences greedily non-overlapping:
        selected_starting_positions.reserve(starting_positions.size());

        int last =  0-length;
        uint current;
        for (std::vector<uint>::iterator it=starting_positions.begin(); it!=starting_positions.end(); ++it){

            current = *it;
            //DLOG(INFO) << "checking starting position: " << current << " length: " << top.first << "last " << last;
            if(last+length <= current){
                selected_starting_positions.push_back(current);
                last = current;

            }

        }
        return selected_starting_positions;
    }



public:
    inline static Meta meta() {
        Meta m("compressor", "longest_first_substitution_compressor",
            "This is an implementation of the longest first substitution compression scheme.");
        m.option("coder").templated<coder_t>();
        m.needs_sentinel_terminator();
        return m;
    }


    inline LFSCompressor(Env&& env):
        Compressor(std::move(env))
    {
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

        uint min_lrf_length = 5;
        DLOG(INFO) << "compress lfs";
        //auto ostream = output.as_stream();
        //creating lcp and sa
        DLOG(INFO) << "reading input";
        auto in = input.as_view();
        //in.ensure_null_terminator();


        //TextDS<> t(in);
        text_t t(in);
        DLOG(INFO) << "building sa and lcp";
        t.require(text_t::SA | text_t::ISA | text_t::LCP);

        DLOG(INFO) << "done building sa and lcp";
        auto& sa_t = t.require_sa();
        auto& lcp_t = t.require_lcp();

        // iterate over lcp array, add indexes with non overlapping prefix length greater than 2 to vector
        std::vector<std::pair<uint,uint>> lrf_occurences;
        //std::vector<std::string> dictionary;

        //vector of position, length
        std::vector<std::pair<uint,uint>> dictionary;

        DLOG(INFO) << "iterate over lcp";
        uint dif ;
        uint factor_length;
        for(int i = 1; (uint)i<lcp_t.size(); i++){

            if(lcp_t[i] >= min_lrf_length){
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
                std::pair<uint,uint> pair (factor_length,(uint) i);
                if(factor_length>1){

                  //  DLOG(INFO) << "found lrf";
                    lrf_occurences.push_back(pair);
                }
            }
        }

        // Pop PQ, Select occurences of suffix, check if contains replaced symbols
        sdsl::bit_vector non_terminals(sa_t.size(), 0);
        //Pq for the non-terminal symbols
        //the first in pair is position, the seconds the number of the non terminal symbol

        DLOG(INFO) << "computing lrfs";

        std::sort(lrf_occurences.begin(),lrf_occurences.end());

        std::vector<std::tuple<uint,uint,uint>> non_terminal_symbols;
        non_terminal_symbols.reserve(lrf_occurences.size());
        uint non_terminal_symbol_number = 0;
        while(!lrf_occurences.empty()){
            std::pair<uint,uint> top = lrf_occurences.back();
            lrf_occurences.pop_back();

            if(non_terminals[sa_t[top.second]] == 1 || non_terminals[sa_t[top.second-1]] == 1 || non_terminals[sa_t[top.second]+top.first-1] == 1 || non_terminals[sa_t[top.second-1]+top.first-1] == 1){
                continue;
            }

            //detect all starting positions of this string using the sa and lcp:
            std::vector<uint> starting_positions;

            starting_positions.push_back(sa_t[top.second]);



            // and ceck in bitvector viable starting positions
            // there is no 1 bit on the corresponding positions
            // it suffices to check start and end position, because lrf can only be same length and shorter
            uint i = top.second;
            while(i>=0 && ( lcp_t[i])>=top.first){
                if(non_terminals[sa_t[i-1]] == 0 && non_terminals[sa_t[i-1]+top.first-1] == 0){
                    starting_positions.push_back(sa_t[i-1]);
                }
                i--;
            }
            i = top.second+1;
            while(i< lcp_t.size() &&  lcp_t[i]>=top.first){
                if(non_terminals[sa_t[i]] == 0 && non_terminals[sa_t[i]+top.first-1] == 0){
                    starting_positions.push_back(sa_t[i]);
                }
                i++;
            }







            //if the factor is still repeating, make the corresponding positions unviable

            if(starting_positions.size()>=2){
                std::vector<uint> selected_starting_positions = select_starting_positions(starting_positions, top.first);
                //computing substring to be replaced
                if(selected_starting_positions.size()>=2){



                    uint offset = sa_t[top.second-1];
                    std::pair<uint,uint> longest_repeating_factor(offset, top.first);
                    for (std::vector<uint>::iterator it=selected_starting_positions.begin(); it!=selected_starting_positions.end(); ++it){
                        for(uint k = 0; k<top.first; k++){
                            non_terminals[*it+k]=1;
                        }

                        uint length_of_symbol = top.first;
                        std::tuple<uint,uint,uint> symbol(*it, non_terminal_symbol_number, length_of_symbol);
                        non_terminal_symbols.push_back(symbol);
                    }
                    dictionary.push_back(longest_repeating_factor);
                    non_terminal_symbol_number++;
                }
            }
        }
        DLOG(INFO) << "sorting occurences";
        //, std::greater<std::tuple<uint,uint,uint>>()
        std::sort(non_terminal_symbols.begin(), non_terminal_symbols.end());


        Range intrange (0, UINT_MAX);//uint32_r

        // encode dictionary:
        DLOG(INFO) << "encoding dictionary";
        coder.encode(dictionary.size(),intrange);//uint32_r
        // encode dictionary:
        if(dictionary.size() >=1 ){
            auto it = dictionary.begin();
            std::pair<uint,uint> symbol = *it;
            Range slength_r (0, symbol.second);
            coder.encode(symbol.second,intrange);//uint32_r

             while(it != dictionary.end()){
            //first length of non terminal symbol
            symbol = *it;
            coder.encode(symbol.second,slength_r);

            for(uint k = 0; k<symbol.second; k++){
                coder.encode(t[symbol.first + k],literal_r);
            }
                it++;
            }

             coder.encode(0,slength_r);
        } else {
            coder.encode(1,intrange);

            Range min_range (0,1);
            coder.encode(0,min_range);
        }


        DLOG(INFO) << "dictionary size: " << dictionary.size();
        Range dict_r(0, dictionary.size());

        DLOG(INFO) << "encoding string";
        //encode string
        uint pos = 0;
        uint start_position;
        uint symbol_number ;
        uint symbol_length;
        //int i=0;  &&i<10 i++;
        for(auto it = non_terminal_symbols.begin(); it!= non_terminal_symbols.end(); it++){
        //while(!non_terminal_symbols.empty() ){

            std::tuple<uint,uint,uint> next_position = *it;

            start_position = std::get<0>(next_position);
            symbol_number =std::get<1>(next_position);
            symbol_length = std::get<2>(next_position);

            //std::pop_heap(non_terminal_symbols.begin(),non_terminal_symbols.end(), std::greater<std::tuple<uint,uint,uint>>());
            //non_terminal_symbols.pop_back();
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
        //if no more terminals, write rest of text
        //one pos less, because ensure_null_ending adds a symbol
        //TODO!!
        while( pos<t.size()-1){

            coder.encode(0, bit_r);
            coder.encode(t[pos], literal_r);
            pos++;
        }

        DLOG(INFO) << "compression with lfs done";

    }

    inline virtual void decompress(Input& input, Output& output) override {
        DLOG(INFO) << "decompress lfs";

        Range intrange (0, UINT_MAX);//uint32_r

        typename coder_t::Decoder decoder(
                    env().env_for_option("coder"), // Environment
                    input);                        // Eingabe

        std::vector<std::string> dictionary;
        bool reading_dictionary=true;
        uint dictionary_size = decoder.template decode<uint>(intrange);

        uint slength = decoder.template decode<uint>(intrange);

        Range dictionary_r (0, dictionary_size);
        Range slength_r (0, slength);

        uint length_of_symbol;
        std::string non_terminal_symbol;
        DLOG(INFO) << "reading dictionary";
        while(reading_dictionary){;
            length_of_symbol = decoder.template decode<uint>(slength_r); // Dekodiere Literal
            //length_of_symbol = (int) length;
            non_terminal_symbol ="";

            char c1;
            if(length_of_symbol<=0) {
                reading_dictionary=false;
            } else {
                for(uint i =0; i< length_of_symbol;i++){
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
            uint symbol_number;
            // if bit = 0 its a literal
            if(!bit1){
                c1 = decoder.template decode<char>(literal_r); // Dekodiere Literal

                ostream << c1;
            } else {
            //else its a non-terminal
                symbol_number = decoder.template decode<uint>(dictionary_r); // Dekodiere Literal

                //int symbol_number = (int) c1;

                ostream << dictionary.at(symbol_number);
            }
        }
    }

};

}


#endif
