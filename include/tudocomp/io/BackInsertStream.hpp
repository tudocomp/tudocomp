#ifndef _INCLUDED_BACK_INSERT_STREAM_HPP
#define _INCLUDED_BACK_INSERT_STREAM_HPP

#include <iostream>
#include <memory>
#include <sstream>
#include <utility>

#include <tudocomp/io/vecstreambuf.hpp>

namespace tudocomp {
namespace io {

/// A ostream that writes bytes into a stl byte container.
class BackInsertStream {
    std::vector<uint8_t>* buffer;
    std::unique_ptr<vecstreambuf<uint8_t>> outBuf;
    std::unique_ptr<std::stringstream> ss;
    std::ostream* o;

public:
    BackInsertStream(const BackInsertStream& other): BackInsertStream(*other.buffer) {
    }

    BackInsertStream(BackInsertStream&& other) {
        buffer = other.buffer;
        outBuf = std::move(other.outBuf);
        ss = std::move(other.ss);
        o = other.o;

        other.buffer = nullptr;
        other.o = nullptr;
    }

    BackInsertStream(std::vector<uint8_t>& buf) {
        buffer = &buf;
        outBuf = std::unique_ptr<vecstreambuf<uint8_t>>(new vecstreambuf<uint8_t>(buf));
        ss = std::unique_ptr<std::stringstream> {
            new std::stringstream()
        };
        o = &*ss;
        o->rdbuf(&*outBuf);
    }

    std::ostream& stream() {
        return *o;
    }
};

}}

#endif

