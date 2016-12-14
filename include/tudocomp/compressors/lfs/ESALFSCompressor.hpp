#ifndef _INCLUDED_ESA_LFS_COMPRESSOR_HPP_
#define _INCLUDED_ESA_LFS_COMPRESSOR_HPP_



#include <tudocomp/Compressor.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/util.hpp>

#include <tudocomp/io.hpp>

//#include <iostream>

#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/io/BitIStream.hpp>
#include <tudocomp/io/BitOStream.hpp>


//#include <tudocomp/tudocomp.hpp>


namespace tdc {

template<typename literal_coder_t, typename len_coder_t>
class ESALFSCompressor : public Compressor {
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
        m.option("lit_coder").templated<literal_coder_t>();
        m.option("len_coder").templated<len_coder_t>();
        m.needs_sentinel_terminator();
        return m;
    }


    inline ESALFSCompressor(Env&& env):
        Compressor(std::move(env))
    {
        DLOG(INFO) << "Compressor instantiated";

    }
    inline virtual void compress(Input& input, Output& output) override {


        uint min_lrf_length = 5;
        //creating lcp and sa
        auto in = input.as_view();

        //TextDS<> t(in);
        text_t t(in);
        DLOG(INFO) << "building sa and lcp";
        t.require(text_t::SA | text_t::ISA | text_t::LCP);
        auto& sa_t = t.require_sa();
        auto& lcp_t = t.require_lcp();

        // iterate over lcp array, add indexes with non overlapping prefix length greater than min_lrf_length to vector
        std::vector<std::pair<uint,uint>> lrf_occurences;


        DLOG(INFO) << "iterate over lcp";
        uint dif ;
        uint factor_length;
        for(uint i = 1; i<lcp_t.size(); i++){

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
                std::pair<uint,uint> pair (factor_length, i);
                if(factor_length>=min_lrf_length){
                    lrf_occurences.push_back(pair);
                }
            }
        }

        // Pop PQ, Select occurences of suffix, check if contains replaced symbols
        sdsl::bit_vector non_terminals(sa_t.size(), 0);
        //Pq for the non-terminal symbols
        //the first in pair is position, the seconds the number of the non terminal symbol

        DLOG(INFO) << "computing lrfs";

        //vector of text position, length
        std::vector<std::pair<uint,uint>> dictionary;

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





        // encode dictionary:
        DLOG(INFO) << "encoding dictionary symbol sizes ";

        std::shared_ptr<BitOStream> bitout = std::make_shared<BitOStream>(output);
        typename literal_coder_t::Encoder lit_coder(
            env().env_for_option("lit_coder"),
            bitout,
            NoLiterals()
        );
        typename len_coder_t::Encoder len_coder(
            env().env_for_option("len_coder"),
            bitout,
            NoLiterals()
        );

        auto it = dictionary.begin();
        //range int_r (0,MAX_INT);

        Range intrange (0, UINT_MAX);//uint32_r
        if(dictionary.size() >=1 ){

            std::pair<uint,uint> symbol = *it;
            uint last_length=symbol.second;
            Range s_length_r (0,last_length);
            len_coder.encode(last_length,intrange);
            it++;

            while (it != dictionary.end()){
                symbol=*it;
                len_coder.encode(last_length-symbol.second,s_length_r);
                last_length=symbol.second;
                it++;

            }
            len_coder.encode(symbol.second,s_length_r);
        }else {
            len_coder.encode(0,intrange);

        }


        DLOG(INFO) << "encoding dictionary symbols";
        // encode dictionary strings:
        if(dictionary.size() >=1 ){
            auto it = dictionary.begin();
            std::pair<uint,uint> symbol;// = *it;
            //Range slength_r (0, symbol.second);
            //coder.encode(symbol.second,intrange);//uint32_r

             while(it != dictionary.end()){
            //first length of non terminal symbol
                symbol = *it;
                //coder.encode(symbol.second,slength_r);

                for(uint k = 0; k<symbol.second; k++){
                    lit_coder.encode(t[symbol.first + k],literal_r);
                }
                it++;
            }
        }

        Range dict_r(0, dictionary.size());
        //encode string
        uint pos = 0;
        uint start_position;
        uint symbol_number ;
        uint symbol_length;
        bool first_char = true;
        for(auto it = non_terminal_symbols.begin(); it!= non_terminal_symbols.end(); it++){


            std::tuple<uint,uint,uint> next_position = *it;

            start_position = std::get<0>(next_position);
            symbol_number =std::get<1>(next_position);
            symbol_length = std::get<2>(next_position);

            while(pos< start_position){
                //get original text, because no symbol...
                lit_coder.encode(0, bit_r);
                lit_coder.encode(t[pos], literal_r);

                if(first_char){
                    first_char=false;
                }
                pos++;
            }

            //write symbol number

            lit_coder.encode(1, bit_r);
            lit_coder.encode(symbol_number, dict_r);

            pos += symbol_length;

        }
        //if no more terminals, write rest of text
        //one pos less, because ensure_null_ending adds a symbol
        while( pos<t.size()-1){

            lit_coder.encode(0, bit_r);
            lit_coder.encode(t[pos], literal_r);
            pos++;
        }

        DLOG(INFO) << "compression with lfs done";

    }

    inline virtual void decompress(Input& input, Output& output) override {
        DLOG(INFO) << "decompress lfs";
        std::shared_ptr<BitIStream> bitin = std::make_shared<BitIStream>(input);

        typename literal_coder_t::Decoder lit_decoder(
            env().env_for_option("lit_coder"),
            bitin
        );
        typename len_coder_t::Decoder len_decoder(
            env().env_for_option("len_coder"),
            bitin
        );
        Range int_r (0,UINT_MAX);

        uint symbol_length = len_decoder.template decode<uint>(int_r);
        Range slength_r (0, symbol_length);
        std::vector<uint> dict_lengths;
        dict_lengths.reserve(symbol_length);
        dict_lengths.push_back(symbol_length);
        while(symbol_length>0){

            uint current_delta = len_decoder.template decode<uint>(slength_r);
            symbol_length-=current_delta;
            dict_lengths.push_back(symbol_length);
        }
        dict_lengths.pop_back();

        std::vector<std::string> dictionary;
        uint dictionary_size = dict_lengths.size();

        Range dictionary_r (0, dictionary_size);


        uint length_of_symbol;
        std::string non_terminal_symbol;
        DLOG(INFO) << "reading dictionary";
        for(uint i = 0; i< dict_lengths.size();i++){
            non_terminal_symbol ="";
            char c1;
            length_of_symbol=dict_lengths[i];
            for(uint i =0; i< length_of_symbol;i++){
                c1 = lit_decoder.template decode<char>(literal_r);
                non_terminal_symbol += c1;
            }
            dictionary.push_back(non_terminal_symbol);
        }
        auto ostream = output.as_stream();

        while(!lit_decoder.eof()){
            //decode bit
            bool bit1 = lit_decoder.template decode<bool>(bit_r);
            char c1;
            uint symbol_number;
            // if bit = 0 its a literal
            if(!bit1){
                c1 = lit_decoder.template decode<char>(literal_r); // Dekodiere Literal

                ostream << c1;
            } else {
            //else its a non-terminal
                symbol_number = lit_decoder.template decode<uint>(dictionary_r); // Dekodiere Literal

                if(symbol_number < dictionary.size()){

                    ostream << dictionary.at(symbol_number);
                } else {
                    DLOG(INFO)<< "too large symbol: " << symbol_number;
                }

            }
        }
    }

};

}


#endif
