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

#include <tudocomp/util.h>

#include <tudocomp/io/IOUtil.hpp>
#include <tudocomp/io/ViewStream.hpp>

namespace tudocomp {
namespace io {

    struct InputView;
    struct InputStream;

    /// Represents the input of an algorithm.
    ///
    /// Can be used as either a istream or a memory buffer
    /// with the as_stream or as_view methods.
    class Input {
        struct Memory {
            const uint8_t* ptr;
            size_t size;
        };
        struct File {
            std::string path;
            size_t offset;
        };

        friend class InputStream;
        friend class InputView;

        using Variant = boost::variant<Memory, File>;
        std::unique_ptr<Variant> m_data;

    public:
        struct Path { std::string path; };

        /// An empty Input
        inline Input():
            m_data(std::make_unique<Variant>(Memory { nullptr, 0 })) {}

        /// Create an copy of the Input, with internal cursors
        /// set to the current position
        inline Input(const Input& other):
            m_data(std::make_unique<Variant>(*other.m_data)) {}

        /// Move constructor
        inline Input(Input&& other):
            m_data(std::move(other.m_data)) {}

        /// An Input referring to the contents of a file
        Input(Input::Path&& path):
            m_data(std::make_unique<Variant>(File { std::move(path.path), 0 })) {}

        /// An Input referring to the contents of an vector
        Input(const std::vector<uint8_t>& buf):
            m_data(std::make_unique<Variant>(Memory { &buf[0], buf.size() })) {}

        /// An Input referring to the contents of an string
        Input(const boost::string_ref buf):
            m_data(std::make_unique<Variant>(
                    Memory { (const uint8_t*) &buf[0], buf.size() })) {}

        Input& operator=(Input&& other) {
            m_data = std::move(other.m_data);
            return *this;
        }

        /// DEPRECATED
        static Input from_path(std::string path) {
            return Input(Path { path });
        }

        /// DEPRECATED
        static Input from_memory(const std::vector<uint8_t>& buf) {
            return Input(buf);
        }

        /// DEPRECATED
        static Input from_memory(const boost::string_ref buf) {
            return Input(buf);
        }

        inline InputView as_view();
        inline InputStream as_stream();

        inline size_t size();
    };

    struct InputView {
        struct Memory {
            const uint8_t* ptr;
            const size_t size;
        };
        struct File {
            std::vector<uint8_t> buffer;
        };

        boost::variant<Memory, File> data;

        boost::string_ref operator* () {
            struct visitor: public boost::static_visitor<boost::string_ref> {
                boost::string_ref operator()(InputView::Memory& mem) const {
                    return boost::string_ref {
                        (char*) mem.ptr, mem.size
                    };
                }
                boost::string_ref operator()(InputView::File& mem) const {
                    return boost::string_ref {
                        (char*) mem.buffer.data(), mem.buffer.size()
                    };
                }
            };
            return boost::apply_visitor(visitor(), data);
        }

        const uint8_t* mem_ptr() {
            struct visitor: public boost::static_visitor<const uint8_t*> {
                const uint8_t* operator()(InputView::Memory& mem) const {
                    return mem.ptr;
                }
                const uint8_t* operator()(InputView::File& mem) const {
                    return mem.buffer.data();
                }
            };
            return boost::apply_visitor(visitor(), data);
        }

        size_t size() {
            struct visitor: public boost::static_visitor<size_t> {
                size_t operator()(InputView::Memory& mem) const {
                    return mem.size;
                }
                size_t operator()(InputView::File& mem) const {
                    return mem.buffer.size();
                }
            };
            return boost::apply_visitor(visitor(), data);
        }

        InputView(const InputView* other) = delete;
        InputView() = delete;

        InputView(InputView::Memory&& mem): data(std::move(mem)) {}
        InputView(InputView::File&& s): data(std::move(s)) {}

    };

    inline InputView Input::as_view() {
        struct visitor: public boost::static_visitor<InputView> {
            InputView operator()(Input::Memory& mem) const {
                Input::Memory mem2 = mem;

                // advance view into memory by its whole length
                mem.ptr += mem.size;
                mem.size = 0;

                return InputView {
                    InputView::Memory { mem2.ptr, mem2.size }
                };
            }
            InputView operator()(Input::File& mem) const {
                // read file into buffer starting at current offset
                auto buf = read_file_to_stl_byte_container<
                    std::vector<uint8_t>>(mem.path, mem.offset);

                // We read the whole file, so skip it on next read.
                mem.offset += buf.size();

                return InputView {
                    InputView::File {
                        std::move(buf)
                    }
                };
            }
        };

        return boost::apply_visitor(visitor(), *m_data);
    };

    struct InputStream {
        class Memory {
            ViewStream m_stream;

            Input::Memory* m_offset_back_ref;
            size_t m_start_pos;

            friend class InputStream;
        public:
            Memory(ViewStream&& stream, Input::Memory* offset_back_ref):
                m_stream(std::move(stream))
            {
                m_offset_back_ref = offset_back_ref;
                m_start_pos = m_stream.stream().tellg();
            }
            ~Memory() {
                size_t len = size_t(m_stream.stream().tellg()) - m_start_pos;
                m_offset_back_ref->ptr += len;
                m_offset_back_ref->size -= len;
            }
        };
        class File {
            std::string m_path;
            std::unique_ptr<std::ifstream> stream;

            Input::File* m_offset_back_ref;
            size_t m_start_pos;

            File(const File& other) = delete;
            File() = delete;

            friend class InputStream;
        public:
            File(std::string&& path,
                 Input::File* offset_back_ref,
                 size_t offset)
            {
                m_path = path;
                stream = std::make_unique<std::ifstream>(
                    m_path, std::ios::in | std::ios::binary);
                stream->seekg(offset, std::ios::beg);
                m_start_pos = stream->tellg();
                m_offset_back_ref = offset_back_ref;
            }

            File(File&& other) {
                stream = std::move(other.stream);
                m_offset_back_ref = other.m_offset_back_ref;
                m_path = other.m_path;
                m_start_pos = other.m_start_pos;
            }

            ~File() {
                if (stream != nullptr) {
                    auto len = size_t(stream->tellg()) - m_start_pos;
                    m_offset_back_ref->offset += len;
                }
            }
        };

        boost::variant<Memory, File> data;

        std::istream& operator* () {
            struct visitor: public boost::static_visitor<std::istream&> {
                std::istream& operator()(InputStream::Memory& m) const {
                    return m.m_stream.stream();
                }
                std::istream& operator()(InputStream::File& m) const {
                    return *m.stream;
                }
            };
            return boost::apply_visitor(visitor(), data);
        }

        InputStream(const InputStream* other) = delete;
        InputStream() = delete;

        InputStream(InputStream::Memory&& mem): data(std::move(mem)) {}
        InputStream(InputStream::File&& f): data(std::move(f)) {}
    };

    inline InputStream Input::as_stream() {
        struct visitor: public boost::static_visitor<InputStream> {
            InputStream operator()(Input::Memory& mem) const {
                return InputStream {
                    InputStream::Memory {
                        ViewStream {
                            (char*)mem.ptr,
                            mem.size
                        },
                        &mem
                    }
                };
            }
            InputStream operator()(Input::File& f) const {
                return InputStream {
                    InputStream::File {
                        std::string(f.path),
                        &f,
                        f.offset,
                    }
                };
            }
        };
        return boost::apply_visitor(visitor(), *m_data);
    };

    inline size_t Input::size() {
        struct visitor: public boost::static_visitor<size_t> {
            size_t operator()(Input::Memory& mem) const {
                return mem.size;
            }
            size_t operator()(Input::File& f) const {
                return read_file_size(f.path) - f.offset;
            }
        };
        return boost::apply_visitor(visitor(), *m_data);
    };

}}

#endif

