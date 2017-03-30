#pragma once

#include<tudocomp/io/RestrictedIOStream.hpp>

namespace tdc {namespace io {
    /// \cond INTERNAL
    class InputStreamInternal {
        class Variant {
        public:
            virtual std::istream& stream() = 0;
            virtual ~Variant() {}
        };

        class Memory: public InputStreamInternal::Variant {
            InputAllocChunkHandle m_handle;
            ViewStream m_stream;

            friend class InputStreamInternal;
        public:
            inline Memory(Memory&& other):
                m_handle(other.m_handle),
                m_stream(std::move(other.m_stream))
            {}

            inline Memory(InputAllocChunkHandle handle, View view):
                m_handle(handle),
                m_stream(view)
            {}

            inline std::istream& stream() override {
                return m_stream.stream();
            }

            inline Memory(const Memory& other) = delete;
            inline Memory() = delete;

            inline ~Memory() {
                if (m_handle) {
                    auto ttmp = m_handle->alloc();
                    auto tmp = InputAllocHandle(ttmp);
                    tmp.remove(m_handle);
                }
            }
        };
        class File: public InputStreamInternal::Variant {
            std::string m_path;
            std::unique_ptr<std::ifstream> m_stream;

            friend class InputStreamInternal;
        public:
            inline File(std::string&& path, size_t offset):
                m_path(std::move(path)),
                m_stream(std::make_unique<std::ifstream>(
                    m_path, std::ios::in | std::ios::binary))
            {
                auto& s = *m_stream;
                s.seekg(offset, std::ios::beg);
                if (!*m_stream) {
                    throw tdc_input_file_not_found_error(m_path);
                }
            }

            inline File(File&& other):
                m_path(std::move(other.m_path)),
                m_stream(std::move(other.m_stream))
            {}

            inline std::istream& stream() override {
                return *m_stream;
            }

            inline File(const File& other) = delete;
            inline File() = delete;
        };

        std::unique_ptr<InputStreamInternal::Variant> m_variant;
        std::unique_ptr<RestrictedIStreamBuf> m_restricted_istream;

        friend class InputStream;
        friend class Input;

        inline InputStreamInternal(const InputStreamInternal& other) = delete;
        inline InputStreamInternal() = delete;

        inline InputStreamInternal(InputStreamInternal::Memory&& mem,
                                   const InputRestrictions& restrictions):
            m_variant(std::make_unique<InputStreamInternal::Memory>(std::move(mem)))
        {
            if (!restrictions.has_no_restrictions()) {
                m_restricted_istream = std::make_unique<RestrictedIStreamBuf>(
                    m_variant->stream(),
                    restrictions
                );
            }
        }
        inline InputStreamInternal(InputStreamInternal::File&& f,
                                   const InputRestrictions& restrictions):
            m_variant(std::make_unique<InputStreamInternal::File>(std::move(f)))
        {
            if (!restrictions.has_no_restrictions()) {
                m_restricted_istream = std::make_unique<RestrictedIStreamBuf>(
                    m_variant->stream(),
                    restrictions
                );
            }
        }
        inline InputStreamInternal(InputStreamInternal&& s):
            m_variant(std::move(s.m_variant)),
            m_restricted_istream(std::move(s.m_restricted_istream)) {}

        inline std::streambuf* internal_rdbuf() {
            if (m_restricted_istream) {
                return &*m_restricted_istream;
            } else {
                return m_variant->stream().rdbuf();
            }
        }
    };
    /// \endcond

    /// \brief Provides a character stream of the underlying input.
    class InputStream: InputStreamInternal, public std::istream {
        friend class Input;

        inline InputStream(InputStreamInternal&& mem):
            InputStreamInternal(std::move(mem)),
            std::istream(InputStreamInternal::internal_rdbuf()) {
        }
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

    inline InputStream Input::Variant::as_stream() const {
        // Change in such a way that restricting stream is applied
        // for both file and memory

        if (source().is_file()) {
            DCHECK(to_unknown())
                << "TODO: Can not yet slice the trailing end of a stream";

            return InputStream {
                InputStreamInternal {
                    InputStream::File {
                        std::string(source().file()),
                        from()
                    },
                    restrictions()
                }
            };
        } if (source().is_view()) {
            return InputStream {
                InputStreamInternal {
                    InputStream::Memory {
                        nullptr,
                        source().view().slice(from(), to())
                    },
                    restrictions()
                }
            };
        } else {
            auto h = alloc().find_or_construct(
                source(), from(), to(), restrictions());
            auto v = h->view();

            return InputStream {
                InputStreamInternal {
                    InputStream::Memory {
                        h,
                        v
                    },
                    // No restrictions since they are already realized in the buffer
                    InputRestrictions()
                }
            };
        }
    }

}}
