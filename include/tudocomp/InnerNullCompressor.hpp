#ifndef _INCLUDED_INNER_NULL_COMPRESSOR_HPP_
#define _INCLUDED_INNER_NULL_COMPRESSOR_HPP_

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/CreateAlgorithm.hpp>
#include <vector>
#include <memory>

namespace tdc {

class InnerNullCompressor: public Compressor {
    std::unique_ptr<Compressor> m_compressor;
    bool m_check_inner_nulls;
    bool m_escape_inner_nulls;
    bool m_null_terminate;
    uint8_t m_escape_byte;
    uint8_t m_replacement_byte;

public:
    inline static Meta meta() {
        Meta m("compressor", "inner_null");
        m.option("compressor").dynamic_compressor();
        m.option("check_inner_nulls").dynamic("true");
        m.option("escape_inner_nulls").dynamic("true");
        m.option("null_terminate").dynamic("false");
        m.option("escape_byte").dynamic("255");
        m.option("replacement_byte").dynamic("254");
        return m;
    }

    inline InnerNullCompressor(Env&& env):
        Compressor(std::move(env)),
        m_compressor(create_algo_with_registry_dynamic(
            this->env().registry(),
            this->env().option("compressor").as_algorithm())),
        m_check_inner_nulls(this->env().option("check_inner_nulls").as_bool()),
        m_escape_inner_nulls(this->env().option("escape_inner_nulls").as_bool()),
        m_null_terminate(this->env().option("null_terminate").as_bool()),
        m_escape_byte(this->env().option("escape_byte").as_integer()),
        m_replacement_byte(this->env().option("replacement_byte").as_integer()) {}

    inline virtual void compress(Input& input, Output& output) override final {
        if (m_escape_inner_nulls || m_check_inner_nulls) {
            size_t bytes_found[256] = {};
            size_t size = 0;
            {
                Input i_copy = input;
                auto stream = i_copy.as_stream();
                for (uint8_t byte : stream) {
                    bytes_found[byte] += 1;
                    size += 1;
                }
            }

            // TODO: Output alphabet just because we can.

            // Since the escaping is escape-byte agnostic,
            // the following code is written to work with arbitrary bytes:

            uint8_t bytes_in_need_of_escaping_buf[2] = { };
            bytes_in_need_of_escaping_buf[0] = 0;
            bytes_in_need_of_escaping_buf[1] = m_escape_byte;

            View bytes_in_need_of_escaping(bytes_in_need_of_escaping_buf,
                                           sizeof(bytes_in_need_of_escaping_buf));

            auto found_bytes_in_need_of_escaping = [&]() -> bool {
                for (uint8_t b : bytes_in_need_of_escaping) {
                    if (bytes_found[b] > 0) {
                        return true;
                    }
                }
                return false;
            };

            auto count_bytes_in_need_of_escaping = [&]() -> size_t {
                size_t amount = 0;
                for (uint8_t b : bytes_in_need_of_escaping) {
                    amount += bytes_found[b];
                }
                return amount;
            };

            auto is_escape_byte = [&](uint8_t chr) -> bool {
                for (uint8_t b : bytes_in_need_of_escaping) {
                    if (chr == b) {
                        return true;
                    }
                }
                return false;
            };

            // Escape scheme:
            // 0xff -> 0xff, 0xff
            // 0xfe -> 0xff, 0xfe
            //
            // (Default: escape_byte == 0xff, replacement_byte == 0xfe)

            // Check if we need actual escaping
            if (found_bytes_in_need_of_escaping()) {
                if (m_escape_inner_nulls) {
                    size_t size_increase = count_bytes_in_need_of_escaping() * 2;
                    size_t null_size = m_null_terminate ? 1 : 0;

                    // Reallocate with bigger size and escape
                    std::vector<uint8_t> escaped_input;
                    escaped_input.reserve(size + size_increase + null_size);

                    auto stream = input.as_stream();
                    for (uint8_t byte : stream) {
                        if (is_escape_byte(byte)) {
                            escaped_input.push_back(m_escape_byte);
                            if (byte == 0) {
                                escaped_input.push_back(m_replacement_byte);
                            } else {
                                escaped_input.push_back(m_escape_byte);
                            }
                        } else {
                            escaped_input.push_back(byte);
                        }
                    }

                    if (m_null_terminate) {
                        escaped_input.push_back(0);
                    }

                    Input new_input(escaped_input);
                    m_compressor->compress(new_input, output);
                    return;
                } else if (m_check_inner_nulls && (bytes_found[0] > 0)) {
                    std::stringstream ss;
                    ss << "Found inner bytes of value 0.";
                    env().error(ss.str());
                }
            }
        }

        m_compressor->compress(input, output);
    }

    inline virtual void decompress(Input& input, Output& output) override final {
        if (m_escape_inner_nulls) {
            // TODO: Define a custom outstream that unescapes on the fly

            std::vector<uint8_t> out_buf;
            Output tmp_output(out_buf);
            auto o = output.as_stream();
            m_compressor->decompress(input, tmp_output);

            bool error_trunc = false;

            auto a = out_buf.cbegin();
            auto b = out_buf.cend();

            while (a != b) {
                uint8_t byte = *a;

                if (byte == m_escape_byte) {
                    a++;
                    if (a == b) {
                        error_trunc = true;
                        break;
                    }
                    uint8_t byte2 = *a;
                    if (byte2 == m_escape_byte) {
                        o.put(m_escape_byte);
                    } else if (byte2 == m_replacement_byte) {
                        o.put(0);
                    }
                } else {
                    o.put(*a);
                }

                a++;
            }

            if (error_trunc) {
                env().error("input was truncated (no byte after escape byte)");
            }

        } else {
            m_compressor->decompress(input, output);
        }
    }
};

}

#endif
