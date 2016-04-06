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

    class OutputStream;

    class Output {
        struct Memory {
            std::vector<uint8_t>* buffer;
        };
        struct File {
            std::string path;
            bool m_overwrite;
        };
        struct Stream {
            std::ostream* stream;
        };

    public:
        boost::variant<Memory, Stream, File> data;

        static Output from_path(std::string path, bool overwrite=false) {
            return Output { File { std::move(path), overwrite } };
        }

        static Output from_memory(std::vector<uint8_t>& buf) {
            return Output { Memory { &buf } };
        }

        static Output from_stream(std::ostream& stream) {
            return Output { Stream { &stream } };
        }

        inline OutputStream as_stream();
    };

    struct OutputStream {
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

            File(std::string&& path, bool overwrite = false) {
                m_path = path;
                if (overwrite) {
                    stream = std::make_unique<std::ofstream>(m_path,
                        std::ios::out | std::ios::binary);
                } else {
                    stream = std::make_unique<std::ofstream>(m_path,
                        std::ios::out | std::ios::binary | std::ios::app);
                }
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
                std::ostream& operator()(OutputStream::Memory& m) const {
                    return m.stream.stream();
                }
                std::ostream& operator()(OutputStream::Stream& m) const {
                    return *m.stream;
                }
                std::ostream& operator()(OutputStream::File& m) const {
                    return *m.stream;
                }
            };
            return boost::apply_visitor(visitor(), data);
        }
    };

    inline OutputStream Output::as_stream() {
        struct visitor: public boost::static_visitor<OutputStream> {
            OutputStream operator()(Output::Memory& mem) const {
                return OutputStream {
                    OutputStream::Memory {
                        BackInsertStream { *mem.buffer }
                    }
                };
            }
            OutputStream operator()(Output::File& f) const {
                auto overwrite = f.m_overwrite;
                f.m_overwrite = false;
                return OutputStream {
                    OutputStream::File {
                        std::string(f.path),
                        overwrite,
                    }
                };
            }
            OutputStream operator()(Output::Stream& strm) const {
                return OutputStream {
                    OutputStream::Stream {
                        strm.stream
                    }
                };
            }
        };
        return boost::apply_visitor(visitor(), data);
    };

}}

#endif

