#ifndef _INCLUDED_INPUT_HPP
#define _INCLUDED_INPUT_HPP

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <iterator>

#include <tudocomp/util.hpp>

#include <tudocomp/io/IOUtil.hpp>
#include <tudocomp/io/ViewStream.hpp>

namespace tdc {
namespace io {

    class InputView;
    class InputStream;

    /// \brief An abstraction layer for algorithm input.
    ///
    /// This class serves as a generic abstraction over different sources of
    /// input: memory buffers, files or streams. It provides two ways of
    /// handling the input: \e streams or \e views. While a view allows random
    /// access on all of the input, it requires the entire input to be stored
    /// in memory. Streaming, on the other hand, is used for character-wise
    /// reading without the ability to rewind (online).
    class Input {
        struct Variant {
            virtual ~Variant() {}
            virtual std::unique_ptr<Variant> virtual_copy() = 0;
            virtual InputView as_view() = 0;
            virtual InputStream as_stream() = 0;
            virtual size_t size() = 0;
            virtual bool has_hidden_null_terminator() = 0;
        };

        struct Memory: Variant {
            View m_view;
            std::shared_ptr<std::vector<uint8_t>> m_owned;

            Memory(View view):
                m_view(view) {}

            Memory(View view, std::shared_ptr<std::vector<uint8_t>> owned):
                m_view(view),
                m_owned(owned) {}

            Memory(const Memory& other):
                m_view(other.m_view),
                m_owned(other.m_owned) {}

            std::unique_ptr<Variant> virtual_copy() override {
                return std::make_unique<Memory>(*this);
            }

            inline InputView as_view() override;
            inline InputStream as_stream() override;
            inline size_t size() override;
            inline bool has_hidden_null_terminator() override {
                // The slice has a hidden null character after its end
                // if it points into m_owned and ends one before its end
                return m_owned && ((m_view.data() + m_view.size()) == &m_owned->back());
            }
        };
        struct File: Variant {
            std::string path;
            size_t offset;

            File(std::string path, size_t offset) {
                this->path = path;
                this->offset = offset;
            }

            File(const File& other):
                path(other.path),
                offset(other.offset) {}

            std::unique_ptr<Variant> virtual_copy() override {
                return std::make_unique<File>(*this);
            }

            inline InputView as_view() override;
            inline InputStream as_stream() override;
            inline size_t size() override;
            inline bool has_hidden_null_terminator() override {
                return false;
            }
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
            m_data(std::make_unique<Memory>(""_v)) {}

        /// \brief Constructs an input from another input, retaining its
        /// internal state ("cursor").
        ///
        /// Use this in combination with streams to store a rewind point.
        ///
        /// \param other The input to copy.
        inline Input(const Input& other):
            m_data(other.m_data->virtual_copy()) {}

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
            m_data(std::make_unique<Memory>(buf)) {}

        /// \brief Constructs an input reading from a string in memory.
        ///
        /// \param buf The input string.
        Input(const string_ref buf):
            m_data(std::make_unique<Memory>(buf)) {}

        /// \brief Constructs an input reading from a stream.
        ///
        /// The stream will still be buffered in memory.
        ///
        /// \param buf The input string.
        Input(std::istream& stream) {
            auto buf = io::read_stream_to_stl_byte_container<
                std::vector<uint8_t>>(stream);
            buf.push_back(0);

            View data(buf.data(), buf.size() - 1);
            auto owned = std::make_shared<std::vector<uint8_t>>(std::move(buf));

            m_data = std::make_unique<Memory>(data, owned);
        }

        /// \brief Move assignment operator.
        Input& operator=(Input&& other) {
            m_data = std::move(other.m_data);
            return *this;
        }

        /// \brief Copy assignment operator.
        Input& operator=(const Input& other) {
            Input cpy(other);
            *this = std::move(cpy);
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
            inline virtual ~Variant() {}
            virtual View view() = 0;
            virtual void ensure_null_terminator() = 0;
        };

        struct Memory: Variant {
            View m_view;
            bool m_view_has_hidden_null_terminator;
            std::unique_ptr<std::vector<uint8_t>> m_owned;

            inline Memory(View view, bool has_hidden_null_terminator = false):
                m_view(view),
                m_view_has_hidden_null_terminator(has_hidden_null_terminator) {}

            inline View view() override {
                return m_view;
            }

            inline virtual void ensure_null_terminator() override {
                if (m_view_has_hidden_null_terminator) {
                    m_view = View(m_view.data(), m_view.size() + 1);
                } else if ((m_view.size() == 0) || (m_view.back() != 0)) {
                    m_owned = std::make_unique<std::vector<uint8_t>>();
                    auto& v = *m_owned;
                    v.reserve(m_view.size() + 1);
                    for (uint8_t byte : m_view) {
                        v.push_back(byte);
                    }
                    v.push_back(0);
                    m_view = *m_owned;
                }
            }
        };
        struct File: Variant {
            std::vector<uint8_t> buffer;
            size_t m_null_skip = 1;

            inline File(std::vector<uint8_t>&& buffer_):
                buffer(std::move(buffer_)) {}

            inline View view() override {
                return View {
                    buffer.data(),
                    buffer.size() - m_null_skip
                };
            }

            inline virtual void ensure_null_terminator() override {
                m_null_skip = 0;
            }
        };

        std::unique_ptr<Variant> m_variant;

        friend class InputView;
        friend class Input;

        inline InputViewInternal(const InputViewInternal& other) = delete;
        inline InputViewInternal() = delete;

        inline InputViewInternal(InputViewInternal::Memory&& mem):
            m_variant(std::make_unique<Memory>(std::move(mem))) {}
        inline InputViewInternal(InputViewInternal::File&& s):
            m_variant(std::make_unique<File>(std::move(s))) {}
        inline InputViewInternal(InputViewInternal&& s):
            m_variant(std::move(s.m_variant)) {}
    };
    /// \endcond


    /// \brief Provides a view on the input that allows for random access.
    ///
    /// \sa View.
    class InputView: InputViewInternal, public View {
        friend class Input;

        inline InputView(InputViewInternal&& mem):
            InputViewInternal(std::move(mem)),
            View(m_variant->view()) {}
    public:
        /// Move constructor.
        inline InputView(InputView&& mem):
            InputViewInternal(std::move(mem)),
            View(std::move(mem)) {}

        /// Copy constructor (deleted).
        inline InputView(const InputView& other) = delete;

        /// Default constructor (deleted).
        inline InputView() = delete;

        /// Ensures that that this View is terminated with a null byte.
        ///
        /// Depending on internal Input source, this might involve
        /// a (re)allocation of a buffer for the input data.
        inline void ensure_null_terminator() {
            m_variant->ensure_null_terminator();
            (View&) *this = m_variant->view();
        }
    };

    inline InputView Input::Memory::as_view() {
        Input::Memory mem2 = *this;

        // advance view into memory by its whole length
        m_view = m_view.substr(m_view.size());

        return InputView {
            InputView::Memory { mem2.m_view }
        };
    }

    inline InputView Input::File::as_view() {
        // read file into buffer starting at current offset
        auto buf = read_file_to_stl_byte_container<
            std::vector<uint8_t>>(path, offset, true);

        // We read the whole file, so skip it on next read.
        // `size - 1` because of the null terminator that
        // does not exist in the file.
        offset += buf.size() - 1;

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
                    m_offset_back_ref->m_view = m_offset_back_ref->m_view.substr(len);
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

            File(std::string&& path, Input::File* offset_back_ref, size_t offset):
                m_path(std::move(path)),
                m_stream(std::make_unique<std::ifstream>(
                    m_path, std::ios::in | std::ios::binary))
            {
                auto& s = *m_stream;
                s.seekg(offset, std::ios::beg);
                m_start_pos = s.tellg();
                m_offset_back_ref = offset_back_ref;
            }

            File(File&& other) {
                m_path = std::move(other.m_path);
                m_stream = std::move(other.m_stream);
                m_offset_back_ref = other.m_offset_back_ref;
                m_start_pos = other.m_start_pos;
            }

            std::istream& stream() override {
                return *m_stream;
            }

            virtual ~File() {
                if (m_stream) {
                    auto len = size_t(stream().tellg()) - m_start_pos;
                    m_offset_back_ref->offset += len;
                }
            }

        };

        std::unique_ptr<InputStreamInternal::Variant> m_variant;

        friend class InputStream;
        friend class Input;

        inline InputStreamInternal(const InputStreamInternal& other) = delete;
        inline InputStreamInternal() = delete;

        inline InputStreamInternal(InputStreamInternal::Memory&& mem):
            m_variant(std::make_unique<InputStreamInternal::Memory>(std::move(mem))) {}
        inline InputStreamInternal(InputStreamInternal::File&& f):
            m_variant(std::make_unique<InputStreamInternal::File>(std::move(f))) {}
        inline InputStreamInternal(InputStreamInternal&& s):
            m_variant(std::move(s.m_variant)) {}

    };
    /// \endcond

   /// \brief Provides a character stream of the underlying input.
    class InputStream: InputStreamInternal, public std::istream {
        friend class Input;

        inline InputStream(InputStreamInternal&& mem):
            InputStreamInternal(std::move(mem)),
            std::istream(m_variant->stream().rdbuf()) {}
    public:
        /// Move constructor.
        inline InputStream(InputStream&& mem):
            InputStreamInternal(std::move(mem)),
            std::istream(mem.rdbuf()) {}

        /// Copy constructor (deleted).
        inline InputStream(const InputStream& other) = delete;

        /// Default constructor (deleted).
        inline InputStream() = delete;

        using iterator = std::istreambuf_iterator<char>;
        inline iterator begin() {
            return iterator(*this);
        }
        inline iterator end() {
            return iterator();
        }
    };

    inline InputStream Input::Memory::as_stream() {
        return InputStream {
            InputStream::Memory {
                ViewStream {
                    (char*) m_view.data(),
                    m_view.size()
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
                offset
            }
        };
    }

    inline InputStream Input::as_stream() {
        return m_data->as_stream();
    }

    inline size_t Input::Memory::size() {
        return m_view.size();
    }

    inline size_t Input::File::size() {
        return read_file_size(path) - offset;
    }

}}

#endif

