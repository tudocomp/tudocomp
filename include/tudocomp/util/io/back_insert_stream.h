#ifndef TUDOCOMP_BACK_INSERT_STREAM_H
#define TUDOCOMP_BACK_INSERT_STREAM_H

#include <iostream>
#include <memory>
#include <sstream>
#include <utility>

#include "boost/iostreams/stream_buffer.hpp"
#include "boost/iostreams/device/back_inserter.hpp"

namespace tudocomp {

/// A ostream that writes bytes into a stl byte container.
class BackInsertStream {
    // NB: Very hacky implementation right now...

    std::vector<uint8_t>* buffer;
    std::unique_ptr<
        boost::iostreams::stream_buffer<
            boost::iostreams::back_insert_device<
                std::vector<char>>>> outBuf;
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
        outBuf = std::unique_ptr<
            boost::iostreams::stream_buffer<
                boost::iostreams::back_insert_device<
                    std::vector<char>>>> {
            new boost::iostreams::stream_buffer<
                    boost::iostreams::back_insert_device<
                        std::vector<char>>> {
                // TODO: Very iffy cast here...
                *((std::vector<char>*) buffer)
            }
        };
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

}

#endif

