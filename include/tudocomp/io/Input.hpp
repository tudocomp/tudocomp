#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <iterator>
#include <memory>

#include <tudocomp/util.hpp>

#include <tudocomp/io/Path.hpp>
#include <tudocomp/io/InputRestrictions.hpp>
#include <tudocomp/io/InputAlloc.hpp>
#include <tudocomp/io/IOUtil.hpp>
#include <tudocomp/io/ViewStream.hpp>

namespace tdc {namespace io {
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
    public:
        static constexpr size_t npos = -1;
    private:
        class Variant {
            InputSource m_source;
            InputAllocHandle m_handle;
            InputRestrictions m_input_restrictions;
            size_t m_from = 0;
            size_t m_to = npos;
            mutable size_t m_escaped_size_cache = npos;
        protected:
            inline void set_escaped_size(size_t size) const {
                m_escaped_size_cache = size;
            }
            inline Variant(const Variant& other,
                           size_t from,
                           size_t to): Variant(other) {
                m_from = from;
                m_to = to;
            }
            inline Variant(const Variant& other,
                           const InputRestrictions& restrictions): Variant(other) {
                m_input_restrictions = restrictions;
            }
        public:
            inline Variant(const InputSource& src): m_source(src) {}

            inline const InputAllocHandle& alloc() const {
                return m_handle;
            }

            inline const InputRestrictions& restrictions() const {
                return m_input_restrictions;
            }

            inline size_t from() const {
                return m_from;
            }

            inline size_t to() const {
                return m_to;
            }

            inline bool to_unknown() const {
                return m_to == npos;
            }

            inline bool escaped_size_unknown() const {
                return m_escaped_size_cache == npos;
            }

            inline size_t escaped_size() const {
                DCHECK(!escaped_size_unknown());
                return m_escaped_size_cache;
            }

            inline const InputSource source() const {
                return m_source;
            }

            /// Creates a slice of this Variant.
            /// The arguments `from` and `to` are relative to the current size()
            inline std::shared_ptr<Variant> slice(size_t from, size_t to) const;
            inline std::shared_ptr<Variant> restrict(const InputRestrictions& rest) const;
            inline size_t size() const;
            inline InputView as_view() const;
            inline InputStream as_stream() const;
        };

        friend class InputStream;
        friend class InputStreamInternal;
        friend class InputView;

        std::shared_ptr<Variant> m_data;
    public:
        /// \brief Constructs an empty input.
        inline Input():
            m_data(std::make_shared<Variant>(InputSource(""_v))) {}

        /// \brief Constructs an input from another input, retaining its
        /// internal state ("cursor").
        ///
        /// Use this in combination with streams to store a rewind point.
        ///
        /// \param other The input to copy.
        inline Input(const Input& other):
            m_data(other.m_data) {}

        /// \brief Move constructor.
        inline Input(Input&& other):
            m_data(std::move(other.m_data)) {}

        /// \brief Constructs a file input reading from the file at the given
        /// path.
        ///
        /// \param path The path to the input file.
        Input(Path&& path):
            m_data(std::make_shared<Variant>(InputSource(path.path))) {}

        /// \brief Constructs an input reading from a string in memory.
        ///
        /// \param buf The input string.
        Input(const string_ref& buf):
            m_data(std::make_shared<Variant>(View(buf))) {}

        /// \brief Constructs an input reading from the specified byte buffer.
        ///
        /// \param buf The input byte buffer.
        Input(const std::vector<uint8_t>& buf):
            Input(string_ref(buf)) {}

        /// \brief Constructs an input reading from a stream.
        ///
        /// The stream will still be buffered in memory.
        ///
        /// \param buf The input string.
        Input(std::istream& stream):
            m_data(std::make_shared<Variant>(InputSource(&stream))) {}

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
        inline InputView as_view() const;

        /// \brief Creates a stream that allows for character-wise reading of
        /// the input.
        ///
        /// \return A character stream for the input.
        inline InputStream as_stream() const;

        /// \brief Yields the total amount of characters in the input.
        ///
        /// \return The total amount of characters in the input.
        inline size_t size() const {
            return m_data->size();
        }

        /// \cond INTERNAL
        /// Slice constructor
        inline Input(const Input& other, size_t from, size_t to = npos):
            m_data(other.m_data->slice(from, to)) {}

        /// Restrict constructor
        inline Input(const Input& other, const InputRestrictions& restrictions):
            m_data(other.m_data->restrict(restrictions)) {}
        /// \endcond
    };

}}

#include <tudocomp/io/InputView.hpp>
#include <tudocomp/io/InputStream.hpp>
#include <tudocomp/io/InputSize.hpp>

namespace tdc {namespace io {
    inline InputView Input::as_view() const {
        return m_data->as_view();
    }

    inline InputStream Input::as_stream() const {
        return m_data->as_stream();
    }
}}
