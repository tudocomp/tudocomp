#ifndef _INCLUDED_INPUT_HPP
#define _INCLUDED_INPUT_HPP

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <tudocomp/util.h>

#include <tudocomp/io/IOUtil.hpp>
#include <tudocomp/io/ViewStream.hpp>

namespace tudocomp {
namespace io {

    struct InputView;
    struct InputStream;

    /// \brief An abstraction layer for algorithm input.
    ///
    /// This class serves as a generic abstraction over different sources of
    /// input: memory, files or streams. It provides two ways of handling the
    /// input: \e streams or \e views. While a view allows random access on all
    /// of the input, it requires the entire input to be stored in memory.
    /// Streaming, on the other hand, is used for character-wise reading
    /// without the ability to rewind (online).
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
        friend class InputStreamInternal;
        friend class InputView;

        std::unique_ptr<Variant> m_data;

    public:
        /// \brief Represents a file path.
        ///
        /// Pass a Path instance to the respective constructor in order to
        /// create an input from the file pointed to by the path.
        struct Path {
            /// The path string.
            std::string path;
        };

        /// \brief Constructs an empty input.
        inline Input():
            m_data(std::make_unique<Memory>(nullptr, 0)) {}

        /// \brief Constructs an input from another input, retaining its
        /// internal state ("cursor").
        ///
        /// Use this in combination with streams to store a rewind point.
        ///
        /// \param other The input to copy.
        inline Input(const Input& other):
            m_data(std::move(other.m_data->virtual_copy())) {}

        /// \brief Move constructor.
        inline Input(Input&& other):
            m_data(std::move(other.m_data)) {}

        /// \brief Constructs a file input reading from the file at the given
        /// path.
        ///
        /// \param path The path to the input file.
        Input(Input::Path&& path):
            m_data(std::make_unique<File>(std::move(path.path), 0)) {}

        /// \brief Constructs an input reading from the specified byte buffer.
        ///
        /// \param buf The input byte buffer.
        Input(const std::vector<uint8_t>& buf):
            m_data(std::make_unique<Memory>(&buf[0], buf.size())) {}

        /// \brief Constructs an input reading from a string in memory.
        ///
        /// \param buf The input string.
        Input(const string_ref buf):
            m_data(std::make_unique<Memory>(
                    (const uint8_t*) &buf[0], buf.size())) {}

        /// \brief Move operator.
        Input& operator=(Input&& other) {
            m_data = std::move(other.m_data);
            return *this;
        }

        /// \deprecated Use the respective constructor instead.
        /// \brief Constructs a file input reading from the file at the given
        /// path.
        ///
        /// \param path The path to the input file.
        static Input from_path(std::string path) {
            return Input(Path { path });
        }

        /// \deprecated Use the respective constructor instead.
        /// \brief Constructs a file input reading from a byte buffer.
        ///
        /// \param buf The input byte buffer.
        static Input from_memory(const std::vector<uint8_t>& buf) {
            return Input(buf);
        }

        /// \deprecated Use the respective constructor instead.
        /// \brief Constructs a file input reading from a string in memory.
        ///
        /// \param buf The input string.
        static Input from_memory(const string_ref buf) {
            return Input(buf);
        }

        /// \brief Provides a view on the input that allows for random access.
        ///
        /// This will store the entire input in memory, ie a file or stream
        /// will be fully read in order to provide the view.
        ///
        /// \return A random access view on the input.
        inline InputView as_view();

        /// \brief Creates a stream that allows for character-wise reading of
        /// the input.
        ///
        /// \return A character stream for the input.
        inline InputStream as_stream();

        /// \brief Yields the total amount of characters in the input.
        ///
        /// \return The total amount of characters in the input.
        inline size_t size() {
            return m_data->size();
        }
    };

    /// \cond INTERNAL
    class InputViewInternal {
        struct Variant {
            virtual string_ref view() = 0;
        };

        struct Memory: Variant {
            const uint8_t* ptr;
            size_t size;

            Memory(const uint8_t* ptr, size_t size) {
                this->ptr = ptr;
                this->size = size;
            }

            string_ref view() override {
                return string_ref {
                    (char*) ptr,
                    size
                };
            }
        };
        struct File: Variant {
            std::vector<uint8_t> buffer;

            File(std::vector<uint8_t>&& buffer_):
                buffer(std::move(buffer_)) {}

            string_ref view() override {
                return string_ref {
                    (char*) buffer.data(),
                    buffer.size()
                };
            }
        };

        std::unique_ptr<Variant> m_variant;

        friend class InputView;
        friend class Input;

        InputViewInternal(const InputViewInternal& other) = delete;
        InputViewInternal() = delete;

        InputViewInternal(InputViewInternal::Memory&& mem):
            m_variant(std::make_unique<Memory>(std::move(mem))) {}
        InputViewInternal(InputViewInternal::File&& s):
            m_variant(std::make_unique<File>(std::move(s))) {}
        InputViewInternal(InputViewInternal&& s):
            m_variant(std::move(s.m_variant)) {}
    };
    /// \endcond


    /// \brief Provides a view on the input that allows for random access.
    ///
    /// \sa View.
    class InputView: InputViewInternal, public View {
        friend class Input;

        InputView(InputViewInternal&& mem):
            InputViewInternal(std::move(mem)),
            View(m_variant->view()) {}
    public:
        /// Move constructor.
        InputView(InputView&& mem):
            InputViewInternal(std::move(mem)),
            View(std::move(mem)) {}

        /// Copy constructor (deleted).
        InputView(const InputView& other) = delete;

        /// Default constructor (deleted).
        InputView() = delete;
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

    /// \cond INTERNAL
    class InputStreamInternal {
        class Variant {
        public:
            virtual std::istream& stream() = 0;
            virtual ~Variant() {}
        };

        class Memory: public InputStreamInternal::Variant {
            ViewStream m_stream;

            Input::Memory* m_offset_back_ref;
            size_t m_start_pos;

            bool m_is_empty = false;

            friend class InputStreamInternal;

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
        class File: public InputStreamInternal::Variant {
            std::string m_path;
            std::unique_ptr<std::ifstream> m_stream;

            Input::File* m_offset_back_ref;
            size_t m_start_pos;

            friend class InputStreamInternal;
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

        std::unique_ptr<InputStreamInternal::Variant> m_variant;

        friend class InputStream;
        friend class Input;

        InputStreamInternal(const InputStreamInternal& other) = delete;
        InputStreamInternal() = delete;

        InputStreamInternal(InputStreamInternal::Memory&& mem):
            m_variant(std::move(std::make_unique<InputStreamInternal::Memory>(std::move(mem)))) {}
        InputStreamInternal(InputStreamInternal::File&& f):
            m_variant(std::move(std::make_unique<InputStreamInternal::File>(std::move(f)))) {}
        InputStreamInternal(InputStreamInternal&& s):
            m_variant(std::move(s.m_variant)) {}

    };
    /// \endcond

   /// \brief Provides a character stream of the underlying input.
    class InputStream: InputStreamInternal, public std::istream {
        friend class Input;

        InputStream(InputStreamInternal&& mem):
            InputStreamInternal(std::move(mem)),
            std::istream(m_variant->stream().rdbuf()) {}
    public:
        /// Move constructor.
        InputStream(InputStream&& mem):
            InputStreamInternal(std::move(mem)),
            std::istream(mem.rdbuf()) {}

        /// Copy constructor (deleted).
        InputStream(const InputStream& other) = delete;

        /// Default constructor (deleted).
        InputStream() = delete;
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

