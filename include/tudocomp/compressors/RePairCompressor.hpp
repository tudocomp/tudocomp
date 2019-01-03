#pragma once

#include <tudocomp/Compressor.hpp>
#include <tudocomp/decompressors/WrapDecompressor.hpp>

#include <tudocomp/Range.hpp>
#include <tudocomp/coders/BitCoder.hpp> //default

#include <tudocomp/util/Counter.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

template <typename coder_t>
class RePairCompressor : public CompressorAndDecompressor {
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
    class Literals : LiteralIterator {
    private:
        const text_t* m_text;
        len_t         m_text_size;
        const len_compact_t*  m_next;
        len_t         m_pos;

        std::vector<uliteral_t> m_g_literals;
        len_t                   m_g_pos;

    public:
        inline Literals(const text_t& text,
                        len_t text_size,
                        const len_compact_t* next,
                        const grammar_t& grammar)
            : m_text(&text), m_text_size(text_size), m_next(next),
              m_pos(0), m_g_pos(0) {

            // count literals from right side of grammar rules
            for(digram_t di : grammar) {
                sym_t l = left(di);
                if(l < sigma) m_g_literals.push_back(uliteral_t(l));

                sym_t r = right(di);
                if(r < sigma) m_g_literals.push_back(uliteral_t(r));
            }
        }

        inline bool has_next() const {
            return m_pos < m_text_size || m_g_pos < m_g_literals.size();
        }

        inline Literal next() {
            assert(has_next());

            if(m_pos < m_text_size) {
                // from encoded text
                auto l = Literal { uliteral_t((*m_text)[m_pos]), m_pos };
                m_pos = m_next[m_pos];
                return l;
            } else {
                // from grammar right sides
                auto l = Literal { m_g_literals[m_g_pos],
                                   m_text_size + 2 * m_g_pos };
                ++m_g_pos;
                return l;
            }
        }
    };

public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "repair",
            "Grammar compression using Re-Pair.");
        m.param("coder", "The output encoder.")
            .strategy<coder_t>(TypeDesc("coder"), Meta::Default<BitCoder>());
        m.param("max_rules",
            "The maximum amount of grammar rules (0 = unlimited)."
        ).primitive(0);
        return m;
    }

    using CompressorAndDecompressor::CompressorAndDecompressor;

    virtual void compress(Input& input, Output& output) override {
        // options
        size_t max_rules = config().param("max_rules").as_uint();
        if(max_rules == 0) max_rules = SIZE_MAX;

        // prepare editable text
        len_t n;
        sym_t *text;
        len_compact_t *next; //TODO use an int vector of required bit width

        {
            auto view = input.as_view();
            n = view.size();
            text = new sym_t[n];
            next = new len_compact_t[n];

            for(size_t i = 0; i < n; i++) {
                text[i] = view[i];
                next[i] = i + 1;
            }
        }

        // compute RePair grammar
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

        StatPhase::log("rules", grammar.size());
        StatPhase::log("replaced", num_replaced);

        // instantiate encoder
        typename coder_t::Encoder coder(config().sub_config("coder"),
            output, Literals<sym_t*>(text, n, next, grammar));

        // encode amount of grammar rules
        coder.encode(grammar.size(), len_r);

        // lambda for encoding symbols
        auto encode_sym = [&](sym_t x, const Range& r) {
            if(x < sigma) {
                coder.encode(false, bit_r);
                coder.encode(x, literal_r);
            } else {
                coder.encode(true, bit_r);
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

        StatPhase::log("grammar_terms", num_grammar_terminals);
        StatPhase::log("grammar_nonterms", num_grammar_nonterminals);

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

        StatPhase::log("text_terms", num_text_terminals);
        StatPhase::log("text_nonterms", num_text_nonterminals);

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
        typename coder_t::Decoder decoder(config().sub_config("coder"), input);

        // lambda for decoding symbols
        auto decode_sym = [&](const Range& r) {
            bool is_nonterminal = decoder.template decode<bool>(bit_r);
            if(is_nonterminal) {
                auto dec = decoder.template decode<sym_t>(r);
                return sigma + dec;
            } else {
                auto dec = sym_t(decoder.template decode<uliteral_t>(literal_r));
                return dec;
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

    inline std::unique_ptr<Decompressor> decompressor() const override {
        return std::make_unique<WrapDecompressor>(*this);
    }
};

}

