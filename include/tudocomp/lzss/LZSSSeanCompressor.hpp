/*
    This will be a basic implementation of the LZ77 compressor
*/

#ifndef _INCLUDED_LZSS_SEAN_COMPRESSOR_HPP
#define _INCLUDED_LZSS_SEAN_COMPRESSOR_HPP

#ifndef SLIDING_WINDOW_SIZE
#define SLIDING_WINDOW_SIZE 1000
#endif

#include <sdsl/int_vector.hpp>

#include <tudocomp/util.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/io.hpp>
#include <tudocomp/io/BitOStream.hpp>
#include <tudocomp/io/BitIStream.hpp>

namespace tdc {
namespace lzss {

// const std::string WINDOW_OPTION = "lzss.window";

/// Computes the LZ77 factorization of the input
class LZSSSeanCompressor : public Compressor {

public:
    inline static Meta meta() {
        Meta m("compressor", "lz77ss_sean", "Sean's LZ77 compressor");
        return m;
    }

private:
    // size_t m_window;

public:
    /// Default constructor (not supported).
    inline LZSSSeanCompressor() = delete;

    /// Construct the class with an environment.
    inline LZSSSeanCompressor(Env&& env) : Compressor(std::move(env)) {
        // m_window = env.option_as<size_t>(WINDOW_OPTION, 16);
    }

    /// Compress the input into an output
    virtual void compress(Input& input, Output& output) override final {
        char c;

        auto in = input.as_stream();
        auto out_guard = output.as_stream();
        BitOStream* bito = new BitOStream(out_guard);

        std::stringstream input_str;
        size_t curr_pos=0, text_length, pos, len;

        // Here we grab the input text and store it in input_text
        while(in.get(c) && c != '\0') {
            input_str << c;
        }
        std::string input_text;
        input_text = input_str.str();

        // Here we store the size of the compressed string
        text_length = (input_text.length());
        bito->write_compressed_int(text_length);

        // Here we convert the string into compressed factors
        while(curr_pos < text_length) {
            pos = 0;
            len = 0;
            size_t iter_pos = 1;

            // Here we check for the longest match in the string
            while(curr_pos >= iter_pos && iter_pos <= SLIDING_WINDOW_SIZE) {
            // while(curr_pos >= iter_pos) {
                bool searchFlag = false;
                size_t iter_len = 0, i = 0;

                // This loop looks for the longest match starting at the given iteration position
                while(!searchFlag) {

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
                if(len < iter_len && iter_len > 1) {
                    len = iter_len;
                    pos = iter_pos;
                }

                iter_pos++;
            }

            // Here we output a symbol, which is initialized with a 0 bit
            if(len == 0) {
                bito->write_bit(0);
                // bito->flush();
                // bito->write_aligned_bytes(&input_text[curr_pos],1);
                bito->write_int(input_text[curr_pos], 8);
                curr_pos++;
            }
            // Here we output a factor, which is initialized with a 1 bit
            else {
                curr_pos += len;
                bito->write_bit(1);
                // bito->flush();
                bito->write_compressed_int(pos);
                bito->write_compressed_int(len);
            }
        }

        // Flush out the last few bits
        bito->flush();
    }

    /// Decompress the input into an output
    virtual void decompress(Input& input, Output& output) override {
        // char c;
        bool factor = false;

        auto in_guard = input.as_stream();
        auto out = output.as_stream();
        BitIStream* biti = new BitIStream(in_guard);

        std::stringstream total_len_str;
        size_t curr_pos=0, total_len=0;

        // Here we get the length of the original string from the compressed string
        // and use it to declare a vector of the correct size to hold the result
        total_len = biti->read_compressed_int<size_t>();
        sdsl::int_vector<8> text = sdsl::int_vector<8>(total_len, 0);

        // Here we start moving through each of the factors in the compressed string
        while(total_len > 0) {
            size_t pos, len;
            factor = biti->read_bit();
            // biti->readBits<char>(7);
            if(!factor) {
;               text[curr_pos] = biti->read_int<char>(8);
                curr_pos++;
                total_len--;
            }
            // Here we are decompressing a factor
            else {
                pos = biti->read_compressed_int<size_t>();
                len = biti->read_compressed_int<size_t>();
                while(len != 0) {
                    text[curr_pos] = text[curr_pos-pos];
                    curr_pos++;
                    total_len--;
                    len--;
                }
            }
        }

        // Here we write the output of the decompressed string
        out.write((const char*)text.data(), text.size());

    }

protected:


};

}}

#endif
