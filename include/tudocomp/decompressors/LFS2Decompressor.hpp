#pragma once

//std includes:
#include <tuple>
#include <unordered_map>
#include <vector>

//general includes:
#include <tudocomp/util.hpp>
#include <tudocomp/Decompressor.hpp>

//includes encoding:
#include <tudocomp/io/BitIStream.hpp>

#include <tudocomp/coders/EliasGammaCoder.hpp>
#include <tudocomp/coders/HuffmanCoder.hpp>

namespace tdc {
namespace lfs {

template<
    typename literal_coder_t = HuffmanCoder,
    typename len_coder_t = EliasGammaCoder
>
class LFS2Decompressor : public Decompressor {
public:
    inline static Meta meta() {
        Meta m(Decompressor::type_desc(), "lfs2", "lfs2 decompressor");
        m.param("lfs2_lit_coder").strategy<literal_coder_t>(
            Coder::type_desc(), Meta::Default<HuffmanCoder>());
        m.param("lfs2_len_coder").strategy<len_coder_t>(
            Coder::type_desc(), Meta::Default<EliasGammaCoder>());
        return m;
    }

    using Decompressor::Decompressor;

    inline virtual void decompress(Input& input, Output& output) override {
        DLOG(INFO) << "decompress lfs";
        std::shared_ptr<BitIStream> bitin = std::make_shared<BitIStream>(input);

        typename literal_coder_t::Decoder lit_decoder(
            config().sub_config("lfs2_lit_coder"),
            bitin
        );
        typename len_coder_t::Decoder len_decoder(
            config().sub_config("lfs2_len_coder"),
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

        DLOG(INFO)<<"decoded number of nts: "<< dict_lengths.size();

        std::vector<std::string> dictionary;
        uint dictionary_size = dict_lengths.size();

        Range dictionary_r (0, dictionary_size);

        dictionary.resize(dict_lengths.size());

        std::stringstream ss;
        uint symbol_number;
        char c1;

        DLOG(INFO) << "reading dictionary";
        for(long i = dict_lengths.size() -1; i>=0 ;i--){
            ss.str("");
            ss.clear();
            long size_cur = (long) dict_lengths[i];
            while(size_cur > 0){
                bool bit1 = lit_decoder.template decode<bool>(bit_r);
                if(bit1){
                    //bit = 1, is nts, decode nts num and copy
                    symbol_number = lit_decoder.template decode<uint>(dictionary_r); // Dekodiere Literal

                    symbol_number-=1;

                    if(symbol_number < dictionary.size()){

                        ss << dictionary.at(symbol_number);
                        size_cur-= dict_lengths[symbol_number];
                    } else {
                        break;
                    }
                } else {
                    //bit = 0, decode literal
                    c1 = lit_decoder.template decode<char>(literal_r); // Dekodiere Literal
                    size_cur--;

                    ss << c1;
                }
            }

            dictionary[i]=ss.str();
        }

        auto ostream = output.as_stream();
        //reading start symbol:
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
                symbol_number-=1;

                if(symbol_number < dictionary.size()){

                    ostream << dictionary.at(symbol_number);
                } else {
                    DLOG(INFO)<< "too large symbol: " << symbol_number;
                }

            }
        }
    }
};


}} //ns

