#pragma once

#include <streambuf>

namespace tdc {
namespace io {

// Wrapper around a streambuf to only read a prefix of the underlying
// buffer.
//
// Note that seeking from the end is a tad heavier, because the wrapper
// needs to find out where the actual end is to decide where to jump.
class PrefixStreamBuffer : public std::streambuf {
private:
    std::streambuf* m_delegate;
    size_t m_prefix;
    size_t m_pos;

public:
    inline PrefixStreamBuffer(std::streambuf& delegate, pos_type prefix)
        : m_delegate(&delegate), m_prefix(prefix), m_pos(0) {
    }

    inline virtual pos_type seekoff(
        off_type off,
        std::ios_base::seekdir dir,
        std::ios_base::openmode which) override {

        if(which == std::ios_base::out) {
            return 0; // not supported
        }

        // TODO: limit to prefix!
        pos_type result;
        if(dir == std::ios_base::end) {
            // find out where the end is
            auto end = m_delegate->pubseekoff(0, std::ios_base::end);
            if(end < pos_type(m_prefix)) {
                // stream is shorter than prefix, perform originally desired
                // seek
                result = m_delegate->pubseekoff(off, dir, which);
            } else {
                // seek from prefix length
                off_type pos = off_type(m_prefix) + off;
                if(pos >= 0) {
                    result = m_delegate->pubseekpos(pos_type(pos), which);
                } else {
                    result = m_delegate->pubseekpos(0, which);
                }
            }
        } else {
            // simply delegate
            result = m_delegate->pubseekoff(off, dir, which);
        }

        m_pos = size_t(result);
        return result;
    }

    inline virtual pos_type seekpos(
        pos_type pos,
        std::ios_base::openmode which) override {

        if(which == std::ios_base::out) {
            return 0; // not supported
        }

        auto result = m_delegate->pubseekpos(pos, which);
        m_pos = size_t(result);
        return result;
    }

    inline virtual int_type underflow() override {
        if(m_pos >= m_prefix) {
            return traits_type::eof(); // prefix fully read
        } else {
            return m_delegate->sgetc();
        }
    }

    inline virtual int_type uflow() override {
        if(m_pos >= m_prefix) {
            return traits_type::eof(); // prefix fully read
        } else if(traits_type::eq_int_type(
            m_delegate->sgetc(),
            traits_type::eof())) {

            return traits_type::eof();
        } else {
            ++m_pos;
            return m_delegate->sbumpc();
        }
    }
};

}}
