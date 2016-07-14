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

namespace tudocomp {
namespace io {

    class OutputStream;

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
        /// An empty Output. Defaults to writing to stdout
        inline Output(): Output(std::cout) {}

        /// Move constructor
        inline Output(Output&& other):
            m_data(std::move(other.m_data)) {}

        Output(std::string path, bool overwrite=false):
            m_data(std::make_unique<File>(std::move(path), overwrite)) {}

        Output(std::vector<uint8_t>& buf):
            m_data(std::make_unique<Memory>(&buf)) {}

        Output(std::ostream& stream):
            m_data(std::make_unique<Stream>(&stream)) {}

        Output& operator=(Output&& other) {
            m_data = std::move(other.m_data);
            return *this;
        }

        /// DEPRECATED
        static Output from_path(std::string path, bool overwrite=false) {
            return Output(std::move(path), overwrite);
        }

        /// DEPRECATED
        static Output from_memory(std::vector<uint8_t>& buf) {
            return Output(buf);
        }

        /// DEPRECATED
        static Output from_stream(std::ostream& stream) {
            return Output(stream);
        }

        inline OutputStream as_stream();
    };

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

            Memory(BackInsertStream&& stream): m_stream(stream) {}

            inline std::ostream& stream() override {
                return m_stream.stream();
            }
        };
        class Stream: public Variant {
            std::ostream* m_stream;
        public:
            friend class OutputStreamInternal;

            Stream(std::ostream* stream): m_stream(stream) {}

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

            File(const File& other): File(std::string(other.m_path)) {
            }

            File(std::string&& path, bool overwrite = false) {
                m_path = path;
                if (overwrite) {
                    m_stream = std::make_unique<std::ofstream>(m_path,
                        std::ios::out | std::ios::binary);
                } else {
                    m_stream = std::make_unique<std::ofstream>(m_path,
                        std::ios::out | std::ios::binary | std::ios::app);
                }
            }

            File(std::unique_ptr<std::ofstream>&& s) {
                m_stream = std::move(s);
            }

            File(File&& other) {
                m_stream = std::move(other.m_stream);
            }
        };

        std::unique_ptr<Variant> m_variant;

        friend class Output;
        friend class OutputStream;

        OutputStreamInternal(OutputStreamInternal::Memory&& mem):
            m_variant(std::make_unique<Memory>(std::move(mem))) {}
        OutputStreamInternal(OutputStreamInternal::File&& s):
            m_variant(std::make_unique<File>(std::move(s))) {}
        OutputStreamInternal(OutputStreamInternal::Stream&& s):
            m_variant(std::make_unique<Stream>(std::move(s))) {}
        OutputStreamInternal(OutputStreamInternal&& other):
            m_variant(std::move(other.m_variant)) {}

        OutputStreamInternal(const OutputStreamInternal& other) = delete;
        OutputStreamInternal() = delete;
    };

    class OutputStream: OutputStreamInternal, public std::ostream {
        friend class Output;

        OutputStream(OutputStreamInternal&& mem):
            OutputStreamInternal(std::move(mem)),
            std::ostream(m_variant->stream().rdbuf()) {}
    public:
        OutputStream(OutputStream&& mem):
            OutputStreamInternal(std::move(mem)),
            std::ostream(mem.rdbuf()) {}

        OutputStream(const OutputStream& other) = delete;
        OutputStream() = delete;
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

