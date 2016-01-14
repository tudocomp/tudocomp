#ifndef TUDOCOMP_IO_HANDLE_H
#define TUDOCOMP_IO_HANDLE_H

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

namespace tudocomp {

namespace input {
    struct SliceGuard;
    struct StreamGuard;

    /// Represents the input of an algorithm.
    ///
    /// Can be used as either a istream or a memory buffer
    /// with the as_stream or as_view methods.
    class Input {
        struct Memory {
            const uint8_t* ptr;
            size_t size;
        };
        struct Stream {
            std::istream* stream;
        };
        struct File {
            std::string path;
        };

    public:
        boost::variant<Memory, Stream, File> data;

        static Input from_path(std::string path) {
            return Input { File { std::move(path) } };
        }

        static Input from_memory(const std::vector<uint8_t>& buf) {
            return Input { Memory { &buf[0], buf.size() } };
        }

        static Input from_memory(const boost::string_ref buf) {
            return Input { Memory { (const uint8_t*) &buf[0], buf.size() } };
        }

        static Input from_stream(std::istream& stream) {
            return Input { Stream { &stream } };
        }

        inline SliceGuard as_view();
        inline StreamGuard as_stream();

        inline bool has_size();
        inline size_t size();
    };

    struct SliceGuard {
        struct Memory {
            const uint8_t* ptr;
            const size_t size;
        };

        struct Stream {
            std::vector<uint8_t> buffer;
        };

        boost::variant<Memory, Stream> data;

        boost::string_ref operator* () {
            struct visitor: public boost::static_visitor<boost::string_ref> {
                boost::string_ref operator()(SliceGuard::Memory& mem) const {
                    return boost::string_ref {
                        (char*) mem.ptr, mem.size
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
                    SliceGuard::Memory { mem.ptr, mem.size }
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
                            (char*)mem.ptr,
                            mem.size
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

    inline bool Input::has_size() {
        struct visitor: public boost::static_visitor<bool> {
            bool operator()(Input::Memory& mem) const {
                return true;
            }
            bool operator()(Input::File& f) const {
                return true;
            }
            bool operator()(Input::Stream& strm) const {
                return false;
            }
        };
        return boost::apply_visitor(visitor(), data);
    };

    inline size_t Input::size() {
        struct visitor: public boost::static_visitor<size_t> {
            size_t operator()(Input::Memory& mem) const {
                return mem.size;
            }
            size_t operator()(Input::File& f) const {
                return read_file_size(f.path);
            }
            size_t operator()(Input::Stream& strm) const {
                return SIZE_MAX;
            }
        };
        return boost::apply_visitor(visitor(), data);
    };

}

namespace output {
    class StreamGuard;

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

        inline StreamGuard as_stream();
    };

    struct StreamGuard {
        struct Memory {
            BackInsertStream stream;
        };
        struct Stream {
            std::ostream* stream;
        };
        struct File {
            std::ofstream stream;
        };

        boost::variant<Memory, Stream, File> data;

        std::ostream& operator* () {
            struct visitor: public boost::static_visitor<std::ostream&> {
                std::ostream& operator()(StreamGuard::Memory& m) const {
                    return m.stream.stream();
                }
                std::ostream& operator()(StreamGuard::Stream& m) const {
                    return *m.stream;
                }
                std::ostream& operator()(StreamGuard::File& m) const {
                    return m.stream;
                }
            };
            return boost::apply_visitor(visitor(), data);
        }
    };

    inline StreamGuard Output::as_stream() {
        struct visitor: public boost::static_visitor<StreamGuard> {
            StreamGuard operator()(Output::Memory& mem) const {
                return StreamGuard {
                    StreamGuard::Memory {
                        BackInsertStream { *mem.buffer }
                    }
                };
            }
            StreamGuard operator()(Output::File& f) const {
                return StreamGuard {
                    StreamGuard::File {
                        std::ofstream(f.path, std::ios::out | std::ios::binary)
                    }
                };
            }
            StreamGuard operator()(Output::Stream& strm) const {
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

}

#endif
