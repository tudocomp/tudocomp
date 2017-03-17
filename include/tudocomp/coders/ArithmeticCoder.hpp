#ifndef ARITHMETICCODER_H
#define ARITHMETICCODER_H

#pragma once

#include <sstream>
#include <tudocomp/Coder.hpp>
#include <iostream>

namespace tdc {

/// \brief Defines data encoding to and decoding from a stream of ASCII
///        characters.
///
/// Literals are encoded by their character representation, bits are
/// encoded using either the character \e 0 or \e 1 and integers are
/// encoded as their string representations, terminated by the \e :
/// character.
class ArithmeticCoder : public Algorithm {



public:
    /// \brief Yields the coder's meta information.
    /// \sa Meta
    inline static Meta meta() {
        Meta m("coder", "arithmetic", "Simple range encoding");
        return m;
    }

    /// \cond DELETED
    ArithmeticCoder() = delete;
    /// \endcond

    /// \brief Encodes data to an ASCII character stream.
    class Encoder : public tdc::Encoder {
    private:
        //        std::map<value_t ,int> map;
        //        template<typename value_t, typename container_t>
        //        std::map<value_t, int> count_dict(const container_t& cont) {
        //            std::map<value_t, int> dict;

        //            for(const value_t &v:cont) {
        //               dict[v]++;
        //            }
        //            return dict;
        //        }
        len_t*const C;

        ulong lower_bound=0;
        ulong upper_bound=std::numeric_limits<ulong>::max();

        int literal_counter = 0;

        template<class T>
        len_t* count_alphabet_literals(T&& input) {
            len_t* C { new len_t[ULITERAL_MAX+1] };
            std::memset(C, 0, sizeof(len_t)*(ULITERAL_MAX+1));

            while(input.has_next()) {
                literal_t c = input.next().c;
                DCHECK_LT(static_cast<uliteral_t>(c), ULITERAL_MAX+1);
                DCHECK_LT(C[static_cast<uliteral_t>(c)], std::numeric_limits<len_t>::max());
                ++C[static_cast<uliteral_t>(c)];
            }
            ++C[3]; //End of Text is appended by the algorithm!

            return C;
        }

        void build_intervals(len_t *const c) {
            for(int i=1; i<=ULITERAL_MAX; i++) {
                c[i] = c[i] + c[i-1];
            }
        }

        template<typename value_t>
        inline void setNewBounds(value_t v) {
            ulong range = upper_bound-lower_bound;
            CHECK_NE(lower_bound,upper_bound);
            upper_bound=lower_bound+range/C[ULITERAL_MAX]*C[(int) v];
            if(v != 0) {
                lower_bound=lower_bound+range/C[ULITERAL_MAX]*C[(int) v-1];
            }

        }

        void postProcessing() {
            int counter=0;
            while(upper_bound!=lower_bound) {
                counter++;
                upper_bound>>=1;
                lower_bound>>=1;
            }
            lower_bound <<= 1;
            lower_bound += 1;
            lower_bound <<= counter-1;

            m_out->write_int(lower_bound);
            if(C[0]!=0) {
                m_out->write_int(literal_t(0));
                m_out->write_int(C[0]);
            }
            for(int i=1; i<=ULITERAL_MAX;i++) {
                if(C[i]!=C[i-1]) {
                    m_out->write_int((literal_t) i);
                    m_out->write_int(C[i]);
                }

            }


        }
    public:
        ENCODER_CTOR(env, out, literals),
        C(count_alphabet_literals(std::move(literals))) {
            build_intervals(C);
        }

        template<typename value_t>
        inline void encode(value_t v, const Range& r) {
            literal_counter++;
            setNewBounds(v);

            //minus 1 because of End-of-text which is not in input-string, but counted in dictionary
            if(literal_counter==C[ULITERAL_MAX] -1){
                v=3;
                setNewBounds(v);
                postProcessing();
            }
        }
    };

    /// \brief Decodes data from an Arithmetic character stream.
    class Decoder : public tdc::Decoder {
    private:



    public:
        DECODER_CTOR(env, in) {}

        template<typename value_t>
        inline value_t decode(const Range& r) {
            std::ostringstream os;

            //read code
            ulong code = m_in->read_int<ulong>();

            //read and parse dictionary
            std::vector<std::pair<literal_t ,int> > literals;
            while(!m_in->eof()) {
                literal_t c = m_in->read_int<literal_t>();
                int val = m_in->read_int<int>();
                literals.push_back(std::pair<literal_t, int>(c, val));
            }

            ulong lower_bound = 0;
            ulong upper_bound = std::numeric_limits<ulong>::max();

            CHECK_NE(0, literals.size());

            //count of characters in stream
            int literal_count = literals.back().second;

            char lastChar = 0;
            while(lastChar != 3) {
                const ulong range = upper_bound - lower_bound;
                ulong interval_lower_bound = lower_bound;
                for(const std::pair<literal_t, int> pair : literals) {
                    upper_bound = lower_bound + range/literal_count*pair.second;
                    if(code < upper_bound) {
                        lastChar = pair.first;
                        os << lastChar;
                        lower_bound = interval_lower_bound;
                        break;
                    }
                    interval_lower_bound = upper_bound;
                }
            }

            std::string s = os.str();

            value_t v;
            std::istringstream is(s);
            is >> v;

            std::cout << s <<std::endl;
            std::cout << os <<std::endl;
            std::cout << is <<std::endl;
            std::cout << v <<std::endl;

            return v;
        }
    };
};

}

#endif // ARITHMETICCODER_H
