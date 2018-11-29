#pragma once

#include <ostream>
#include <vector>
#include <tudocomp/def.hpp>

namespace tdc {
namespace lzss {

class DecompBackBuffer {
private:
    std::vector<uliteral_t> m_buffer;

public:
    inline DecompBackBuffer() {
    }

    inline void initialize(size_t n) {
        m_buffer.reserve(n);
    }

    inline void decode_literal(uliteral_t c) {
        m_buffer.emplace_back(c);
    }

    inline void decode_factor(len_t src, len_t len) {
        while(len--) m_buffer.emplace_back(m_buffer[src++]);
    }

    inline void process() {
        // no post-processing needed
    }

    inline len_t longest_chain() const {
        return 0;
    }

    inline void write_to(std::ostream& out) {
        for(auto c : m_buffer) out << c;
    }
};

}} //ns

