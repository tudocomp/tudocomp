#pragma once

#include<tudocomp/io/EscapeMap.hpp>

namespace tdc {namespace io {
    // TODO: Make these adapters use buffers to reduce
    // virtual call overhead.

    /// Adapter class over a `std::ostream` that
    /// reverse the escaping and null termination
    /// of data written to it according
    /// to the provided input restrictions.
    class RestrictedOStreamBuf: public std::streambuf {
    private:
        std::ostream* m_stream;
        FastUnescapeMap m_fast_unescape_map;
        bool m_saw_escape = false;
        bool m_saw_null = false;

        /*
         Null termination logic is going to be a bit funky:
         If null termination is enabled, this adapter will
         hold on to each null character that should be output,
         and only output it if there are further characters after it.
         This way, the very last null will be ellided but no earlier ones.
         */

        inline void internal_flush() {
            // Nothing to do here, as any state needs further input to react
        }

        inline void push_unescape(uint8_t c) {
            auto put_internal = [&](uint8_t d){
                m_stream->put(char(d));
            };

            if (m_saw_escape) {
                m_saw_escape = false;
                if (m_saw_null) {
                    put_internal(0);
                    m_saw_null = false;
                }
                put_internal(m_fast_unescape_map.lookup_byte(c));
            } else if (m_fast_unescape_map.has_escape_bytes()
                    && (c == m_fast_unescape_map.escape_byte())) {
                m_saw_escape = true;
            } else if (c == 0 && m_fast_unescape_map.null_terminate()) {
                if (m_saw_null) {
                    put_internal(0);
                    m_saw_null = false;
                }
                // drop it temporarily
                m_saw_null = true;
            } else {
                if (m_saw_null) {
                    put_internal(0);
                    m_saw_null = false;
                }
                put_internal(c);
            }
        }

    public:
        inline RestrictedOStreamBuf(std::ostream& stream,
                                    const InputRestrictions& restrictions):
            m_stream(&stream),
            m_fast_unescape_map(EscapeMap(restrictions)) {}

        inline RestrictedOStreamBuf() = delete;
        inline RestrictedOStreamBuf(const RestrictedOStreamBuf& other) = delete;
        inline RestrictedOStreamBuf(RestrictedOStreamBuf&& other) = delete;

        inline virtual ~RestrictedOStreamBuf() {
            if (m_fast_unescape_map.null_terminate()) {
                DCHECK(m_saw_null) << "Text to be unescaped did not end with a 0";
            }
        }

    protected:
        inline virtual int overflow(int ch) override {
            if (ch == traits_type::eof()) {
                internal_flush();
            } else {
                push_unescape(traits_type::to_char_type(ch));
            }

            return ch;
        }
    };

    /// Adapter class over a `std::istream` that
    /// escapes and null terminates the data read from it
    /// according to the provided input restrictions.
    class RestrictedIStreamBuf: public std::streambuf {
    private:
        std::istream* m_stream;
        FastEscapeMap m_fast_escape_map;
        int m_char_buf = traits_type::eof();
        bool m_nt_done = false;
        int m_current;

        inline int pull_escape() {
            if (m_char_buf != traits_type::eof()) {
                int tmp = m_char_buf;
                m_char_buf = traits_type::eof();
                return tmp;
            }

            char c;
            if (m_stream->get(c)) {
                auto byte = uint8_t(c);
                if (m_fast_escape_map.lookup_flag(byte)) {
                    m_char_buf = m_fast_escape_map.lookup_byte(byte);
                    return m_fast_escape_map.escape_byte();
                } else {
                    return byte;
                }
            } else {
                if (m_fast_escape_map.null_terminate() && !m_nt_done) {
                    m_nt_done = true;
                    return 0;
                }
                return traits_type::eof();
            }
        }

    public:
        inline RestrictedIStreamBuf(std::istream& stream,
                                    InputRestrictions restrictions):
            m_stream(&stream),
            m_fast_escape_map(EscapeMap(restrictions)) {
            m_current = pull_escape();
        }

        inline RestrictedIStreamBuf() = delete;
        inline RestrictedIStreamBuf(const RestrictedIStreamBuf& other) = delete;
        inline RestrictedIStreamBuf(RestrictedIStreamBuf&& other) = delete;

        inline virtual ~RestrictedIStreamBuf() {
        }

    protected:
        inline virtual int underflow() override {
            return m_current;
        }
        inline virtual int uflow() override {
            int tmp = m_current;
            m_current = pull_escape();
            return tmp;
        }
    };

}}
