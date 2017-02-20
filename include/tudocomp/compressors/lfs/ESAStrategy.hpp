#pragma once



#include <tudocomp/Compressor.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/util.hpp>
#include <vector>
#include <tuple>

#include <tudocomp/io.hpp>


#include <tudocomp/io/BitIStream.hpp>
#include <tudocomp/io/BitOStream.hpp>


#include <tudocomp/ds/IntVector.hpp>





#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/Algorithm.hpp>

#include <tudocomp/Literal.hpp>


//#include <tudocomp/tudocomp.hpp>


namespace tdc {

template<typename text_t = TextDS<> , uint min_lrf = 2>
class ESAStrategy : public Algorithm {
private:

    //(position in text, non_terminal_symbol_number, length_of_symbol);
    //typedef std::tuple<uint,uint,uint> non_term;
  //  typedef std::vector<non_term> non_terminal_symbols;

   // typedef std::vector<std::pair<uint,uint>> rules;

    //(position in text, non_terminal_symbol_number, length_of_symbol);
    typedef std::tuple<uint,uint,uint> non_term;
    typedef std::vector<non_term> non_terminal_symbols;

    typedef std::vector<std::pair<uint,uint>> rules;

    //uint min_lrf;

    BitVector dead_positions;

    //typedef TextDS<> text_t;




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

    using Algorithm::Algorithm; //import constructor

    inline static Meta meta() {
        Meta m("lfs_comp_esas", "esa_strat");

        m.option("textds").templated<text_t, TextDS<>>();
        return m;
    }


    inline void compute_rules(const io::InputView & input, rules & dictionary, non_terminal_symbols & nts_symbols){

//env().env_for_option("textds"),
        text_t t(env().env_for_option("textds"),input);
       // text_t t(env, in);
        DLOG(INFO) << "building sa and lcp";
        t.require(text_t::SA | text_t::ISA | text_t::LCP);
        auto& sa_t = t.require_sa();
        auto& lcp_t = t.require_lcp();

        //min_lrf=2;

        // iterate over lcp array, add indexes with non overlapping prefix length greater than min_lrf_length to vector
        std::vector<std::pair<uint,uint>> lrf_occurences;


        DLOG(INFO) << "iterate over lcp";
        uint dif ;
        uint factor_length;
        for(uint i = 1; i<lcp_t.size(); i++){

            if(lcp_t[i] >= min_lrf){
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
                if(factor_length>=min_lrf){
                    lrf_occurences.push_back(pair);
                }
            }
        }

        // Pop PQ, Select occurences of suffix, check if contains replaced symbols
        dead_positions = BitVector(input.size(), 0);
        //Pq for the non-terminal symbols
        //the first in pair is position, the seconds the number of the non terminal symbol

        DLOG(INFO) << "computing lrfs";

        //vector of text position, length
        //std::vector<std::pair<uint,uint>> dictionary;

        std::sort(lrf_occurences.begin(),lrf_occurences.end());

        //std::vector<std::tuple<uint,uint,uint>> nts_symbols;
        nts_symbols.reserve(lrf_occurences.size());
        uint non_terminal_symbol_number = 0;
        while(!lrf_occurences.empty()){
            std::pair<uint,uint> top = lrf_occurences.back();
            lrf_occurences.pop_back();

            if(dead_positions[sa_t[top.second]] == 1 || dead_positions[sa_t[top.second-1]] == 1 || dead_positions[sa_t[top.second]+top.first-1] == 1 || dead_positions[sa_t[top.second-1]+top.first-1] == 1){
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
                if(dead_positions[sa_t[i-1]] == 0 && dead_positions[sa_t[i-1]+top.first-1] == 0){
                    starting_positions.push_back(sa_t[i-1]);
                }
                i--;
            }
            i = top.second+1;
            while(i< lcp_t.size() &&  lcp_t[i]>=top.first){
                if(dead_positions[sa_t[i]] == 0 && dead_positions[sa_t[i]+top.first-1] == 0){
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
                            dead_positions[*it+k]=1;
                        }

                        uint length_of_symbol = top.first;
                        std::tuple<uint,uint,uint> symbol(*it, non_terminal_symbol_number, length_of_symbol);
                        nts_symbols.push_back(symbol);
                    }
                    dictionary.push_back(longest_repeating_factor);
                    non_terminal_symbol_number++;
                }
            }
        }
        DLOG(INFO) << "sorting occurences";
        //, std::greater<std::tuple<uint,uint,uint>>()
        std::sort(nts_symbols.begin(), nts_symbols.end());
        DLOG(INFO)<<"dict size: "<<dictionary.size();
        DLOG(INFO)<<"symbols:"<<nts_symbols.size();

    }
};
}

