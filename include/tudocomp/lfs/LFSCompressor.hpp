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

//#include <sdsl/suffixtrees.hpp>

namespace tdc {

class LFSCompressor : public Compressor {
private:

public:
    inline static Meta meta() {
        Meta m("compressor", "longest_first_substitution_compressor",
            "This is an implementation of the longest first substitution compression scheme.");
        return m;
    }

    /*inline LFSCompressor(Env&& env) : Compressor(std::move(env)) {
        DLOG(INFO) << "output"<< std::endl;
    }*/

    inline LFSCompressor(Env&& env):
        Compressor(std::move(env))
    {
        // ...

        DLOG(INFO) << "Compressor instantiated"<< std::endl;
        //env.begin_stat_phase("LFS compression");

    }
    inline virtual void compress(Input& input, Output& output) override {
        //env().begin_stat_phase("LFS compression");
       // env().log_stat("A statistic", 147);
        //build lcp array and suffix array:
        //auto istream = input.as_stream();
        auto ostream = output.as_stream();




        //alternative method of creating lcp and sa
         auto in = input.as_view();
        TextDS<> t(in);
      //  env.begin_stat_phase("Construct text ds");
        t.require(TextDS<>::SA | TextDS<>::LCP);
       // env.end_stat_phase();


       // t.print(DLOG(INFO));

        auto& sa_t = t.require_sa();
        auto& lcp_t = t.require_lcp();
        //t.sa[0];
       // DLOG(INFO) << sa_t[0] << std::endl;



       // std::string file = "test_files/lfs_test.txt";
        //std::string text;
       // char cur_char;
       // while(in.get(cur_char)){
       //     text += cur_char;
       // }


        //just necessary for construct()
        //std::ofstream myfile;
        //myfile.open (file);
        //myfile << text;
        //myfile.close();



       // sdsl::cache_config cc(false); // do not delete temp files after csa construction
      //  sdsl::csa_wt<> csa;
      //  sdsl::construct(csa, file, 1);

      //  cc.delete_files = true; // delete temp files after lcp construction
       // sdsl::lcp_wt<> lcp;
       // sdsl::construct(lcp, file, 1);

        //DLOG(INFO) << "Text:" << std::endl << text << std::endl  << std::endl;

        //DLOG(INFO) << "suffix array:" << std::endl;
       // DLOG(INFO) << csa << std::endl;
        //DLOG(INFO) << "-------" << std::endl;
        //DLOG(INFO) << "lcp array:" << std::endl;
        //DLOG(INFO) << lcp << std::endl;

        //DLOG(INFO) << lcp << std::endl;
        // iterate over lcp array, add indexes with non overlapping prefix length grater than 2 to pq
        std::priority_queue<std::pair<int,int>> pq;

        std::list<std::string> dictionary;

        int dif ;
        int factor_length;
        for(uint i = 1; i<lcp_t.size(); i++){

            //DLOG(INFO) <<  lcp[i] << " ";
            if(lcp_t[i] >= 2){
                //compute length of non-overlapping factor:

                if(sa_t[i-1] > sa_t[i]){
                    dif =  sa_t[i-1] - sa_t[i];
                } else {
                    dif =  sa_t[i] - sa_t[i-1];
                }
                factor_length = lcp_t[i];


                if(dif < factor_length) {
                    factor_length = dif;
                }
                //second is position in suffix array
                //first is length of common prefix
                std::pair<int,int> pair (factor_length, i);
                if(factor_length>1){
                    pq.push(pair);
                }
            }
        }
       // DLOG(INFO) << std::endl;
       // DLOG(INFO) << "repeating factors:" << std::endl;
        // Pop PQ, Select occurences of suffix, check if contains replaced symbols
        sdsl::bit_vector non_terminals(sa_t.size(), 0);
        //Pq for the non-terminal symbols
        //the first in pair is position, the seconds the number of the non terminal symbol
        std::priority_queue<std::tuple<int,int,int>, std::vector<std::tuple<int,int,int>>, std::greater<std::tuple<int,int,int>> > non_terminal_symbols;
        int non_terminal_symbol_number = 1;
        while(!pq.empty()){

            std::pair<int,int> top = pq.top();
          //  DLOG(INFO) << "pos1: "<< sa_t[top.second] << " pos2: "<< sa_t[top.second-1] << " len: " << top.first << "; ";
          //  DLOG(INFO) << std::endl;
            pq.pop();

            std::string substring;// = text.substr(sa_t[top.second-1], top.first);
            int offset= sa_t[top.second-1];
            for(int k = 0; k<top.first;k++){
                substring += t[offset+k];
            }
          //  DLOG(INFO) << "string: " << substring << std::endl;

            //detect all starting positions of this string using the sa and lcp:
            std::vector<int> starting_positions;

            starting_positions.push_back(sa_t[top.second]);
           // DLOG(INFO) <<"starting positions: " << std::endl;
            int i = top.second;
            while(i>=0 && lcp_t[i]>=top.first){
                starting_positions.push_back(sa_t[i-1]);
                //DLOG(INFO) << sa_t[i-1] << " ";
                i--;
            }
            i = top.second+1;
            while(i< lcp_t.size() && lcp_t[i]>=top.first){
                starting_positions.push_back(sa_t[i]);
                //DLOG(INFO) << csa[i];
                i++;
            }
            std::sort(starting_positions.begin(), starting_positions.end());
          //  for (std::vector<int>::iterator it=starting_positions.begin(); it!=starting_positions.end(); ++it)
          //      DLOG(INFO) << ' ' << *it;
          //  DLOG(INFO) << '\n';
            //select occurences greedily:
            int last =  starting_positions.front();
            int current;
            for (std::vector<int>::iterator it=starting_positions.begin()+1; it!=starting_positions.end(); ++it){

                current = *it;
                if(last+top.first>current){
                    it = starting_positions.erase(it);
                }
                last = current;
            }
            //DLOG(INFO) <<"selected starting positions: " << std::endl;
           // for (std::vector<int>::iterator it=starting_positions.begin(); it!=starting_positions.end(); ++it)
          //      DLOG(INFO) << ' ' << *it;
           // DLOG(INFO) << '\n';

            //now ceck in bitvector viable starting positions
            // there is no 1 bit on the corresponding positions

            std::vector<int>::iterator it=starting_positions.begin();

            while(it!=starting_positions.end()){
                bool non_viable = false;
                //DLOG(INFO) << "check position: " << *it << std::endl;
                for(int k = 0; k<top.first; k++){
                    if(non_terminals[*it+k]==1){

                        non_viable=true;
                        break;
                    }
                }
                if(non_viable){
                   // DLOG(INFO) << "position not viable: " << *it << std::endl;
                    //it =
                    it = starting_positions.erase(it);
                } else {
                    it++;
                }
                //DLOG(INFO) << "position viable: " << *it << std::endl;
            }
            //if the factor is still repeating, make the corresponding positions unviable


            if(starting_positions.size()>=2){
                //DLOG(INFO) << "These starting positions are still viable." <<  std::endl;
                //DLOG(INFO) << "non-terminals before:" << std::endl << non_terminals << std::endl;

                for (std::vector<int>::iterator it=starting_positions.begin(); it!=starting_positions.end(); ++it){

                   // DLOG(INFO) << "position removed: " << *it << " to " << *it+top.first << std::endl;
                    for(int k = 0; k<top.first; k++){
                        //DLOG(INFO) << "position non viable: " << *it+k << std::endl;
                        non_terminals[*it+k]=1;
                    }
                    int length_of_symbol = top.first;
                    std::tuple<int,int,int> symbol(*it, non_terminal_symbol_number, length_of_symbol);
                    DLOG(INFO) << "added symbol: " << non_terminal_symbol_number << " at pos " << *it;
                    non_terminal_symbols.push(symbol);
                }
                dictionary.push_back(substring);
                DLOG(INFO) << "added rule:";
                DLOG(INFO) << (char) (64+non_terminal_symbol_number) << " -> " << substring;
                non_terminal_symbol_number++;
                //DLOG(INFO) << "non-terminals after:" << std::endl << non_terminals << std::endl;

            }




        }
        //DLOG(INFO) << std::endl;

        //while(!non_terminal_symbols.empty()){
         //   std::tuple<int,int,int> position = non_terminal_symbols.top();
          //  DLOG(INFO) << "at pos: " << std::get<0>(position) << " symbol: " << std::get<1>(position)  << " length: "<< std::get<2>(position)  <<  std::endl;
           // non_terminal_symbols.pop();
       // }



        std::string output_string ="";

        // replace
        int pos = 0;
        int start_position;
        int symbol_number ;
        int symbol_length;
        while(!non_terminal_symbols.empty()){
            std::tuple<int,int,int> next_position = non_terminal_symbols.top();
            start_position = std::get<0>(next_position);
            symbol_number =std::get<1>(next_position);
            symbol_length = std::get<2>(next_position);
            non_terminal_symbols.pop();

            while(pos< start_position){
                //get original text, because no symbol...
                output_string+=t[pos];
                pos++;
            }

            //write symbol number
             output_string += '\\';
            output_string += (char) (64+symbol_number);
            pos+= symbol_length;

        }
        //start_position=0;
        //if no more terminals, write rest of text
        while( pos<t.size()){
            output_string+=t[pos++];
        }

        DLOG(INFO) << "output string: ";
        DLOG(INFO) << output_string ;
        ostream << output_string;
        ostream << "\\$";

        DLOG(INFO) << "dictionary: ";


        auto it = dictionary.begin();

        int rule =0;
        while(it != dictionary.end()){

            DLOG(INFO) <<  (char) (65+rule++)<< " -> " << *it;
            ostream << *it;
            ostream << "\\$";
            it++;
        }



        //encode string
    }

    inline virtual void decompress(Input& input, Output& output) override {
        std::vector<std::pair<int,int>> symbol_positions;// = new std::vector<std::pair<int,int>>;
        //auto istream = input.as_stream();
        auto in = input.as_stream();
        std::string text;
        char cur_char;
        bool escape=false;
        bool dict = false;
        char escape_symbol = '\\';
        char dollar_symbol = '$';
        std::string symbol;
        std::vector<std::string> symbol_list;// = new std::vector<std::string>;
        while(in.get(cur_char)){
            if(dict){
                if(cur_char != escape_symbol ){
                    symbol += cur_char;
                } else {
                    //symbol ends...

                    symbol_list.push_back(symbol);
                     DLOG(INFO) << "symobl: " << symbol << " symbol number: " << symbol_list.size();
                    symbol="";
                    in.get(cur_char);
                }
            } else
            if(!escape){
                escape=false;
                if(cur_char != escape_symbol ){
                    text += cur_char;
                } else {
                    escape = true;
                }
            } else {
                escape=false;
                if(cur_char == dollar_symbol){
                    DLOG(INFO) << text;
                    dict=true;
                } else {
                    std::pair<int,int> pair (text.length(), cur_char-64);
                    symbol_positions.push_back(pair);
                    DLOG(INFO) << "non terminal symbol at: " << text.length() << " symbol: " << cur_char-64;
                }

            }

        }
        int added_symbols=0;
        auto it = symbol_positions.begin();
        while(it!= symbol_positions.end()){
            std::pair<int,int> position = *it;
            std::string symbol = symbol_list[position.second-1];
            text = text.substr(0, position.first + added_symbols) + symbol + text.substr(position.first + added_symbols , text.size());
            DLOG(INFO) << text;
            it++;

        }

        auto ostream = output.as_stream();
        ostream << text;
    }

};

}


#endif
