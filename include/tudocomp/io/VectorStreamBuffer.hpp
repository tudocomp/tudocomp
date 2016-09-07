#ifndef _INCLUDED_VEC_STREAM_BUF_HPP
#define _INCLUDED_VEC_STREAM_BUF_HPP

#include <streambuf>
#include <vector>

namespace tudocomp {
namespace io {

/// \cond INTERNAL

template<typename T>
class VectorStreamBuffer : public std::streambuf {

private:
    std::vector<T> *m_vec;

public:
    VectorStreamBuffer(std::vector<T>& vec) : m_vec(&vec) {
    }

    virtual ~VectorStreamBuffer() {
    }

protected:
    virtual int overflow(int ch) override {
        if(ch == EOF) {
            return EOF;
        } else {
            m_vec->push_back((T)ch);
            return ch;
        }
    }

    virtual int underflow() override {
        return EOF;
    }
};

/// \endcond

}}

#endif

