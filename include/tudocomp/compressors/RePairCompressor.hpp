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

    template<typename text_t>
    class Literals {
    private:
        const text_t* m_text;
        len_t         m_text_size;
        const len_t*  m_next;
        len_t         m_pos;

    public:
        inline Literals(const text_t& text, len_t text_size, const len_t* next)
            : m_text(&text), m_text_size(text_size), m_next(next), m_pos(0) {
        }

        inline bool has_next() const {
            return m_pos < m_text_size;
        }

        inline Literal next() {
            assert(has_next());

            auto l = Literal { uliteral_t((*m_text)[m_pos]), m_pos };
            m_pos = m_next[m_pos];
            return l;
        }
    };

public:
    inline static Meta meta() {
        Meta m("compressor", "repair", "Re-Pair compression");
        m.option("coder").templated<coder_t, BitOptimalCoder>();
        m.option("max_rules").dynamic("0");
        return m;
    }

    inline RePairCompressor(Env&& env) : Compressor(std::move(env)) {}

    virtual void compress(Input& input, Output& output) override {
        // options
        size_t max_rules = env().option("max_rules").as_integer();
        if(max_rules == 0) max_rules = SIZE_MAX;

        // prepare editable text
        len_t n;
        sym_t *text;
        len_t *next;

        {
            auto view = input.as_view();
            n = view.size();
            text = new sym_t[n];
            next = new len_t[n];

            for(size_t i = 0; i < n; i++) {
                text[i] = view[i];
                next[i] = i + 1;
            }
        }

        // compute RePair grammar
        //sdsl::bit_vector b(n, 1); // if b[i] = 0, there is no more symbol at i
        grammar_t grammar;

        size_t num_replaced = 0;

        do {
            // count digrams
            digram_t max;
            size_t max_count = 0;

            {
                std::unordered_map<digram_t, size_t> count;
                // TODO: probably not optimal, but twice as fast as std::map

                size_t i = 0;
                while(i < n - 1) {
                    size_t j = next[i];
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
                    size_t j = next[i];
                    if(j >= n) break; // break if at end

                    digram_t di = digram(text[i], text[j]);
                    if(di == max) {
                        text[i] = new_sym; // replace symbol at i by new symbol
                        next[i] = next[j];

                        ++num_replaced;
                    }

                    // advance
                    i = next[i];
                }
            } else {
                break; // done
            }
        } while(grammar.size() < max_rules);

        // debug
        /*
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
            for(size_t i = 0; i < n; i = next[i]) {
                sym_t x = text[i];
                start << uint8_t((x < sigma) ? x : ('A' + x - sigma));
            }

            DLOG(INFO) << "S -> " << start.str();
        }
        */

        env().log_stat("rules", grammar.size());
        env().log_stat("replaced", num_replaced);

        // instantiate encoder
        typename coder_t::Encoder coder(env().env_for_option("coder"),
            output, Literals<sym_t*>(text, n, next));

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
        size_t num_grammar_terminals = 0;
        size_t num_grammar_nonterminals = 0;

        for(size_t i = 0; i < grammar.size(); i++) {
            digram_t di = grammar[i];

            Range grammar_r(i);

            // statistics
            sym_t l = left(di);
            if(l < sigma) ++num_grammar_terminals;
            else ++num_grammar_nonterminals;

            sym_t r = right(di);
            if(r < sigma) ++num_grammar_terminals;
            else ++num_grammar_nonterminals;

            encode_sym(l, grammar_r);
            encode_sym(r, grammar_r);
        }

        env().log_stat("grammar_terms", num_grammar_terminals);
        env().log_stat("grammar_nonterms", num_grammar_nonterminals);

        // encode compressed text (start rule)
        size_t num_text_terminals = 0;
        size_t num_text_nonterminals = 0;

        Range grammar_r(grammar.size());
        for(size_t i = 0; i < n; i = next[i]) {
            sym_t x = text[i];

            // statistics
            if(x < sigma) ++num_text_terminals;
            else ++num_text_nonterminals;

            encode_sym(text[i], grammar_r);
        }

        env().log_stat("text_terms", num_text_terminals);
        env().log_stat("text_nonterms", num_text_nonterminals);

        // clean up
        delete[] next;
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