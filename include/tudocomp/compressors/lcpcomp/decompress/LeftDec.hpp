#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/lzss/DecompBackBuffer.hpp>

namespace tdc {
namespace lcpcomp {

class LeftDec : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("lcpcomp_dec", "left");
        return m;
    }

private:
    lzss::DecompBackBuffer m_buf;

public:
    using Algorithm::Algorithm;

    inline void initialize(size_t n) {
        m_buf.initialize(n);
    }

    inline void decode_literal(uliteral_t c) {
        m_buf.decode_literal(c);
    }

    inline void decode_factor(len_t pos, len_t num) {
        m_buf.decode_factor(pos, num);
    }

    inline void process() {
        m_buf.process();
    }

    inline len_t longest_chain() const {
        return m_buf.longest_chain();
    }

    inline void write_to(std::ostream& out) {
        m_buf.write_to(out);
    }
};

}} //ns

