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
        std::cout << "output"<< std::endl;
    }*/

    inline LFSCompressor(Env&& env):
        Compressor(std::move(env))
    {
        // ...

        std::cout << "Compressor instantiated"<< std::endl;
        //env.begin_stat_phase("LFS compression");

    }
    inline virtual void compress(Input& input, Output& output) override {
        //env().begin_stat_phase("LFS compression");
       // env().log_stat("A statistic", 147);
        //build lcp array and suffix array:
        auto istream = input.as_stream();
       // auto ostream = output.as_stream();




        //alternative method of creating lcp and sa
         auto in = input.as_view();
        TextDS<> t(in);
      //  env.begin_stat_phase("Construct text ds");
        t.require(TextDS<>::SA | TextDS<>::LCP);
       // env.end_stat_phase();


        t.print(std::cout);

        auto& sa_t = t.require_sa();
        auto& lcp_t = t.require_lcp();
        //t.sa[0];
        std::cout << sa_t[0] << std::endl;

        std::string file = "test_files/lfs_test.txt";
        std::string text;
        char cur_char;
        while(istream.get(cur_char)){
            text += cur_char;
        }

        //text = "abaaabbababb$";
        //input.as_stream;

        //just necessary for construct()
        std::ofstream myfile;
        myfile.open (file);
        myfile << text;
        myfile.close();



        sdsl::cache_config cc(false); // do not delete temp files after csa construction
        sdsl::csa_wt<> csa;
        sdsl::construct(csa, file, 1);

        cc.delete_files = true; // delete temp files after lcp construction
        sdsl::lcp_wt<> lcp;
        sdsl::construct(lcp, file, 1);

        std::cout << "Text:" << std::endl << text << std::endl  << std::endl;

        std::cout << "suffix array:" << std::endl;
        std::cout << csa << std::endl;
        std::cout << "-------" << std::endl;
        std::cout << "lcp array:" << std::endl;
        std::cout << lcp << std::endl;

        //std::cout << lcp << std::endl;
        // iterate over lcp array, add indexes with non overlapping prefix length grater than 2 to pq
        std::priority_queue<std::pair<int,int>> pq;

        int dif ;
        int factor_length;
        for(uint i = 1; i<lcp.size(); i++){

            //std::cout <<  lcp[i] << " ";
            if(lcp[i] >= 2){
                //compute length of non-overlapping factor:

                if(csa[i-1] > csa[i]){
                    dif =  csa[i-1] - csa[i];
                } else {
                    dif =  csa[i] - csa[i-1];
                }
                factor_length = lcp[i];


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
        std::cout << std::endl;
        std::cout << "repeating factors:" << std::endl;
        // Pop PQ, Select occurences of suffix, check if contains replaced symbols
        sdsl::bit_vector non_terminals(csa.size(), 0);
        //Pq for the non-terminal symbols
        //the first in pair is position, the seconds the number of the non terminal symbol
        std::priority_queue<std::pair<int,int>> non_terminal_symbols;
        int non_terminal_symbol_number = 1;
        while(!pq.empty()){

            std::pair<int,int> top = pq.top();
            std::cout << "pos1: "<< csa[top.second] << " pos2: "<< csa[top.second-1] << " len: " << top.first << "; ";
            std::cout << std::endl;
            pq.pop();

            std::string substring = text.substr(csa[top.second-1], top.first);
            std::cout << "string: " << substring << std::endl;

            //detect all starting positions of this string using the sa and lcp:
            std::vector<int> starting_positions;

            starting_positions.push_back(csa[top.second]);
            std::cout <<"starting positions: " << std::endl;
            int i = top.second;
            while(i>=0 && lcp[i]>=top.first){
                starting_positions.push_back(csa[i-1]);
                //std::cout << csa[i-1] << " ";
                i--;
            }
            i = top.second+1;
            while(i< lcp.size() && lcp[i]>=top.first){
                starting_positions.push_back(csa[i]);
                //std::cout << csa[i];
                i++;
            }
            std::sort(starting_positions.begin(), starting_positions.end());
            for (std::vector<int>::iterator it=starting_positions.begin(); it!=starting_positions.end(); ++it)
                std::cout << ' ' << *it;
            std::cout << '\n';
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
            std::cout <<"selected starting positions: " << std::endl;
            for (std::vector<int>::iterator it=starting_positions.begin(); it!=starting_positions.end(); ++it)
                std::cout << ' ' << *it;
            std::cout << '\n';

            //now ceck in bitvector viable starting positions
            // there is no 1 bit on the corresponding positions

            std::vector<int>::iterator it=starting_positions.begin();

            while(it!=starting_positions.end()){
                bool non_viable = false;
                std::cout << "check position: " << *it << std::endl;
                for(int k = 0; k<top.first; k++){
                    if(non_terminals[*it+k]==1){

                        non_viable=true;
                        break;
                    }
                }
                if(non_viable){
                    std::cout << "position not viable: " << *it << std::endl;
                    //it =
                    it = starting_positions.erase(it);
                } else {
                    it++;
                }
                //std::cout << "position viable: " << *it << std::endl;
            }
            //if the factor is still repeating, make the corresponding positions unviable


            if(starting_positions.size()>=2){
                std::cout << "These starting positions are still viable." <<  std::endl;
                std::cout << "non-terminals before:" << std::endl << non_terminals << std::endl;

                for (std::vector<int>::iterator it=starting_positions.begin(); it!=starting_positions.end(); ++it){

                    std::cout << "position removed: " << *it << " to " << *it+top.first << std::endl;
                    for(int k = 0; k<top.first; k++){
                        //std::cout << "position non viable: " << *it+k << std::endl;
                        non_terminals[*it+k]=1;
                    }
                }
                std::pair<int,int> symbol(*it, non_terminal_symbol_number);
                non_terminal_symbols.push(symbol);
                std::cout << "non-terminals after:" << std::endl << non_terminals << std::endl;
            }




        }
        std::cout << std::endl;

        while(!non_terminal_symbols.empty()){
            std::pair<int,int> position = non_terminal_symbols.top();
            std::cout << "at pos: " << position.first << " symbol: " << position.second << std::endl;
            non_terminal_symbols.pop();
        }



        // replace

        //encode string
    }

    inline virtual void decompress(Input& input, Output& output) override {
    }

};

}


#endif
