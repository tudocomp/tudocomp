#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <tudocomp/io/BackInsertStream.hpp>
#include<tudocomp/io/RestrictedIOStream.hpp>

namespace tdc {
namespace io {
    /// \cond INTERNAL
    class OutputStreamInternal {
        class Variant {
        public:
            virtual ~Variant() {}
            virtual std::ostream& stream() = 0;

            virtual std::streampos tellp() {
                return stream().tellp();
            }
        };

        class Memory: public Variant {
            BackInsertStream m_stream;
        public:
            friend class OutputStreamInternal;

            inline Memory(BackInsertStream&& stream): m_stream(stream) {}

            inline std::ostream& stream() override {
                return m_stream.stream();
            }

            inline Memory(Memory&& other):
                m_stream(std::move(other.m_stream)) {}

            inline Memory(const Memory& other) = delete;
            inline Memory() = delete;

            virtual std::streampos tellp() override {
                return m_stream.size();
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

            inline Stream(Stream&& other):
                m_stream(std::move(other.m_stream)) {}

            inline Stream(const Stream& other) = delete;
            inline Stream() = delete;
        };
        class File: public Variant {
            std::string m_path;
            std::unique_ptr<std::ofstream> m_stream;

        public:
            friend class OutputStreamInternal;

            inline File(std::string&& path, bool overwrite = false) {
                m_path = path;
                if (overwrite) {
                    m_stream = std::make_unique<std::ofstream>(m_path,
                        std::ios::out | std::ios::binary);
                } else {
                    m_stream = std::make_unique<std::ofstream>(m_path,
                        std::ios::out | std::ios::binary | std::ios::app);
                }
                if (!*m_stream) {
                    throw tdc_output_file_not_found_error(m_path);
                }
            }

            inline File(File&& other):
                m_path(std::move(other.m_path)),
                m_stream(std::move(other.m_stream)) {}

            inline std::ostream& stream() override {
                return *m_stream;
            }

            inline File(const File& other) = delete;
            inline File() = delete;
        };

        std::unique_ptr<Variant> m_variant;
        std::unique_ptr<RestrictedOStreamBuf> m_restricted_ostream;

        friend class Output;
        friend class OutputStream;

        inline OutputStreamInternal(const OutputStreamInternal& other) = delete;
        inline OutputStreamInternal() = delete;

        inline OutputStreamInternal(OutputStreamInternal::Memory&& mem,
                                    const InputRestrictions& restrictions):
            m_variant(std::make_unique<Memory>(std::move(mem)))
        {
            if (!restrictions.has_no_restrictions()) {
                m_restricted_ostream = std::make_unique<RestrictedOStreamBuf>(
                    m_variant->stream(),
                    restrictions
                );
            }
        }
        inline OutputStreamInternal(OutputStreamInternal::File&& s,
                                    const InputRestrictions& restrictions):
            m_variant(std::make_unique<File>(std::move(s)))
        {
            if (!restrictions.has_no_restrictions()) {
                m_restricted_ostream = std::make_unique<RestrictedOStreamBuf>(
                    m_variant->stream(),
                    restrictions
                );
            }
        }
        inline OutputStreamInternal(OutputStreamInternal::Stream&& s,
                                    const InputRestrictions& restrictions):
            m_variant(std::make_unique<Stream>(std::move(s)))
        {
            if (!restrictions.has_no_restrictions()) {
                m_restricted_ostream = std::make_unique<RestrictedOStreamBuf>(
                    m_variant->stream(),
                    restrictions
                );
            }
        }

        inline OutputStreamInternal(OutputStreamInternal&& other):
            m_variant(std::move(other.m_variant)),
            m_restricted_ostream(std::move(other.m_restricted_ostream)) {}

        inline std::streambuf* internal_rdbuf() {
            if (m_restricted_ostream) {
                return &*m_restricted_ostream;
            } else {
                return m_variant->stream().rdbuf();
            }
        }

        inline std::streampos tellp() {
            return m_variant->tellp();
        }
    };
    /// \endcond

    /// \brief Provides a character stream to the underlying output.
    class OutputStream: OutputStreamInternal, public std::ostream {
        friend class Output;

        inline OutputStream(OutputStreamInternal&& mem):
            OutputStreamInternal(std::move(mem)),
            std::ostream(OutputStreamInternal::internal_rdbuf()) {
        }
    public:
        /// \brief Move constructor.
        inline OutputStream(OutputStream&& mem):
            OutputStreamInternal(std::move(mem)),
            std::ostream(mem.rdbuf()) {}

        /// \brief Copy constructor (deleted).
        inline OutputStream(const OutputStream& other) = delete;

        /// \brief Default constructor (deleted).
        inline OutputStream() = delete;

        inline std::streampos tellp() {
            return OutputStreamInternal::tellp();
        }
    };

    inline OutputStream Output::Memory::as_stream() const {
        return OutputStream {
            OutputStreamInternal {
                OutputStream::Memory {
                    BackInsertStream { *m_buffer }
                },
                restrictions()
            }
        };
    }

    inline OutputStream Output::File::as_stream() const {
        // TODO: Replace this mutation with a read-only solution
        // Eg by making it always append-only
        auto overwrite = m_overwrite;
        m_overwrite = false;
        return OutputStream {
            OutputStreamInternal {
                OutputStream::File {
                    std::string(m_path),
                    overwrite,
                },
                restrictions()
            }
        };
    }

    inline OutputStream Output::Stream::as_stream() const {
        return OutputStream {
            OutputStreamInternal {
                OutputStream::Stream {
                    m_stream
                },
                restrictions()
            }
        };
    }

    inline OutputStream Output::as_stream() const {
        return m_data->as_stream();
    }
}}
