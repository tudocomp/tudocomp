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

        Counter<sym_t>::ranking_t m_ranking;
        size_t m_sigma_bits;

    public:
        ENCODER_CTOR(env, out, literals) {
            m_k     = this->env().option("kmer").as_integer();
            assert(m_k > 1 && m_k <= max_kmer);

            uliteral_t* kmer = new uliteral_t[m_k];
            size_t last_pos = 0;
            size_t kmer_len = 0;

            Counter<sym_t> alphabet;
            Counter<sym_t> kmers;

            while(literals.has_next()) {
                Literal l = literals.next();

                // fill k-mer (TODO: ring buffer)
                if(l.pos == last_pos + 1) {
                    kmer_len = std::min(m_k, kmer_len + 1);
                    for(auto i = m_k - 1; i > 0; i--) kmer[i] = kmer[i-1];
                } else {
                    kmer_len = 1;
                }
                kmer[0] = l.c;

                // count k-mer if complete
                if(kmer_len == m_k) {
                    kmers.increase(encode_kmer(kmer, m_k));
                }

                // count single literal
                alphabet.increase(sym_t(l.c));

                // save position
                last_pos = l.pos;
            }

            m_ranking  = alphabet.createRanking();

            auto sigma = m_ranking.size();
            m_sigma_bits = bits_for(sigma);

            // TODO merge kmers into alphabet ?

            // debug
            /*{
                DLOG(INFO) << "m_sigma_bits = " << m_sigma_bits;

                DLOG(INFO) << "ranking:";
                for(auto e : m_ranking) {
                    DLOG(INFO) << "\t'" << uliteral_t(e.first) << "' -> " << e.second;
                }

                auto kmer_ranking = kmers.createRanking(); //TODO limit size
                DLOG(INFO) << "kmer ranking:";
                for(auto e : kmer_ranking) {
                    std::ostringstream s;
                    decode_kmer(e.first, kmer, m_k);
                    for(ssize_t i = m_k - 1; i >= 0; i--) s << kmer[i];
                    DLOG(INFO) << "\t'" << s.str() << "' -> " << e.second;
                }
            }*/

            // encode ranking
            m_out->write_compressed_int(sigma);
            for(auto e : alphabet.getSorted()) {
                m_out->write_compressed_int(e.first);
            }

            // clean up
            delete[] kmer;
        }

        template<typename value_t>
        inline void encode(value_t v, const Range& r) {
            // see [Dinklage, 2015]
            v -= value_t(r.min());
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
                m_out->write_int(v - value_t(32),
                    bits_for(r.max() - r.min() - 32UL));
            }
        }

        template<typename value_t>
        inline void encode(value_t v, const LiteralRange&) {
            // TODO: k-mer handling
            sym_t x = sym_t(uliteral_t(v));
            auto r = m_ranking[x];

            // see [Dinklage, 2015]
            if(m_sigma_bits < 6) {
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

        template<typename value_t>
        inline void encode(value_t v, const BitRange&) {
            // single bit
			m_out->write_bit(v);
        }
    };

    class Decoder : public tdc::Decoder {
    private:
        size_t m_k;

        size_t m_sigma_bits;
        sym_t* m_inv_ranking;

    public:
        DECODER_CTOR(env, in) {
            m_k = this->env().option("kmer").as_integer();

            // decode literal ranking
            auto sigma = m_in->read_compressed_int<size_t>();

            m_sigma_bits = bits_for(sigma);
            m_inv_ranking = new sym_t[sigma];

            for(size_t rank = 0; rank < sigma; rank++) {
                auto c = m_in->read_compressed_int<sym_t>();

                //DLOG(INFO) << "decoded rank: " << rank << " -> " <<
                //    c << ", is_kmer = " << is_kmer(c);

                m_inv_ranking[rank] = c;
            }
        }

        ~Decoder() {
            delete[] m_inv_ranking;
        }

		template<typename value_t>
		inline value_t decode(const Range& r) {
            value_t v;
            auto x = m_in->read_int<uint8_t>(2);
            switch(x) {
                case 0: v = m_in->read_int<value_t>(3); break;
                case 1: v = value_t(8) + m_in->read_int<value_t>(3); break;
                case 2: v = value_t(16) + m_in->read_int<value_t>(4); break;
                case 3: v = value_t(32) + m_in->read_int<value_t>(
                                            bits_for(r.max() - r.min() - 32UL));
                        break;
            }

            return v + value_t(r.min());
		}

		template<typename value_t>
		inline value_t decode(const LiteralRange&) {
            // TODO: k-mer handling ???
            size_t r;

            // see [Dinklage, 2015]
            if(m_sigma_bits < 6) {
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

            return value_t(m_inv_ranking[r]);
		}

		template<typename value_t>
		inline value_t decode(const BitRange&) {
            // single bit
			return value_t(m_in->read_bit());
		}
    };
};

}

#endif
