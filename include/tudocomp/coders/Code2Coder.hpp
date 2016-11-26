#ifndef _INCLUDED_CODE2_CODER_HPP
#define _INCLUDED_CODE2_CODER_HPP

#include <tudocomp/util.hpp>
#include <tudocomp/Coder.hpp>
#include <tudocomp/util/Counter.hpp>

namespace tdc {

class Code2Coder : public Algorithm {
private:
    typedef size_t sym_t;
    static const size_t max_kmer = sizeof(sym_t) - 1;
    static const size_t kmer_mask = 0xFFUL << (8UL * max_kmer);

    static inline bool is_kmer(sym_t x) {
        return (x & kmer_mask) == kmer_mask;
    }

    static inline sym_t encode_kmer(const uliteral_t* kmer, size_t k) {
        sym_t kmer_sym = 0;

        for(size_t i = 0; i < k; i++) {
            kmer_sym |= (kmer[i] << (8 * i));
        }

        return kmer_sym | kmer_mask;
    }

    static inline void decode_kmer(sym_t x, uliteral_t* kmer, size_t k) {
        for(size_t i = 0; i < k; i++) {
            kmer[i] = (x >> (8 * i)) & 0xFF;
        }
    }

public:
    inline static Meta meta() {
        Meta m("coder", "code2", "Encoding conforming [Dinklage, 2015]");
        m.option("kmer").dynamic("3");
        return m;
    }

    Code2Coder() = delete;

    class Encoder : public tdc::Encoder {
    private:
        size_t m_k;

        uliteral_t* m_kmer;
        size_t m_kmer_cur;

        Counter<sym_t>::ranking_t m_ranking;
        size_t m_sigma_bits;

        //DEBUG
        size_t dm_encoded_kmers;

    public:
        ENCODER_CTOR(env, out, literals) {
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
                    if(l.pos == last_literal_pos + 1) {
                        if(m_kmer_cur == m_k) {
                            for(size_t i = 0; i < m_k - 1; i++) m_kmer[i] = m_kmer[i+1];
                            --m_kmer_cur;
                        }
                    } else {
                        m_kmer_cur = 0;
                    }
                    m_kmer[m_kmer_cur++] = l.c;

                    // count k-mer if complete
                    if(m_kmer_cur == m_k) {
                        kmers.increase(encode_kmer(m_kmer, m_k));
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
                    if(is_kmer(e.first)) {
                        std::ostringstream s;
                        decode_kmer(e.first, m_kmer, m_k);
                        for(size_t i = 0; i < m_k; i++) s << m_kmer[i];
                        DLOG(INFO) << "\t'" << s.str() << "' -> " << e.second;
                    } else {
                        DLOG(INFO) << "\t'" << uliteral_t(e.first) << "' -> " << e.second;
                    }
                }
            }*/

            // encode ranking
            m_out->write_compressed_int(sigma);
            for(auto e : alphabet.getSorted()) {
                m_out->write_compressed_int(e.first);
            }

            // reset current k-mer
            m_kmer_cur = 0;

            dm_encoded_kmers = 0;
        }

        ~Encoder() {
            encode_current_kmer(); // remaining
            std::cerr << "actually encoded " << m_k << "-mers: " <<
                dm_encoded_kmers << std::endl;

            delete[] m_kmer;
        }

        template<typename value_t>
        inline void encode(value_t v, const Range& r) {
            encode_current_kmer(); // k-mer interrupted

            v -= value_t(r.min());

            // see [Dinklage, 2015]
            auto delta = r.max() - r.min();
            auto delta_bits = bits_for(delta);

            if(delta_bits <= 5) {
                m_out->write_int(v, delta_bits);
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
                    m_out->write_int(v - value_t(32UL), bits_for(delta - 32UL));
                }
            }
        }

    private:
        inline void encode_current_kmer() {
            if(m_kmer_cur == m_k) {
                sym_t x = encode_kmer(m_kmer, m_k);
                if(m_ranking.find(x) != m_ranking.end()) {
                    // k-mer exists in ranking
                    ++dm_encoded_kmers;
                    encode_sym(x);
                    m_kmer_cur = 0;
                }
            }

            // encode k-mer one by one
            if(m_kmer_cur > 0) {
                for(size_t i = 0; i < m_kmer_cur; i++) {
                    encode_sym(m_kmer[i]);
                }

                m_kmer_cur = 0;
            }
        }

        inline void encode_sym(sym_t x) {
            auto r = m_ranking[x];

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
            if(m_k > 1) {
                m_kmer[m_kmer_cur++] = uliteral_t(v);
                if(m_kmer_cur == m_k) {
                    encode_current_kmer();
                }
            } else {
                encode_sym(uliteral_t(v));
            }
        }

        template<typename value_t>
        inline void encode(value_t v, const BitRange&) {
            encode_current_kmer(); // k-mer interrupted

            // single bit
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

    public:
        DECODER_CTOR(env, in) {
            m_k = this->env().option("kmer").as_integer();

            m_kmer = new uliteral_t[m_k];
            m_kmer_read = SIZE_MAX;

            // decode literal ranking
            auto sigma = m_in->read_compressed_int<size_t>();

            m_sigma_bits = bits_for(sigma - 1);
            m_inv_ranking = new sym_t[sigma];

            for(size_t rank = 0; rank < sigma; rank++) {
                auto c = m_in->read_compressed_int<sym_t>();

                /*DLOG(INFO) << "decoded rank: " << rank << " -> " <<
                    c << ", is_kmer = " << is_kmer(c);*/

                m_inv_ranking[rank] = c;
            }
        }

        ~Decoder() {
            delete[] m_kmer;
            delete[] m_inv_ranking;
        }

		template<typename value_t>
		inline value_t decode(const Range& r) {
            m_kmer_read = SIZE_MAX; // current k-mer interrupted

            auto delta = r.max() - r.min();
            auto delta_bits = bits_for(delta);

            value_t v;

            if(delta_bits <= 5) {
                v = m_in->read_int<value_t>(delta_bits);
            } else {
                auto x = m_in->read_int<uint8_t>(2);
                switch(x) {
                    case 0: v = m_in->read_int<value_t>(3); break;
                    case 1: v = value_t(8) + m_in->read_int<value_t>(3); break;
                    case 2: v = value_t(16) + m_in->read_int<value_t>(4); break;
                    case 3: v = value_t(32) + m_in->read_int<value_t>(
                                                bits_for(r.max() - r.min() - 32UL));
                            break;
                }
            }

            return v + value_t(r.min());
		}

		template<typename value_t>
		inline value_t decode(const LiteralRange&) {
            if(m_kmer_read < m_k) {
                // continue reading from current k-mer
                return value_t(m_kmer[m_kmer_read++]);
            }

            size_t r;

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

            sym_t x = m_inv_ranking[r];
            if(is_kmer(x)) {
                decode_kmer(x, m_kmer, m_k);
                m_kmer_read = 1;
                return m_kmer[0];
            } else {
                return value_t(x);
            }
		}

		template<typename value_t>
		inline value_t decode(const BitRange&) {
            m_kmer_read = SIZE_MAX; // current k-mer interrupted

            // single bit
			return value_t(m_in->read_bit());
		}
    };
};

}

#endif
