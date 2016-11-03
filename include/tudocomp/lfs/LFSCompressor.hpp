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


        //sdsl::lcp_wt<> lcp_array;
        //sdsl::csa_wt<> suffix_array;

        std::string file = "test_files/lfs_test.txt";
        std::string text;
        char cur_char;
        while(istream.get(cur_char)){
            text += cur_char;
        }

        //text = "abaaabbababb$";
        //input.as_stream;

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
        while(!pq.empty()){

            std::pair<int,int> top = pq.top();
            std::cout << "pos1: "<< csa[top.second] << "pos2: "<< csa[top.second-1] << " len: " << top.first << "; ";
            std::cout << std::endl;
            pq.pop();

            std::string substring = text.substr(csa[top.second-1], top.first);
            std::cout << substring << std::endl;
        }
        std::cout << std::endl;


        // Pop PQ, Select occurences of suffix, check if contains replaced symbols
        // replace

        //encode string
    }

    inline virtual void decompress(Input& input, Output& output) override {
    }

};

}


#endif
