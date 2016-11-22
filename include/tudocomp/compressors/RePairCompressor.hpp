#ifndef _INCLUDED_RE_PAIR_COMPRESSOR_HPP_
#define _INCLUDED_RE_PAIR_COMPRESSOR_HPP_

#include <sdsl/int_vector.hpp>

#include <tudocomp/Compressor.hpp>

#include <tudocomp/Range.hpp>
#include <tudocomp/coders/BitOptimalCoder.hpp> //default

#include <tudocomp/util/Counter.hpp>

namespace tdc {

template <typename coder_t>
class RePairCompressor : public Compressor {
private:
    typedef uint32_t sym_t;
    typedef uint64_t digram_t;
    static const size_t digram_shift = 32UL;

    inline static digram_t digram(sym_t l, sym_t r) {
        return (digram_t(l) << digram_shift) | digram_t(r);
    }

    inline static sym_t left(digram_t di) {
        return sym_t(di >> digram_shift);
    }

    inline static sym_t right(digram_t di) {
        return sym_t(di);
    }

public:
    inline static Meta meta() {
        Meta m("compressor", "repair", "Re-Pair compression");
        m.option("coder").templated<coder_t, BitOptimalCoder>();
        return m;
    }

    inline RePairCompressor(Env&& env) : Compressor(std::move(env)) {}

    virtual void compress(Input& input, Output& output) override {
        const sym_t sigma = 256; //TODO

        // prepare editable text
        size_t n;
        sym_t *text;

        {
            auto view = input.as_view();
            n = view.size();
            text = new sym_t[n];

            for(size_t i = 0; i < view.size(); i++) {
                text[i] = view[i];
            }
        }

        // compute RePair grammar
        sdsl::bit_vector b(n, 1); // if b[i] = 0, there is no more symbol at i
        std::vector<digram_t> grammar;

        do {
            // count digrams
            digram_t max;
            size_t max_count = 0;

            {
                std::map<digram_t, size_t> count; // TODO: anything better?

                size_t i = 0;
                while(i < n - 1) {
                    size_t j = i + 1;
                    while(j < n && !b[j]) ++j; // find next unreplaced symbol
                    if(j >= n) break; // break if at end

                    digram_t di = digram(text[i], text[j]);

                    // update counter
                    size_t c = count[di] + 1;
                    count[di] = c;

                    // update max
                    if(c > max_count) {
                        max = di;
                        max_count = c;
                    }

                    // advance
                    i = j;
                }
            }

            // replace most common digram (max)
            if(max_count > 1) {
                sym_t new_sym = sigma + grammar.size();
                grammar.push_back(max);

                /*DLOG(INFO) << "most common digram is " << std::hex << max <<
                    " (count = " << max_count << ")" <<
                    ", introducing new symbol " << uint8_t('A' + new_sym - sigma);*/

                size_t i = 0;
                while(i < n - 1) {
                    size_t j = i + 1;
                    while(j < n && !b[j]) ++j; // find next unremoved symbol
                    if(j >= n) break; // break if at end

                    digram_t di = digram(text[i], text[j]);
                    if(di == max) {
                        text[i] = new_sym; // replace symbol at i by new symbol
                        b[j] = 0; // j was removed
                    }

                    // advance
                    i = j;
                    while(i < n - 1 && !b[i]) ++i;
                }
            } else {
                break; // done
            }
        } while(true);

        // debug
        {
            DLOG(INFO) << "grammar:";
            for(size_t i = 0; i < grammar.size(); i++) {
                digram_t di = grammar[i];
                sym_t l = left(di);
                sym_t r = right(di);

                DLOG(INFO) << uint8_t('A' + i) << " -> " <<
                    uint8_t((l < sigma) ? l : ('A' + l - sigma)) <<
                    uint8_t((r < sigma) ? r : ('A' + r - sigma));
            }

            std::ostringstream start;
            for(size_t i = 0; i < n; i++) {
                if(b[i]) {
                    sym_t x = text[i];
                    start << uint8_t((x < sigma) ? x : ('A' + x - sigma));
                }
            }

            DLOG(INFO) << "S -> " << start.str();
        }


        // instantiate encoder
        typename coder_t::Encoder coder(env().env_for_option("coder"),
            output, NoLiterals()); // TODO literator

        // encode amount of grammar rules
        coder.encode(grammar.size(), size_r);

        // lambda for encoding symbols
        auto encode_sym = [&](sym_t x, const Range& r) {
            if(x < sigma) {
                coder.encode(0, bit_r);
                coder.encode(x, literal_r);
            } else {
                coder.encode(1, bit_r);
                coder.encode(x - sigma, r);
            }
        };

        // encode grammar rules
        for(size_t i = 0; i < grammar.size(); i++) {
            digram_t di = grammar[i];

            Range grammar_r(i);
            encode_sym(left(di), grammar_r);
            encode_sym(right(di), grammar_r);
        }

        // encode compressed text (start rule)
        Range grammar_r(grammar.size());
        for(size_t i = 0; i < n; i++) {
            if(b[i]) encode_sym(text[i], grammar_r);
        }

        // clean up
        delete[] text;
    }

    virtual void decompress(Input& input, Output& out) override {
    }
};

}

#endif