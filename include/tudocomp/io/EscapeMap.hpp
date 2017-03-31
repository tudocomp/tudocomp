#pragma once

#include <tudocomp/ds/TextDSFlags.hpp>

namespace tdc {namespace io {
    // All bytes that can be used for escaping.
    // As a heuristic, this contains all illigal bytes in the utf8
    // unicode encoding to prevent uneccessary escaping for text inputs
    static const std::vector<uint8_t> ESCAPE_BYTE_POOL {
        255,
        254,
        192,
        193,
        245,
        246,
        247,
        248,
        249,
        250,
        251,
        252,
        253,
    };

    struct EscapeMap {
        // The map is defined as such:
        // For escaping:
        // if input byte is escape_bytes[i], then
        //     escape_bytes[i] -> [replacement_bytes[0], replacement_bytes[i]]
        // For unescaping:
        // if input byte is replacement_bytes[0], then
        //     [replacement_bytes[0], replacement_bytes[i]] -> escape_bytes[i]

        std::vector<uint8_t> m_replacement_bytes;
        std::vector<uint8_t> m_escape_bytes;
        bool m_null_terminate = false;

        inline EscapeMap(const InputRestrictions& rest)  {
            m_escape_bytes = rest.escape_bytes();
            m_null_terminate = rest.null_terminate();

            if (!m_escape_bytes.empty()) {
                // Determine which bytes are legal to use for escaping
                // by using the POOL constant and removing all
                // bytes that need escaping
                for (uint8_t byte : ESCAPE_BYTE_POOL) {
                    bool found = false;
                    for (uint8_t x : m_escape_bytes) {
                        if (x == byte) found = true;
                    }
                    if (!found && m_replacement_bytes.size() < (m_escape_bytes.size() + 1)) {
                        m_replacement_bytes.push_back(byte);
                    }
                }

                // m_replacement_bytes[0] is our escape byte, so add it to m_escape_bytes
                m_escape_bytes.insert(m_escape_bytes.begin(), m_replacement_bytes.at(0));
            }

            // For the algorithm to work we need a escape byte, and a byte for each
            // to-be-escaped byte
            DCHECK_EQ(m_replacement_bytes.size(), m_escape_bytes.size());
        }

        inline bool has_escape_bytes() const {
            return !m_escape_bytes.empty();
        }

        inline uint8_t escape_byte() const {
            if (has_escape_bytes()) {
                DCHECK_EQ(m_replacement_bytes.front(), m_escape_bytes.front());
                return m_replacement_bytes.front();
            } else {
                return -1;
            }
        }

        inline const std::vector<uint8_t>& replacement_bytes() const {
            return m_replacement_bytes;
        }

        inline const std::vector<uint8_t>& escape_bytes() const {
            return m_escape_bytes;
        }

        inline bool null_terminate() const {
            return m_null_terminate;
        }

    };

    // For quick on-the-stack lookups of escaping chars
    class FastEscapeMap {
        std::array<uint8_t, 256> m_escape_map;
        std::array<uint8_t, 256> m_escape_map_flag;
        uint8_t m_escape_byte = 0;
        bool m_null_terminate = false;
    public:
        inline FastEscapeMap() {
            for (size_t i = 0; i < 256; i++) {
                m_escape_map[i] = i;
                m_escape_map_flag[i] = 0;
            }
        }

        inline FastEscapeMap(const EscapeMap& em): FastEscapeMap() {
            auto& em_eb = em.escape_bytes();
            auto& em_rb = em.replacement_bytes();
            for (size_t i = 0; i < em_eb.size(); i++) {
                m_escape_map[em_eb[i]] = em_rb[i];
                m_escape_map_flag[em_eb[i]] = 1;
            }
            m_escape_byte = em.escape_byte();
            m_null_terminate = em.null_terminate();
        }

        inline uint8_t lookup_flag(size_t i) const {
            DCHECK_LT(i, 256);
            return m_escape_map_flag[i];
        }

        inline bool lookup_flag_bool(size_t i) const {
            return lookup_flag(i) != 0;
        }

        inline uint8_t lookup_byte(size_t i) const {
            DCHECK_LT(i, 256);
            return m_escape_map[i];
        }

        inline uint8_t escape_byte() const {
            return m_escape_byte;
        }

        inline bool null_terminate() const {
            return m_null_terminate;
        }
    };

    // For quick on-the-stack lookups of unescaping chars
    class FastUnescapeMap {
        std::array<uint8_t, 256> m_unescape_map;
        uint8_t m_escape_byte = 0;
        bool m_null_terminate = false;
        bool m_has_escape_bytes = false;
    public:
        inline FastUnescapeMap() {
            for (size_t i = 0; i < 256; i++) {
                m_unescape_map[i] = i;
            }
        }

        inline FastUnescapeMap(const EscapeMap& em): FastUnescapeMap() {
            auto& em_eb = em.escape_bytes();
            auto& em_rb = em.replacement_bytes();
            for (size_t i = 0; i < em_eb.size(); i++) {
                m_unescape_map[em_rb[i]] = em_eb[i];
            }
            m_escape_byte = em.escape_byte();
            m_null_terminate = em.null_terminate();
            m_has_escape_bytes = em.has_escape_bytes();
        }

        inline uint8_t lookup_byte(size_t i) const {
            DCHECK_LT(i, 256);
            return m_unescape_map[i];
        }

        inline uint8_t escape_byte() const {
            return m_escape_byte;
        }

        inline bool null_terminate() const {
            return m_null_terminate;
        }

        inline bool has_escape_bytes() const {
            return m_has_escape_bytes;
        }
    };
}}
