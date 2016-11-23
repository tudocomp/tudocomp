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
    typedef std::vector<digram_t> grammar_t;
    static const size_t digram_shift = 32UL;
    static const sym_t sigma = 256; //TODO

    inline static digram_t digram(sym_t l, sym_t r) {
        return (digram_t(l) << digram_shift) | digram_t(r);
    }

    inline static sym_t left(digram_t di) {
        return sym_t(di >> digram_shift);
    }

    inline static sym_t right(digram_t di) {
        return sym_t(di);
    }

    template<typename text_t, typename bv_t>
    class Literals {
    private:
        const text_t* m_text;
        len_t         m_text_size;
        const bv_t*   m_bv;
        len_t         m_pos;

        void skip_removed() {
            while(m_pos < m_text_size && !(*m_bv)[m_pos]) ++m_pos;
        }

    public:
        inline Literals(const text_t& text, len_t text_size, const bv_t& bv)
            : m_text(&text), m_text_size(text_size), m_bv(&bv), m_pos(0) {

            skip_removed();
        }

        inline bool has_next() const {
            return m_pos < m_text_size;
        }

        inline Literal next() {
            assert(has_next());

            auto l = Literal { uliteral_t((*m_text)[m_pos]), m_pos };
            ++m_pos;
            skip_removed();
            return l;
        }
    };

public:
    inline static Meta meta() {
        Meta m("compressor", "repair", "Re-Pair compression");
        m.option("coder").templated<coder_t, BitOptimalCoder>();
        return m;
    }

    inline RePairCompressor(Env&& env) : Compressor(std::move(env)) {}

    virtual void compress(Input& input, Output& output) override {
        // prepare editable text
        len_t n;
        sym_t *text;

        {
            auto view = input.as_view();
            n = view.size();
            text = new sym_t[n];

            for(size_t i = 0; i < n; i++) {
                text[i] = view[i];
            }
        }

        // compute RePair grammar
        sdsl::bit_vector b(n, 1); // if b[i] = 0, there is no more symbol at i
        grammar_t grammar;

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
        /*{
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
        }*/

        // instantiate encoder
        typename coder_t::Encoder coder(env().env_for_option("coder"),
            output, Literals<sym_t*, sdsl::bit_vector>(text, n, b));

        // encode amount of grammar rules
        coder.encode(grammar.size(), len_r);

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

private:
    inline static void decode(sym_t x, const grammar_t& grammar, std::ostream& ostream) {
        if(x < sigma) {
            // terminal
            ostream << uliteral_t(x);
        } else {
            // non-terminal
            digram_t di = grammar[x - sigma];
            decode(left(di),  grammar, ostream);
            decode(right(di), grammar, ostream);
        }
    }

public:
    virtual void decompress(Input& input, Output& output) override {
        // instantiate decoder
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);

        // lambda for decoding symbols
        auto decode_sym = [&](const Range& r) {
            bool is_nonterminal = decoder.template decode<bool>(bit_r);
            if(is_nonterminal) {
                return sigma + decoder.template decode<sym_t>(r);
            } else {
                return decoder.template decode<sym_t>(literal_r);
            }
        };

        // decode grammar
        grammar_t grammar;
        {
            auto num_rules = decoder.template decode<size_t>(len_r);
            while(num_rules--) {
                Range grammar_r(grammar.size());
                sym_t l = decode_sym(grammar_r);
                sym_t r = decode_sym(grammar_r);
                grammar.push_back(digram(l, r));
            }
        }

        // debug
        /*{
            DLOG(INFO) << "decoded grammar:";
            for(size_t i = 0; i < grammar.size(); i++) {
                digram_t di = grammar[i];
                sym_t l = left(di);
                sym_t r = right(di);

                DLOG(INFO) << uint8_t('A' + i) << " -> " <<
                    uint8_t((l < sigma) ? l : ('A' + l - sigma)) <<
                    uint8_t((r < sigma) ? r : ('A' + r - sigma));
            }
        }*/

        // decode text
        Range grammar_r(grammar.size());

        auto ostream = output.as_stream();
        while(!decoder.eof()) {
            decode(decode_sym(grammar_r), grammar, ostream);
        }
    }
};

}

#endif