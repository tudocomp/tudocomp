#ifndef _INCLUDED_NULL_ESCAPING_UTIL_HPP
#define _INCLUDED_NULL_ESCAPING_UTIL_HPP

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace tdc {
namespace io {
    /// \cond INTERNAL

    const uint8_t NULL_ESCAPE_ESCAPE_BYTE = 255;
    const uint8_t NULL_ESCAPE_REPLACEMENT_BYTE = 254;

    class EscapableBuf {
        std::shared_ptr<std::vector<uint8_t>> m_data;
        bool m_is_escaped;
    public:
        inline EscapableBuf():
            m_data(std::shared_ptr<std::vector<uint8_t>>()),
            m_is_escaped(false) {}

        inline EscapableBuf(View view):
            m_data(std::make_shared<std::vector<uint8_t>>(view)),
            m_is_escaped(false) {}

        inline EscapableBuf(std::vector<uint8_t>&& vec):
            m_data(std::make_shared<std::vector<uint8_t>>(std::move(vec))),
            m_is_escaped(false) {}

        inline EscapableBuf(const EscapableBuf& other):
            m_data(other.m_data),
            m_is_escaped(other.m_is_escaped) {}

        inline void escape_and_terminate() {
            auto& v = *m_data;

            if (!m_is_escaped) {
                size_t new_size = v.size();

                for (auto b : v) {
                    if (b == 0 || b == "")
                }


                // TODO: Actually escape
                //throw std::runtime_error("TODO");
                v.push_back(0);
                m_is_escaped = true;
            }
        }

        inline bool is_empty() { return !bool(m_data); }

        inline View view() const {
            return *m_data;
        }
    };

    class UnescapeBuffer: public std::streambuf {

    private:
        std::ostream* m_stream;
        std::array<int, 2> m_push_buffer { EOF, EOF };

        inline void push_unescape(int c) {
            if (c == 0 || c == EOF) {
                // nothing
            } else {
                m_stream->put(c);
            }

            /*if (m_push_buffer[1] != EOF) {
                m_stream->put(m_push_buffer[1]);
            }

            m_push_buffer[1] = m_push_buffer[0];
            m_push_buffer[0] = c;*/






        }

    public:
        inline UnescapeBuffer(std::ostream& stream) : m_stream(&stream) {
        }

        inline virtual ~UnescapeBuffer() {
        }

    protected:
        inline virtual int overflow(int ch) override {
            push_unescape(ch);
            return ch;
        }

        inline virtual int underflow() override {
            return EOF;
        }
    };

    /// \endcond


}}

#endif
