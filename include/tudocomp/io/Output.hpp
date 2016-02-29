#ifndef _INCLUDED_OUTPUT_HPP
#define _INCLUDED_OUTPUT_HPP

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/variant.hpp>

#include <tudocomp/io/BackInsertStream.hpp>

namespace tudocomp {
namespace io {

    class OutputStreamGuard;

    class Output {
        struct Memory {
            std::vector<uint8_t>* buffer;
        };
        struct File {
            std::string path;
        };
        struct Stream {
            std::ostream* stream;
        };

    public:
        boost::variant<Memory, Stream, File> data;

        static Output from_path(std::string path) {
            return Output { File { std::move(path) } };
        }

        static Output from_memory(std::vector<uint8_t>& buf) {
            return Output { Memory { &buf } };
        }

        static Output from_stream(std::ostream& stream) {
            return Output { Stream { &stream } };
        }

        inline OutputStreamGuard as_stream();
    };

    struct OutputStreamGuard {
        struct Memory {
            BackInsertStream stream;
        };
        struct Stream {
            std::ostream* stream;
        };
        struct File {
            std::string m_path;
            std::unique_ptr<std::ofstream> stream;

            File(const File& other): File(std::string(other.m_path)) {
            }

            File(std::string&& path) {
                m_path = path;
                stream = std::unique_ptr<std::ofstream> {
                    new std::ofstream(m_path, std::ios::out | std::ios::binary)
                };
            }

            File(std::unique_ptr<std::ofstream>&& s) {
                stream = std::move(s);
            }

            File(File&& other) {
                stream = std::move(other.stream);
            }
        };

        boost::variant<Memory, Stream, File> data;

        std::ostream& operator* () {
            struct visitor: public boost::static_visitor<std::ostream&> {
                std::ostream& operator()(OutputStreamGuard::Memory& m) const {
                    return m.stream.stream();
                }
                std::ostream& operator()(OutputStreamGuard::Stream& m) const {
                    return *m.stream;
                }
                std::ostream& operator()(OutputStreamGuard::File& m) const {
                    return *m.stream;
                }
            };
            return boost::apply_visitor(visitor(), data);
        }
    };

    inline OutputStreamGuard Output::as_stream() {
        struct visitor: public boost::static_visitor<OutputStreamGuard> {
            OutputStreamGuard operator()(Output::Memory& mem) const {
                return OutputStreamGuard {
                    OutputStreamGuard::Memory {
                        BackInsertStream { *mem.buffer }
                    }
                };
            }
            OutputStreamGuard operator()(Output::File& f) const {
                return OutputStreamGuard {
                    OutputStreamGuard::File {
                        std::string(f.path)
                    }
                };
            }
            OutputStreamGuard operator()(Output::Stream& strm) const {
                return OutputStreamGuard {
                    OutputStreamGuard::Stream {
                        strm.stream
                    }
                };
            }
        };
        return boost::apply_visitor(visitor(), data);
    };

}}

#endif

