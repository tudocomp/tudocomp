#ifndef _INCLUDED_OUTPUT_HPP
#define _INCLUDED_OUTPUT_HPP

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <tudocomp/io/BackInsertStream.hpp>

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
        public:
            virtual ~Variant() {}
            virtual OutputStream as_stream() = 0;
        };

        class Memory: public Variant {
            std::vector<uint8_t>* m_buffer;

        public:
            Memory(std::vector<uint8_t>* buffer): m_buffer(buffer) {}

            inline OutputStream as_stream() override;
        };
        class File: public Variant {
            std::string m_path;
            bool m_overwrite;

        public:
            File(std::string path, bool overwrite):
                m_path(path), m_overwrite(overwrite) {}

            inline OutputStream as_stream() override;
        };
        class Stream: public Variant {
            std::ostream* m_stream;

        public:
            Stream(std::ostream* stream): m_stream(stream) {}

            inline OutputStream as_stream() override;
        };

        std::unique_ptr<Variant> m_data;

        friend class OutputStream;

    public:
        /// \brief Constructs an output to \c stdout.
        inline Output(): Output(std::cout) {}

        /// \brief Move constructor.
        inline Output(Output&& other):
            m_data(std::move(other.m_data)) {}

        /// \brief Constructs a file output writing to the file at the given
        /// path.
        ///
        /// \param path The path to the output file.
        /// \param overwrite If \c true, the file will be overwritten in case
        /// it already exists, otherwise the output will be appended to it.
        inline Output(std::string path, bool overwrite=false):
            m_data(std::make_unique<File>(std::move(path), overwrite)) {}

        /// \brief Constructs an output to a byte buffer.
        ///
        /// \param buf The byte buffer to write to.
        inline Output(std::vector<uint8_t>& buf):
            m_data(std::make_unique<Memory>(&buf)) {}

        /// \brief Constructs an output to a stream.
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
        inline static Output from_path(std::string path, bool overwrite=false) {
            return Output(std::move(path), overwrite);
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
        inline OutputStream as_stream();
    };

    /// \cond INTERNAL
    class OutputStreamInternal {
        class Variant {
        public:
            virtual ~Variant() {}
            virtual std::ostream& stream() = 0;
        };

        class Memory: public Variant {
            BackInsertStream m_stream;
        public:
            friend class OutputStreamInternal;

            inline Memory(BackInsertStream&& stream): m_stream(stream) {}

            inline std::ostream& stream() override {
                return m_stream.stream();
            }
        };
        class Stream: public Variant {
            std::ostream* m_stream;
        public:
            friend class OutputStreamInternal;

            inline Stream(std::ostream* stream): m_stream(stream) {}

            inline std::ostream& stream() override {
                return *m_stream;
            }
        };
        class File: public Variant {
            std::string m_path;
            std::unique_ptr<std::ofstream> m_stream;

        public:
            friend class OutputStreamInternal;

            inline std::ostream& stream() override {
                return *m_stream;
            }

            inline File(const File& other): File(std::string(other.m_path)) {
            }

            inline File(std::string&& path, bool overwrite = false) {
                m_path = path;
                if (overwrite) {
                    m_stream = std::make_unique<std::ofstream>(m_path,
                        std::ios::out | std::ios::binary);
                } else {
                    m_stream = std::make_unique<std::ofstream>(m_path,
                        std::ios::out | std::ios::binary | std::ios::app);
                }
            }

            inline File(std::unique_ptr<std::ofstream>&& s) {
                m_stream = std::move(s);
            }

            inline File(File&& other) {
                m_stream = std::move(other.m_stream);
            }
        };

        std::unique_ptr<Variant> m_variant;

        friend class Output;
        friend class OutputStream;

        inline OutputStreamInternal(OutputStreamInternal::Memory&& mem):
            m_variant(std::make_unique<Memory>(std::move(mem))) {}
        inline OutputStreamInternal(OutputStreamInternal::File&& s):
            m_variant(std::make_unique<File>(std::move(s))) {}
        inline OutputStreamInternal(OutputStreamInternal::Stream&& s):
            m_variant(std::make_unique<Stream>(std::move(s))) {}
        inline OutputStreamInternal(OutputStreamInternal&& other):
            m_variant(std::move(other.m_variant)) {}

        inline OutputStreamInternal(const OutputStreamInternal& other) = delete;
        inline OutputStreamInternal() = delete;
    };
    /// \endcond

    /// \brief Provides a character stream to the underlying output.
    class OutputStream: OutputStreamInternal, public std::ostream {
        friend class Output;

        inline OutputStream(OutputStreamInternal&& mem):
            OutputStreamInternal(std::move(mem)),
            std::ostream(m_variant->stream().rdbuf()) {}
    public:
        /// \brief Move constructor.
        inline OutputStream(OutputStream&& mem):
            OutputStreamInternal(std::move(mem)),
            std::ostream(mem.rdbuf()) {}

        /// \brief Copy constructor (deleted).
        inline OutputStream(const OutputStream& other) = delete;

        /// \brief Default constructor (deleted).
        inline OutputStream() = delete;
    };

    inline OutputStream Output::Memory::as_stream() {
        return OutputStream {
            OutputStream::Memory {
                BackInsertStream { *m_buffer }
            }
        };
    }

    inline OutputStream Output::File::as_stream() {
        auto overwrite = m_overwrite;
        m_overwrite = false;
        return OutputStream {
            OutputStream::File {
                std::string(m_path),
                overwrite,
            }
        };
    }

    inline OutputStream Output::Stream::as_stream() {
        return OutputStream {
            OutputStream::Stream {
                m_stream
            }
        };
    }

    inline OutputStream Output::as_stream() {
        return m_data->as_stream();
    }

}}

#endif

