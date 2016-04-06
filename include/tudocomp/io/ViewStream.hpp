#ifndef _INCLUDED_VIEW_STREAM_HPP
#define _INCLUDED_VIEW_STREAM_HPP

#include <iostream>
#include <memory>
#include <utility>

namespace tudocomp {
namespace io {

/// A wrapper around a istream that reads from
/// a existing memory buffer.
class ViewStream {
    struct membuf : std::streambuf {
        membuf(char* begin, size_t size) {
            this->setg(begin, begin, begin + size);
        }
    };

    char* m_begin;
    size_t m_size;
    std::unique_ptr<membuf> mb;
    std::unique_ptr<std::istream> m_stream;

public:
    ViewStream(char* begin, size_t size) {
        m_begin = begin;
        m_size = size;
        mb = std::unique_ptr<ViewStream::membuf>(
            new ViewStream::membuf { begin, size }
        );
        m_stream = std::unique_ptr<std::istream> {
            new std::istream(&*mb)
        };
    }

    ViewStream(const ViewStream& other) {
        m_begin = other.m_begin;
        m_size = other.m_size;
        mb = std::unique_ptr<ViewStream::membuf>(
            new ViewStream::membuf { m_begin, m_size }
        );
        m_stream = std::unique_ptr<std::istream> {
            new std::istream(&*mb)
        };
    }

    ViewStream(ViewStream&& other) {
        mb = std::move(other.mb);
        m_stream = std::move(other.m_stream);
        m_size = other.m_size;
        m_begin = other.m_begin;
    }

    std::istream& stream() {
        return *m_stream;
    }
};

}}

#endif

