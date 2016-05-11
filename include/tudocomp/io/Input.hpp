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
        struct Variant {
            virtual std::unique_ptr<Variant> virtual_copy() = 0;
            virtual InputView as_view() = 0;
            virtual InputStream as_stream() = 0;
            virtual size_t size() = 0;
        };

        struct Memory: Variant {
            const uint8_t* ptr;
            size_t m_size;

            Memory(const uint8_t* ptr, size_t size): m_size(size) {
                this->ptr = ptr;
            }

            std::unique_ptr<Variant> virtual_copy() override {
                return std::make_unique<Memory>(ptr, m_size);
            }

            inline InputView as_view() override;
            inline InputStream as_stream() override;
            inline size_t size() override;
        };
        struct File: Variant {
            std::string path;
            size_t offset;

            File(std::string path, size_t offset) {
                this->path = path;
                this->offset = offset;
            }

            std::unique_ptr<Variant> virtual_copy() override {
                return std::make_unique<File>(path, offset);
            }

            inline InputView as_view() override;
            inline InputStream as_stream() override;
            inline size_t size() override;
        };

        friend class InputStream;
        friend class InputView;

        std::unique_ptr<Variant> m_data;

    public:
        struct Path { std::string path; };

        /// An empty Input
        inline Input():
            m_data(std::make_unique<Memory>(nullptr, 0)) {}

        /// Create an copy of the Input, with internal cursors
        /// set to the current position
        inline Input(const Input& other):
            m_data(std::move(other.m_data->virtual_copy())) {}

        /// Move constructor
        inline Input(Input&& other):
            m_data(std::move(other.m_data)) {}

        /// An Input referring to the contents of a file
        Input(Input::Path&& path):
            m_data(std::make_unique<File>(std::move(path.path), 0)) {}

        /// An Input referring to the contents of an vector
        Input(const std::vector<uint8_t>& buf):
            m_data(std::make_unique<Memory>(&buf[0], buf.size())) {}

        /// An Input referring to the contents of an string
        Input(const boost::string_ref buf):
            m_data(std::make_unique<Memory>(
                    (const uint8_t*) &buf[0], buf.size())) {}

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

        inline size_t size() {
            return m_data->size();
        }
    };

    struct InputView {
        struct Variant {
            virtual boost::string_ref view() = 0;
        };

        struct Memory: Variant {
            const uint8_t* ptr;
            size_t size;

            Memory(const uint8_t* ptr, size_t size) {
                this->ptr = ptr;
                this->size = size;
            }

            boost::string_ref view() override {
                return boost::string_ref {
                    (char*) ptr,
                    size
                };
            }
        };
        struct File: Variant {
            std::vector<uint8_t> buffer;

            File(std::vector<uint8_t>&& buffer_):
                buffer(std::move(buffer_)) {}

            boost::string_ref view() override {
                return boost::string_ref {
                    (char*) buffer.data(),
                    buffer.size()
                };
            }
        };

        std::unique_ptr<Variant> m_data;

        boost::string_ref operator* () {
            return m_data->view();
        }

        size_t size() {
            return m_data->view().size();
        }

        InputView(const InputView* other) = delete;
        InputView() = delete;

        InputView(InputView::Memory&& mem):
            m_data(std::make_unique<Memory>(std::move(mem))) {}
        InputView(InputView::File&& s):
            m_data(std::make_unique<File>(std::move(s))) {}

    };

    inline InputView Input::Memory::as_view() {
        Input::Memory mem2 = *this;

        // advance view into memory by its whole length
        ptr += m_size;
        m_size = 0;

        return InputView {
            InputView::Memory { mem2.ptr, mem2.m_size }
        };
    }

    inline InputView Input::File::as_view() {
        // read file into buffer starting at current offset
        auto buf = read_file_to_stl_byte_container<
            std::vector<uint8_t>>(path, offset);

        // We read the whole file, so skip it on next read.
        offset += buf.size();

        return InputView {
            InputView::File {
                std::move(buf)
            }
        };
    }

    inline InputView Input::as_view() {
        return m_data->as_view();
    }

    struct InputStream {
        class Variant {
        public:
            virtual std::istream& stream() = 0;
            virtual ~Variant() {}
        };

        class Memory: public InputStream::Variant {
            ViewStream m_stream;

            Input::Memory* m_offset_back_ref;
            size_t m_start_pos;

            bool m_is_empty = false;

            friend class InputStream;

        public:
            Memory(const Memory& other) = delete;
            Memory() = delete;

            Memory(Memory&& other):
                m_stream(std::move(other.m_stream)),
                m_offset_back_ref(other.m_offset_back_ref),
                m_start_pos(other.m_start_pos) {
                other.m_is_empty = true;
            }

            Memory(ViewStream&& stream, Input::Memory* offset_back_ref):
                m_stream(std::move(stream))
            {
                m_offset_back_ref = offset_back_ref;
                m_start_pos = m_stream.stream().tellg();
            }
            virtual ~Memory() {
                if (!m_is_empty) {
                    size_t len = size_t(m_stream.stream().tellg()) - m_start_pos;
                    m_offset_back_ref->ptr += len;
                    m_offset_back_ref->m_size -= len;
                }
            }
            std::istream& stream() override {
                return m_stream.stream();
            }
        };
        class File: public InputStream::Variant {
            std::string m_path;
            std::unique_ptr<std::ifstream> m_stream;

            Input::File* m_offset_back_ref;
            size_t m_start_pos;

            friend class InputStream;
        public:
            File(const File& other) = delete;
            File() = delete;

            File(std::string&& path,
                 Input::File* offset_back_ref,
                 size_t offset)
            {
                m_path = path;
                m_stream = std::make_unique<std::ifstream>(
                    m_path, std::ios::in | std::ios::binary);
                m_stream->seekg(offset, std::ios::beg);
                m_start_pos = m_stream->tellg();
                m_offset_back_ref = offset_back_ref;
            }

            File(File&& other) {
                m_stream = std::move(other.m_stream);
                m_offset_back_ref = other.m_offset_back_ref;
                m_path = other.m_path;
                m_start_pos = other.m_start_pos;
            }

            virtual ~File() {
                if (m_stream != nullptr) {
                    auto len = size_t(m_stream->tellg()) - m_start_pos;
                    m_offset_back_ref->offset += len;
                }
            }

            std::istream& stream() override {
                return *m_stream;
            }
        };

        std::unique_ptr<InputStream::Variant> m_data;

        std::istream& operator* () {
            return m_data->stream();
        }

        InputStream(const InputStream* other) = delete;
        InputStream() = delete;

        InputStream(InputStream::Memory&& mem):
            m_data(std::move(std::make_unique<InputStream::Memory>(std::move(mem)))) {}
        InputStream(InputStream::File&& f):
            m_data(std::move(std::make_unique<InputStream::File>(std::move(f)))) {}
    };

    inline InputStream Input::Memory::as_stream() {
        return InputStream {
            InputStream::Memory {
                ViewStream {
                    (char*)ptr,
                    m_size
                },
                this
            }
        };
    }

    inline InputStream Input::File::as_stream() {
        return InputStream {
            InputStream::File {
                std::string(path),
                this,
                offset,
            }
        };
    }

    inline InputStream Input::as_stream() {
        return m_data->as_stream();
    }

    inline size_t Input::Memory::size() {
        return m_size;
    }

    inline size_t Input::File::size() {
        return read_file_size(path) - offset;
    }

}}

#endif

