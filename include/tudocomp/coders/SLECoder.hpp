#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Coder.hpp>
#include <tudocomp/util/Counter.hpp>

namespace tdc {

class SLECoder : public Algorithm {
private:
    typedef size_t sym_t;
    static const size_t max_kmer = sizeof(sym_t) - 1;
    static const size_t kmer_mask = 0xFFUL << (8UL * max_kmer);

    static inline bool is_kmer(sym_t x) {
        return (x & kmer_mask) == kmer_mask;
    }

    static inline sym_t compile_kmer(const uliteral_t* kmer, size_t k) {
        sym_t kmer_sym = 0;

        for(size_t i = 0; i < k; i++) {
            kmer_sym |= (sym_t(kmer[k-1-i]) << 8UL * i);
        }

        return kmer_sym | kmer_mask;
    }

    static inline void decompile_kmer(sym_t x, uliteral_t* kmer, size_t k) {
        for(size_t i = 0; i < k; i++) {
            kmer[k-1-i] = (x >> 8UL * i) & 0xFFUL;
        }
    }

public:
    inline static Meta meta() {
        Meta m("coder", "sle", "Static low entropy encoding conforming [Dinklage, 2015]");
        m.option("kmer").dynamic(3);
        return m;
    }

    SLECoder() = delete;

    class Encoder : public tdc::Encoder {
    private:
        size_t m_k;

        uliteral_t* m_kmer;
        size_t m_kmer_cur;

        Counter<sym_t>::ranking_t m_ranking;
        size_t m_sigma_bits;

        inline bool kmer_full() {
            return m_kmer_cur == m_k;
        }

        inline bool kmer_roll(uliteral_t c, uliteral_t& out) {
            bool roll_out = kmer_full();
            if(roll_out) {
                out = m_kmer[0];

                for(size_t i = 0; i < m_k - 1; i++) m_kmer[i] = m_kmer[i+1];
                --m_kmer_cur;
            }

            m_kmer[m_kmer_cur++] = c;
            return roll_out;
        }

        //DEBUG
        inline std::string kmer_str() {
            std::ostringstream s;
            for(size_t i = 0; i < m_k; i++) s << m_kmer[i];
            return s.str();
        }

    public:
        template<typename literals_t>
        inline Encoder(Env&& env, std::shared_ptr<BitOStream> out, literals_t&& literals)
            : tdc::Encoder(std::move(env), out, literals) {

            m_k     = this->env().option("kmer").as_integer();
            assert(m_k <= max_kmer);

            m_kmer = new uliteral_t[m_k];
            m_kmer_cur = 0;
            size_t last_literal_pos = 0;

            Counter<sym_t> alphabet;
            Counter<sym_t> kmers;

            while(literals.has_next()) {
                Literal l = literals.next();

                if(m_k > 1) {
                    // update k-mer buffer
                    if(l.pos != last_literal_pos + 1) {
                        m_kmer_cur = 0; //reset
                    }

                    uliteral_t out;
                    kmer_roll(l.c, out);

                    // count k-mer if complete
                    if(kmer_full()) {
                        auto x = compile_kmer(m_kmer, m_k);
                        //DLOG(INFO) << "count k-mer: " << kmer_str() <<
                        //    " => 0x" << std::hex << x;

                        kmers.increase(x);
                    }
                }

                // count single literal
                alphabet.increase(sym_t(l.c));

                // save position
                last_literal_pos = l.pos;
            }

            auto sigma = alphabet.getNumItems();
            m_sigma_bits = bits_for(sigma - 1);

            // determine alphabet extension size (see [Dinklage, 2015])
            if(m_k > 1) {
                size_t eta_add_bits = ((1UL << m_sigma_bits) == sigma) ? 1 : 2;
                size_t eta = (1UL << (m_sigma_bits + eta_add_bits)) - sigma;

                // merge eta most common k-mers into alphabet
                /*DLOG(INFO) <<
                    "sigma0 = " << sigma <<
                    ", eta = " << eta <<
                    ", kmers = " << kmers.getNumItems();*/
                for(auto e : kmers.getSorted()) {
                    alphabet.setCount(e.first, e.second);
                    if(--eta == 0) break; //no more than eta
                }

                // update sigma
                sigma = alphabet.getNumItems();
                m_sigma_bits = bits_for(sigma - 1);
            }

            // create ranking
            m_ranking  = alphabet.createRanking();

            // debug
            /*{
                DLOG(INFO) << "m_sigma_bits = " << m_sigma_bits;
                DLOG(INFO) << "ranking:";
                for(auto e : m_ranking) {
                    DLOG(INFO) << "\t'" << std::hex << e.first << "' -> " <<
                        std::dec << e.second << ", is_kmer = " << is_kmer(e.first);
                }
            }*/

            // encode ranking
            m_out->write_compressed_int(sigma);
            for(auto e : alphabet.getSorted()) {
                m_out->write_compressed_int(e.first);
            }

            // reset current k-mer
            m_kmer_cur = 0;
        }

        template<typename literals_t>
        inline Encoder(Env&& env, Output& out, literals_t&& literals)
            : Encoder(std::move(env), std::make_shared<BitOStream>(out), literals) {
        }

        ~Encoder() {
            // flush
            flush_kmer();

            // clean up
            delete[] m_kmer;
        }

    private:
        inline void flush_kmer() {
            // encode literals in k-mer buffer
            for(size_t i = 0; i < m_kmer_cur; i++) {
                encode_sym(sym_t(m_kmer[i]));
            }

            // reset current k-mer
            m_kmer_cur = 0;
        }

        inline void encode_sym(sym_t x) {
            auto r = m_ranking[x];
            //DLOG(INFO) << "encode_sym(0x" << std::hex << x << "), r = " << std::dec << r;

            // see [Dinklage, 2015]
            if(m_sigma_bits < 4) {
                m_out->write_int(r, m_sigma_bits);
            } else if(m_sigma_bits < 6) {
                if(r < 4) {
                    m_out->write_bit(0);
                    m_out->write_int(r, 2);
                } else {
                    m_out->write_bit(1);
                    m_out->write_int(r, m_sigma_bits);
                }
            } else if(m_sigma_bits == 6) {
                if(r < 8) {
                    m_out->write_int(0, 2);
                    m_out->write_int(r, 3);
                } else if(r < 16) {
                    m_out->write_int(1, 2);
                    m_out->write_int(r - 8UL, 3);
                } else if(r < 32) {
                    m_out->write_int(2, 2);
                    m_out->write_int(r - 16UL, 4);
                } else {
                    m_out->write_int(3, 2);
                    m_out->write_int(r, m_sigma_bits);
                }
            } else {
                if(r < 4) {
                    m_out->write_int(0, 3);
                    m_out->write_int(r, 2);
                } else if(r < 8) {
                    m_out->write_int(1, 3);
                    m_out->write_int(r - 4UL, 2);
                } else if(r < 12) {
                    m_out->write_int(2, 3);
                    m_out->write_int(r - 8UL, 2);
                } else if(r < 16) {
                    m_out->write_int(3, 3);
                    m_out->write_int(r - 12UL, 2);
                } else if(r < 24) {
                    m_out->write_int(4, 3);
                    m_out->write_int(r - 16UL, 3);
                } else if(r < 32) {
                    m_out->write_int(5, 3);
                    m_out->write_int(r - 24UL, 3);
                } else if(r < 40) {
                    m_out->write_int(6, 3);
                    m_out->write_int(r - 32UL, 3);
                } else {
                    m_out->write_int(7, 3);
                    m_out->write_int(r, m_sigma_bits);
                }
            }
        }

    public:
        template<typename value_t>
        inline void encode(value_t v, const LiteralRange&) {
            auto c = uliteral_t(v);

            uliteral_t out = 0;
            if(kmer_roll(c, out)) {
                // encode rolled out character
                encode_sym(sym_t(out));
            }

            if(kmer_full()) {
                sym_t x = compile_kmer(m_kmer, m_k);
                if(m_ranking.find(x) != m_ranking.end()) {
                    // current k-mer exists in ranking
                    encode_sym(x);
                    m_kmer_cur = 0; // reset
                }
            }
        }

        template<typename value_t>
        inline void encode(value_t v, const Range& r) {
            flush_kmer(); // k-mer interrupted
            m_out->write_int(v - r.min(), bits_for(r.delta()));
        }

        template<typename value_t>
        inline void encode(value_t v, const MinDistributedRange& r) {
            flush_kmer(); // k-mer interrupted

            // see [Dinklage, 2015]
            v -= r.min();
            auto bits = bits_for(r.delta());

            if(bits <= 5) {
                m_out->write_int(v, bits);
            } else {
                if(v < 8) {
                    m_out->write_int(0, 2);
                    m_out->write_int(v, 3);
                } else if(v < 16) {
                    m_out->write_int(1, 2);
                    m_out->write_int(v - value_t(8), 3);
                } else if(v < 32) {
                    m_out->write_int(2, 2);
                    m_out->write_int(v - value_t(16), 4);
                } else {
                    m_out->write_int(3, 2);
                    m_out->write_int(v, bits);
                }
            }
        }

        template<typename value_t>
        inline void encode(value_t v, const BitRange&) {
            flush_kmer(); // k-mer interrupted
			m_out->write_bit(v);
        }
    };

    class Decoder : public tdc::Decoder {
    private:
        size_t m_k;

        size_t m_sigma_bits;
        sym_t* m_inv_ranking;

        uliteral_t* m_kmer;
        size_t m_kmer_read;

        inline void reset_kmer() {
            m_kmer_read = SIZE_MAX;
        }

    public:
        inline Decoder(Env&& env, std::shared_ptr<BitIStream> in)
            : tdc::Decoder(std::move(env), in) {
            m_k = this->env().option("kmer").as_integer();

            m_kmer = new uliteral_t[m_k];
            reset_kmer();

            // decode literal ranking
            auto sigma = m_in->read_compressed_int<size_t>();

            m_sigma_bits = bits_for(sigma - 1);
            m_inv_ranking = new sym_t[sigma];

            for(size_t rank = 0; rank < sigma; rank++) {
                auto c = m_in->read_compressed_int<sym_t>();

                //DLOG(INFO) << "decoded rank: " << rank << " -> " <<
                //    std::hex << c << ", is_kmer = " << is_kmer(c);

                m_inv_ranking[rank] = c;
            }
        }

        inline Decoder(Env&& env, Input& in)
            : Decoder(std::move(env), std::make_shared<BitIStream>(in)) {
        }

        ~Decoder() {
            delete[] m_kmer;
            delete[] m_inv_ranking;
        }

        inline bool eof() const {
            if(m_kmer_read < m_k) {
                // still decoding a k-mer
                return false;
            } else {
                // check if stream is over
                return tdc::Decoder::m_in->eof();
            }
        }

		template<typename value_t>
		inline value_t decode(const LiteralRange&) {
            if(m_kmer_read < m_k) {
                // continue reading from current k-mer
                return value_t(m_kmer[m_kmer_read++]);
            }

            size_t r = 0;

            // see [Dinklage, 2015]
            if(m_sigma_bits < 4) {
                r = m_in->read_int<size_t>(m_sigma_bits);
            } else if(m_sigma_bits < 6) {
                auto b = m_in->read_bit();
                if(b == 0) r = m_in->read_int<size_t>(2);
                else       r = m_in->read_int<size_t>(m_sigma_bits);
            } else if(m_sigma_bits == 6) {
                auto x = m_in->read_int<uint8_t>(2);
                switch(x) {
                    case 0: r = m_in->read_int<size_t>(3); break;
                    case 1: r = 8UL + m_in->read_int<size_t>(3); break;
                    case 2: r = 16UL + m_in->read_int<size_t>(4); break;
                    case 3: r = m_in->read_int<size_t>(m_sigma_bits); break;
                }
            } else {
                auto x = m_in->read_int<uint8_t>(3);
                switch(x) {
                    case 0: r = m_in->read_int<size_t>(2); break;
                    case 1: r = 4UL + m_in->read_int<size_t>(2); break;
                    case 2: r = 8UL + m_in->read_int<size_t>(2); break;
                    case 3: r = 12UL + m_in->read_int<size_t>(2); break;
                    case 4: r = 16UL + m_in->read_int<size_t>(3); break;
                    case 5: r = 24UL + m_in->read_int<size_t>(3); break;
                    case 6: r = 32UL + m_in->read_int<size_t>(3); break;
                    case 7: r = m_in->read_int<size_t>(m_sigma_bits); break;
                }
            }

            auto x = m_inv_ranking[r];
            //DLOG(INFO) << "decoded 0x" << std::hex << x << " from r = " << std::dec << r;

            if(is_kmer(x)) {
                decompile_kmer(x, m_kmer, m_k);
                m_kmer_read = 1;
                return m_kmer[0];
            } else {
                return value_t(x);
            }
		}

		template<typename value_t>
		inline value_t decode(const Range& r) {
            reset_kmer(); // current k-mer interrupted
            return m_in->read_int<value_t>(bits_for(r.delta())) + value_t(r.min());
        }

		template<typename value_t>
		inline value_t decode(const MinDistributedRange& r) {
            reset_kmer(); // current k-mer interrupted

            auto bits = bits_for(r.delta());
            value_t v = 0;

            if(bits <= 5) {
                v = m_in->read_int<value_t>(bits);
            } else {
                auto x = m_in->read_int<uint8_t>(2);
                switch(x) {
                    case 0: v = m_in->read_int<value_t>(3); break;
                    case 1: v = value_t(8) + m_in->read_int<value_t>(3); break;
                    case 2: v = value_t(16) + m_in->read_int<value_t>(4); break;
                    case 3: v = m_in->read_int<value_t>(bits); break;
                }
            }

            return v + value_t(r.min());
		}

		template<typename value_t>
		inline value_t decode(const BitRange&) {
            reset_kmer(); // current k-mer interrupted
			return value_t(m_in->read_bit());
		}
    };
};

}

