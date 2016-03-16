#ifndef _INCLUDED_INPUT_HPP
#define _INCLUDED_INPUT_HPP

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/utility/string_ref.hpp>
#include <boost/variant.hpp>

#include <tudocomp/io/IOUtil.hpp>
#include <tudocomp/io/ViewStream.hpp>

namespace tudocomp {
namespace io {

    struct InputSliceGuard;
    struct InputStreamGuard;

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

        std::shared_ptr<InputSliceGuard> m_view;

    public:
        boost::variant<Memory, Stream, File> data;

        static Input from_path(std::string path) {
            return Input ( File { std::move(path) } );
        }

        static Input from_memory(const std::vector<uint8_t>& buf) {
            return Input ( Memory { &buf[0], buf.size() } );
        }

        static Input from_memory(const boost::string_ref buf) {
            return Input ( Memory { (const uint8_t*) &buf[0], buf.size() } );
        }

        static Input from_stream(std::istream& stream) {
            return Input ( Stream { &stream } );
        }

        inline Input() {
        }

        inline Input(boost::variant<Memory, Stream, File> _data) : data(_data) {
        }

        inline InputSliceGuard& as_view();
        inline InputStreamGuard as_stream();

        inline bool has_size();
        inline size_t size();
    };

    struct InputSliceGuard {
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
                boost::string_ref operator()(InputSliceGuard::Memory& mem) const {
                    return boost::string_ref {
                        (char*) mem.ptr, mem.size
                    };
                }
                boost::string_ref operator()(InputSliceGuard::Stream& mem) const {
                    return boost::string_ref {
                        (char*) mem.buffer.data(), mem.buffer.size()
                    };
                }
            };
            return boost::apply_visitor(visitor(), data);
        }
        
        const uint8_t* mem_ptr() {
            struct visitor: public boost::static_visitor<const uint8_t*> {
                const uint8_t* operator()(InputSliceGuard::Memory& mem) const {
                    return mem.ptr;
                }
                const uint8_t* operator()(InputSliceGuard::Stream& mem) const {
                    return mem.buffer.data();
                }
            };
            return boost::apply_visitor(visitor(), data);
        }

        size_t size() {
            struct visitor: public boost::static_visitor<size_t> {
                size_t operator()(InputSliceGuard::Memory& mem) const {
                    return mem.size;
                }
                size_t operator()(InputSliceGuard::Stream& mem) const {
                    return mem.buffer.size();
                }
            };
            return boost::apply_visitor(visitor(), data);
        }
    };

    inline InputSliceGuard& Input::as_view() {
        if(!m_view) {
            struct visitor: public boost::static_visitor<InputSliceGuard*> {
                InputSliceGuard* operator()(Input::Memory& mem) const {
                    return new InputSliceGuard {
                        InputSliceGuard::Memory { mem.ptr, mem.size }
                    };
                }
                InputSliceGuard* operator()(Input::File& mem) const {
                    return new InputSliceGuard {
                        InputSliceGuard::Stream {
                            read_file_to_stl_byte_container<
                                std::vector<uint8_t>>(mem.path)
                        }
                    };
                }
                InputSliceGuard* operator()(Input::Stream& strm) const {
                    return new InputSliceGuard {
                        InputSliceGuard::Stream {
                            read_stream_to_stl_byte_container<
                                std::vector<uint8_t>>(*strm.stream)
                        }
                    };
                }
            };
            m_view = std::shared_ptr<InputSliceGuard>(boost::apply_visitor(visitor(), data));
        }
        return *m_view;
    };

    struct InputStreamGuard {
        struct Memory {
            ViewStream stream;
        };
        struct Stream {
            std::istream* stream;
        };
        struct File {
            std::string m_path;
            std::unique_ptr<std::ifstream> stream;

            File(const File& other): File(std::string(other.m_path)) {
            }

            File(std::string&& path) {
                m_path = path;
                stream = std::unique_ptr<std::ifstream> {
                    new std::ifstream(m_path, std::ios::in | std::ios::binary)
                };
            }

            File(std::unique_ptr<std::ifstream>&& s) {
                stream = std::move(s);
            }

            File(File&& other) {
                stream = std::move(other.stream);
            }
        };

        boost::variant<Memory, Stream, File> data;

        std::istream& operator* () {
            struct visitor: public boost::static_visitor<std::istream&> {
                std::istream& operator()(InputStreamGuard::Memory& m) const {
                    return m.stream.stream();
                }
                std::istream& operator()(InputStreamGuard::Stream& m) const {
                    return *m.stream;
                }
                std::istream& operator()(InputStreamGuard::File& m) const {
                    return *m.stream;
                }
            };
            return boost::apply_visitor(visitor(), data);
        }
    };

    inline InputStreamGuard Input::as_stream() {
        if(m_view) {
            //input was already fully loaded, treat as memory
            struct visitor: public boost::static_visitor<InputStreamGuard> {
                InputStreamGuard operator()(InputSliceGuard::Memory& mem) const {
                    return InputStreamGuard {
                        InputStreamGuard::Memory {
                            ViewStream { (char*)mem.ptr, mem.size }
                        }
                    };
                }
                InputStreamGuard operator()(InputSliceGuard::Stream& mem) const {
                    return InputStreamGuard {
                        InputStreamGuard::Memory {
                            ViewStream { (char*) mem.buffer.data(), mem.buffer.size() }
                        }
                    };
                }
            };
            return boost::apply_visitor(visitor(), m_view->data);
        } else {
            struct visitor: public boost::static_visitor<InputStreamGuard> {
                InputStreamGuard operator()(Input::Memory& mem) const {
                    return InputStreamGuard {
                        InputStreamGuard::Memory {
                            ViewStream {
                                (char*)mem.ptr,
                                mem.size
                            }
                        }
                    };
                }
                InputStreamGuard operator()(Input::File& f) const {
                    return InputStreamGuard {
                        InputStreamGuard::File {
                            std::string(f.path)
                        }
                    };
                }
                InputStreamGuard operator()(Input::Stream& strm) const {
                    return InputStreamGuard {
                        InputStreamGuard::Stream {
                            strm.stream
                        }
                    };
                }
            };
            return boost::apply_visitor(visitor(), data);
        }
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

}}

#endif

