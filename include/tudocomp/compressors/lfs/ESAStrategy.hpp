#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/Algorithm.hpp>

#include <vector>
#include <tuple>

namespace tdc {
namespace lfs {

template<typename text_t = TextDS<> , uint min_lrf = 2>
class ESAStrategy : public Algorithm {
private:

    //(position in text, non_terminal_symbol_number, length_of_symbol);
    typedef std::tuple<uint,uint,uint> non_term;
    typedef std::vector<non_term> non_terminal_symbols;
    typedef std::vector<std::pair<uint,uint>> rules;

    BitVector dead_positions;

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



    //could be node_type
    std::vector<std::vector<uint> > lcp_bins;
public:

    using Algorithm::Algorithm; //import constructor

    inline static Meta meta() {
        Meta m("lfs_comp", "esa");

        m.option("textds").templated<text_t, TextDS<>>("textds");

        m.uses_textds<text_t>(text_t::SA | text_t::ISA | text_t::LCP);
        return m;
    }


    inline void compute_rules(io::InputView & input, rules & dictionary, non_terminal_symbols & nts_symbols){

//env().env_for_option("textds"),
        //const auto input = in.as_view();


        text_t t(env().env_for_option("textds"), input);
       // text_t t(env, in);
        DLOG(INFO) << "building sa and lcp";
        StatPhase::wrap("computing sa and lcp", [&]{

            t.require(text_t::SA | text_t::ISA | text_t::LCP);
        });
        auto& sa_t = t.require_sa();
        auto& lcp_t = t.require_lcp();

        //min_lrf=2;
        StatPhase::wrap("computing lrf occurences", [&]{

        // iterate over lcp array, add indexes with non overlapping prefix length greater than min_lrf_length to vector
        //std::vector<std::pair<uint,uint>> lrf_occurences;

        StatPhase::wrap("computing lrf occs", [&]{

        DLOG(INFO) << "iterate over lcp";
        uint dif ;
        uint factor_length;

        uint max = 0;

        for(uint i = 1; i<lcp_t.size(); i++){


            if(lcp_t[i] >= min_lrf){
                if(max < lcp_t[i]){
                    max = lcp_t[i];
                    lcp_bins.resize(max+1);
                }
                //compute length of non-overlapping factor:

                dif = std::abs((long)(sa_t[i-1] - sa_t[i]));
                factor_length = lcp_t[i];
                factor_length = std::min(factor_length, dif);

                //maybe greater non-overlapping factor possible with smaller suffix?
                int j =i-1;
                uint alt_dif;
                while(j>0 && lcp_t[j]>factor_length ){
                    alt_dif = abs((long)(sa_t[j] - sa_t[i]));
                    if(alt_dif>dif){
                        dif = alt_dif;
                    }
                    j--;


                }
                factor_length = lcp_t[i];
                factor_length = std::min(factor_length, dif);
                //second is position in suffix array
                //first is length of common prefix
               // std::pair<uint,uint> pair (factor_length, i);
              //  if(factor_length>=min_lrf){
               //     lrf_occurences.push_back(pair);
               // }

                lcp_bins[factor_length].push_back(i);
            }
        }

        DLOG(INFO)<<"max lcp: "<<max;

        });


        //Pq for the non-terminal symbols
        //the first in pair is position, the seconds the number of the non terminal symbol

        DLOG(INFO) << "computing lrfs";

        //vector of text position, length
        //std::vector<std::pair<uint,uint>> dictionary;
      //  StatPhase::wrap("sorting lrf occs", [&]{

      //  std::sort(lrf_occurences.begin(),lrf_occurences.end());
     //   });

        StatPhase::wrap("selecting occs", [&]{

            // Pop PQ, Select occurences of suffix, check if contains replaced symbols
        dead_positions = BitVector(t.size(), 0);

        //std::vector<std::tuple<uint,uint,uint>> nts_symbols;
        nts_symbols.reserve(lcp_bins.size());
        uint non_terminal_symbol_number = 0;

        for(uint lcp_len = lcp_bins.size()-1; lcp_len>= min_lrf; lcp_len--){
            //DLOG(INFO)<<"size of lrf: "  << lcp_len <<" occs: "<< lcp_bins[lcp_len].size();
            for(auto bin_it = lcp_bins[lcp_len].begin(); bin_it!=lcp_bins[lcp_len].end(); bin_it++){

               // if(dead_positions[sa_t[*bin_it]] || dead_positions[sa_t[*bin_it-1]] || dead_positions[sa_t[*bin_it]+lcp_len-1] || dead_positions[sa_t[*bin_it-1]+lcp_len-1]){
               //     continue;
               // }

                //detect all starting positions of this string using the sa and lcp:
                std::vector<uint> starting_positions;
                starting_positions.reserve(20);

                //starting_positions.push_back(sa_t[*bin_it]);


                // and ceck in bitvector viable starting positions
                // there is no 1 bit on the corresponding positions
                // it suffices to check start and end position, because lrf can only be same length and shorter
                uint i = *bin_it;

                uint shorter_dif = lcp_len;

             //   uint left_bound = 0;
             //   uint right_bound = 0;

                while(i>=0 && ( lcp_t[i])>=lcp_len){
                    if(!dead_positions[sa_t[i-1]]  && !dead_positions[sa_t[i-1]+lcp_len-1]){
                        starting_positions.push_back(sa_t[i-1]);
                    }

                    if(!dead_positions[sa_t[i-1]] && dead_positions[sa_t[i-1]+lcp_len-1] ){
                        //DLOG(INFO)<<"shorter lrf poss";
                        while(!dead_positions[sa_t[i-1]+lcp_len-shorter_dif]){
                            shorter_dif--;
                        }
                    }
                    i--;

//                    left_bound = i;

                }
                i = *bin_it;
                while(i< lcp_t.size() &&  lcp_t[i]>=lcp_len){

                    if(!dead_positions[sa_t[i]]  && !dead_positions[sa_t[i]+lcp_len-1]){
                        starting_positions.push_back(sa_t[i]);
                    }
                    if(!dead_positions[sa_t[i]]   && dead_positions[sa_t[i]+lcp_len-1] ){
                        //DLOG(INFO)<<"shorter lrf poss";
                        while(!dead_positions[sa_t[i]+lcp_len-shorter_dif]){
                            shorter_dif--;
                        }
                    }
             //       right_bound=i;
                    i++;
                }

               // DLOG(INFO) << sa_t[left_bound] << "  "<< sa_t[right_bound];
              //  std::sort(sa_t.begin() + left_bound, sa_t.begin() +right_bound);


                //DLOG(INFO) << sa_t[left_bound] << "  "<< sa_t[right_bound];
               // DLOG(INFO)<<"lrf_len: "<<lcp_len<< " shorter dif: "<< shorter_dif;
                //if the factor is still repeating, make the corresponding positions unviable
                if(lcp_len-shorter_dif>=min_lrf){
                    //commented because extremly slow
                    //lcp_bins[lcp_len-shorter_dif].push_back(*bin_it);
                }

                if(starting_positions.size()>=2){
                    std::vector<uint> selected_starting_positions = select_starting_positions(starting_positions, lcp_len);
                    //computing substring to be replaced

                    if(selected_starting_positions.size()>=2){



                        uint offset = sa_t[*bin_it];
                        std::pair<uint,uint> longest_repeating_factor(offset, lcp_len);
                        for (std::vector<uint>::iterator it=selected_starting_positions.begin(); it!=selected_starting_positions.end(); ++it){
                            for(uint k = 0; k<lcp_len; k++){
                                dead_positions[*it+k]=1;
                            }

                            uint length_of_symbol = lcp_len;
                            std::tuple<uint,uint,uint> symbol(*it, non_terminal_symbol_number, length_of_symbol);
                            nts_symbols.push_back(symbol);
                        }

                        dictionary.push_back(longest_repeating_factor);
                        non_terminal_symbol_number++;
                    }
                }

            }
        }
        });
        });

        StatPhase::wrap("sorting symbols", [&]{
        DLOG(INFO) << "sorting symbols";
        //, std::greater<std::tuple<uint,uint,uint>>()
        std::sort(nts_symbols.begin(), nts_symbols.end());

        });

    }
};
}
}
