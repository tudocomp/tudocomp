/*
    This will be a basic implementation of the LZ77 compressor
*/

#ifndef _INCLUDED_LZ77_TEST_COMPRESSOR_HPP
#define _INCLUDED_LZ77_TEST_COMPRESSOR_HPP

#include <algorithm>
#include <vector>

#include <sdsl/int_vector.hpp>

#include <tudocomp/util.h>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/io.h>
#include <tudocomp/lzss/LZSSCoderOpts.hpp>

namespace tudocomp {
namespace lzss {

// const std::string WINDOW_OPTION = "lzss.window";

/// Computes the LZ77 factorization of the input
class LZ77TestCompressor : public Compressor {

private:
    // size_t m_window;

public:
    /// Default constructor (not supported).
    inline LZ77TestCompressor() = delete;

    /// Construct the class with an environment.
    inline LZ77TestCompressor(Env& env) : Compressor(env) {
        // m_window = env.option_as<size_t>(WINDOW_OPTION, 16);
    }

    /// Compress the input into an output
    virtual void compress(Input& input, Output& output) override final {
        // char c, nextChar='\0';
        char c;

        auto in_guard = input.as_stream();
        std::istream& in = *in_guard;
        auto out_guard = output.as_stream();
        std::ostream& out = *out_guard;

        std::stringstream input_str;
        size_t curr_pos=0, text_length, pos, len;

        // Here we grab the input text and store it in input_text
        while(in.get(c) && c != '\0') {
            input_str << c;
        }
        std::string input_text;
        input_text = input_str.str();

        // Here we store the size of the compressed string
        text_length = input_text.length();
        out << text_length << ':';

        // Here we convert the string into compressed factors
        while(curr_pos < text_length) {
            pos = 0;
            len = 0;
            size_t iter_pos = 1;

            // Here we check for the longest match in the string             
            while(curr_pos >= iter_pos) {
                bool searchFlag = false;
                size_t iter_len = 0, i = 0;

                // This loop looks for the longest match starting at the given iteration position
                while(!searchFlag) {

                    // if (curr_pos+i-iter_pos >= text_length) {
                    //     DLOG(INFO) << curr_pos;
                    //     DLOG(INFO) << i;
                    //     DLOG(INFO) << iter_pos;
                    // }

                    // assert(curr_pos+i < text_length);
                    // assert(curr_pos+i-iter_pos < text_length);

                    // The length is increased and we move along as we match characters
                    if(input_text[curr_pos+i] == input_text[curr_pos+i-iter_pos]) {
                        iter_len++;
                        i++;

                        // If the next character would be beyond the string the matching sequence ends
                        if(curr_pos+i >= text_length) {
                            searchFlag = true;
                        }
                    }
                    // Here there was no match found or the matching sequence ended
                    else {
                        searchFlag = true;
                    } 
                }

                // We store the longest length matched sequence
                if(len < iter_len) {
                    len = iter_len;
                    pos = iter_pos;
                }

                iter_pos++;
            }

            // If the length is 0, we have a symbol
            if (len == 0) {
                pos = 0;
            } else {
                // pos++;
                curr_pos += len;
            }

            // Here we output the current factor
            out << "(" << pos << "," << len << ",";

            // The $ character indicates the end of the string
            if(curr_pos < text_length) {
                out << input_text[curr_pos] << ")";
                curr_pos++;
            } else {
                out << "$" << ")";
            }
        }
    }

    /// Decompress the input into an output
    virtual void decompress(Input& input, Output& output) override {
        char c, nextChar='\0';

        auto in_guard = input.as_stream();
        std::istream& in = *in_guard;
        auto out_guard = output.as_stream();
        std::ostream& out = *out_guard;

        std::stringstream total_len_str;
        size_t curr_pos=0, total_len;

        // Here we get the length of the original string from the compressed string
        // and use it to declare a vector of the correct size to hold the result
        while(in.get(c) && c != ':') {
            total_len_str << c;
        }
        total_len_str >> total_len;
        sdsl::int_vector<8> text = sdsl::int_vector<8>(total_len, 0);

        // Here we start moving through each of the factors in the compressed string
        while(in.get(c) && nextChar != '$') {
            size_t pos, len;
            std::stringstream pos_str, len_str;

            while(in.get(c) && c != ',') {
                pos_str << c;
            }
            while(in.get(c) && c != ',') {
                len_str << c;
            }
            pos_str >> pos;
            len_str >> len;

            // Factors which are not symbols get decompressed
            if(len != 0) {
                while(len != 0) {
                    text[curr_pos] = text[curr_pos-pos];
                    curr_pos++;
                    len--;
                }
            } 

            // The next character is added to the text, unless we have reached the end
            in.get(c);
            nextChar = c;
            if(nextChar != '$') {
                text[curr_pos] = uint8_t(c);
                curr_pos++;                
            }
            in.get(c);

        }

        // Here we write the output of the decompressed string
        out.write((const char*)text.data(), text.size());

    }

protected:


};

}}

#endif
