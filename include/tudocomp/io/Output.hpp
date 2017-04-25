#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <tudocomp/io/Path.hpp>
#include <tudocomp/io/InputRestrictions.hpp>

namespace tdc {
namespace io {
    class OutputStream;

    /// \brief An abstraction layer for algorithm output.
    ///
    /// This class serves as a generic abstraction over different output sinks:
    /// memory, files or streams. Output is generally done in a stream, ie it
    /// is written to the sink character by character.
    class Output {
        class Variant {
            InputRestrictions m_restrictions;
        protected:
            inline Variant(const InputRestrictions& restrictions):
                m_restrictions(restrictions) {}
        public:
            inline Variant() {}

            inline const InputRestrictions& restrictions() const {
                return m_restrictions;
            }

            virtual ~Variant() {}
            virtual std::unique_ptr<Variant> unrestrict(const InputRestrictions& rest) const = 0;
            virtual OutputStream as_stream() const = 0;
        };

        class Memory: public Variant {
            std::vector<uint8_t>* m_buffer;

            Memory(const Memory& other, const InputRestrictions& r):
                Variant(r),
                m_buffer(other.m_buffer) {}
        public:
            Memory(std::vector<uint8_t>* buffer):
                   m_buffer(buffer) {}

            inline std::unique_ptr<Variant> unrestrict(const InputRestrictions& rest) const override {
                return std::make_unique<Memory>(
                    Memory(*this, restrictions() | rest));
            }
            inline OutputStream as_stream() const override;
        };
        class File: public Variant {
            std::string m_path;
            // TODO:
            mutable bool m_overwrite;

            File(const File& other, const InputRestrictions& r):
                Variant(r),
                m_path(other.m_path),
                m_overwrite(other.m_overwrite) {}
        public:
            File(const std::string& path, bool overwrite):
                m_path(path),
                m_overwrite(overwrite) {}

            inline std::unique_ptr<Variant> unrestrict(const InputRestrictions& rest) const override {
                return std::make_unique<File>(
                    File(*this, restrictions() | rest));
            }
            inline OutputStream as_stream() const override;
        };
        class Stream: public Variant {
            std::ostream* m_stream;

            Stream(const Stream& other, const InputRestrictions& r):
                Variant(r),
                m_stream(other.m_stream) {}
        public:
            Stream(std::ostream* stream):
                   m_stream(stream) {}

            inline std::unique_ptr<Variant> unrestrict(const InputRestrictions& rest) const override {
                return std::make_unique<Stream>(
                    Stream(*this, restrictions() | rest));
            }
            inline OutputStream as_stream() const override;
        };

        std::unique_ptr<Variant> m_data;

        friend class OutputStream;
    public:
        /// \brief Constructs an output to \c stdout.
        inline Output(): Output(std::cout) {}

        /// \brief Move constructor.
        inline Output(Output&& other):
            m_data(std::move(other.m_data)) {}

        /// \brief Constructs an output that appends to the file at the given
        /// path.
        ///
        /// \param path The path to the output file.
        /// \param overwrite If \c true, the file will be overwritten in case
        /// it already exists, otherwise the output will be appended to it.
        inline Output(const Path& path, bool overwrite=false):
            m_data(std::make_unique<File>(std::move(path.path), overwrite)) {}

        /// \brief Constructs an output that appends to the byte vector.
        ///
        /// \param buf The byte buffer to write to.
        inline Output(std::vector<uint8_t>& buf):
            m_data(std::make_unique<Memory>(&buf)) {}

        /// \brief Constructs an output that appends to the output stream.
        ///
        /// \param stream The stream to write to.
        inline Output(std::ostream& stream):
            m_data(std::make_unique<Stream>(&stream)) {}

        /// \brief Move assignment operator.
        inline Output& operator=(Output&& other) {
            m_data = std::move(other.m_data);
            return *this;
        }

        /// \deprecated Use the respective constructor instead.
        /// \brief Constructs a file output writing to the file at the given
        /// path.
        ///
        /// \param path The path to the input file.
        /// \param overwrite If \c true, the file will be overwritten in case
        /// it already exists, otherwise the output will be appended to it.
        inline static Output from_path(const Path& path, bool overwrite=false) {
            return Output(path, overwrite);
        }

        /// \deprecated Use the respective constructor instead.
        /// \brief Constructs an output to a byte buffer.
        ///
        /// \param buf The byte buffer to write to.
        inline static Output from_memory(std::vector<uint8_t>& buf) {
            return Output(buf);
        }

        /// \deprecated Use the respective constructor instead.
        /// \brief Constructs an output to a stream.
        ///
        /// \param stream The stream to write to.
        inline static Output from_stream(std::ostream& stream) {
            return Output(stream);
        }

        /// \brief Creates a stream that allows for character-wise output.
        inline OutputStream as_stream() const;

        /// \cond INTERNAL
        /// Unrestrict constructor
        inline Output(const Output& other, const InputRestrictions& restrictions):
            m_data(other.m_data->unrestrict(restrictions)) {}
        /// \endcond
    };

}}

#include <tudocomp/io/OutputStream.hpp>

