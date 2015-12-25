#ifndef COMPRESSFRAMEWORK_H
#define COMPRESSFRAMEWORK_H

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <map>
#include <sstream>
#include <streambuf>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "boost/utility/string_ref.hpp"
#include "boost/variant.hpp"
#include "boost/iostreams/stream_buffer.hpp"
#include "boost/iostreams/device/back_inserter.hpp"
#include "glog/logging.h"

#include "tudocomp_env.h"
#include "tudocomp_util.h"

namespace tudocomp {
    namespace input {
        struct SliceGuard;
        struct StreamGuard;

        /// Represents the input of an algorithm.
        ///
        /// Can be used as either a istream or a memory buffer
        /// with the as_stream or as_view methods.
        struct Input {
            struct Memory {
                const std::vector<uint8_t>* buffer;
            };
            struct Stream {
                std::istream* stream;
            };
            struct File {
                std::string path;
            };

            boost::variant<Memory, Stream, File> data;

            static Input from_path(std::string&& path) {
                return Input { File { path } };
            }

            static Input from_memory(const std::vector<uint8_t>& buf) {
                return Input { Memory { &buf } };
            }

            static Input from_stream(std::istream& stream) {
                return Input { Stream { &stream } };
            }

            inline SliceGuard as_view();
            inline StreamGuard as_stream();
        };

        struct SliceGuard {
            struct Memory {
                const std::vector<uint8_t>* buffer;
            };

            struct Stream {
                std::vector<uint8_t> buffer;
            };

            boost::variant<Memory, Stream> data;

            boost::string_ref operator* () {
                struct visitor: public boost::static_visitor<boost::string_ref> {
                    boost::string_ref operator()(SliceGuard::Memory& mem) const {
                        return boost::string_ref {
                            (char*) mem.buffer->data(), mem.buffer->size()
                        };
                    }
                    boost::string_ref operator()(SliceGuard::Stream& mem) const {
                        return boost::string_ref {
                            (char*) mem.buffer.data(), mem.buffer.size()
                        };
                    }
                };
                return boost::apply_visitor(visitor(), data);
            }
        };

        inline SliceGuard Input::as_view() {
            struct visitor: public boost::static_visitor<SliceGuard> {
                SliceGuard operator()(Input::Memory& mem) const {
                    return SliceGuard {
                        SliceGuard::Memory { mem.buffer }
                    };
                }
                SliceGuard operator()(Input::File& mem) const {
                    return SliceGuard {
                        SliceGuard::Stream {
                            read_file_to_stl_byte_container<
                                std::vector<uint8_t>>(mem.path)
                        }
                    };
                }
                SliceGuard operator()(Input::Stream& strm) const {
                    return SliceGuard {
                        SliceGuard::Stream {
                            read_stream_to_stl_byte_container<
                                std::vector<uint8_t>>(*strm.stream)
                        }
                    };
                }
            };
            return boost::apply_visitor(visitor(), data);
        };

        struct StreamGuard {
            struct Memory {
                ViewStream stream;
            };
            struct Stream {
                std::istream* stream;
            };
            struct File {
                std::ifstream stream;
            };

            boost::variant<Memory, Stream, File> data;

            std::istream& operator* () {
                struct visitor: public boost::static_visitor<std::istream&> {
                    std::istream& operator()(StreamGuard::Memory& m) const {
                        return m.stream.stream();
                    }
                    std::istream& operator()(StreamGuard::Stream& m) const {
                        return *m.stream;
                    }
                    std::istream& operator()(StreamGuard::File& m) const {
                        return m.stream;
                    }
                };
                return boost::apply_visitor(visitor(), data);
            }
        };

        inline StreamGuard Input::as_stream() {
            struct visitor: public boost::static_visitor<StreamGuard> {
                StreamGuard operator()(Input::Memory& mem) const {
                    return StreamGuard {
                        StreamGuard::Memory {
                            ViewStream {
                                (char*)&(*mem.buffer)[0],
                                mem.buffer->size()
                            }
                        }
                    };
                }
                StreamGuard operator()(Input::File& f) const {
                    return StreamGuard {
                        StreamGuard::File {
                            std::ifstream(f.path, std::ios::in | std::ios::binary)
                        }
                    };
                }
                StreamGuard operator()(Input::Stream& strm) const {
                    return StreamGuard {
                        StreamGuard::Stream {
                            strm.stream
                        }
                    };
                }
            };
            return boost::apply_visitor(visitor(), data);
        };

    }

    namespace output {
        struct OutputStream {
            std::ostream* stream;
        };
    }

    //using input::Input;


        /*struct OutputOstreamGuard {
            boost::variant<Memory, std::ostream&>* data;
            std::stringstream ss;
            std::ostream& o;

            OutputOstreamGuard(boost::variant<Memory, std::ostream&>* x):
                data(x), o(ss)
            {

                boost::iostreams::stream_buffer<
                    boost::iostreams::back_insert_device<
                        std::vector<char>>> outBuf(buffer);
                o.rdbuf(&outBuf);
            }
        };

        struct Output {
            boost::variant<Memory, OStream> data;

            Output(Memory&& mem): data(mem) {}

            template<class F>
            void write_with(F f) {
                std::vector<char> buffer;
                std::stringstream ss;
                std::ostream& o = ss;

                boost::iostreams::stream_buffer<
                    boost::iostreams::back_insert_device<
                        std::vector<char>>> outBuf(buffer);

                o.rdbuf(&outBuf);

                f(o);
                o.flush();

                std::cout << char(buffer[0]);
            }
        };*/


/// Type of the input data to be compressed
using Input = std::vector<uint8_t>;

/// Interface for a general compressor.
struct Compressor {
    Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline Compressor() = delete;

    /// Construct the class with an environment.
    inline Compressor(Env& env_): env(env_) {}

    /// Compress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void compress(Input input, std::ostream& out) = 0;

    /// Decompress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void decompress(std::istream& inp, std::ostream& out) = 0;
};

}

#endif
