#pragma once

#include <tudocomp/util.hpp>

namespace tdc {
namespace ds {
    using dsflags_t = unsigned int;

    constexpr dsflags_t NONE = 0x00;
    constexpr dsflags_t SA   = 0x01;
    constexpr dsflags_t ISA  = 0x02;
    constexpr dsflags_t LCP  = 0x04;
    constexpr dsflags_t PHI  = 0x08;
    constexpr dsflags_t PLCP = 0x10;

    class InputRestriction {
        std::vector<uint8_t> m_escape_bytes;
        bool m_null_terminate;

    public:
        inline InputRestriction(const std::vector<uint8_t>& escape_bytes = {},
                                bool null_terminate = false):
            m_escape_bytes(escape_bytes),
            m_null_terminate(null_terminate) {}

        inline const std::vector<uint8_t>& escape_bytes() const {
            return m_escape_bytes;
        }

        inline bool null_terminate() const {
            return m_null_terminate;
        }
    };

    inline std::ostream& operator<<(std::ostream& o,
                                    const InputRestriction& v) {
        o << "{ escape_bytes: " << vec_to_debug_string(v.escape_bytes())
          << ", null_termination: " << (v.null_terminate() ? "true" : "false")
          << " }";
        return o;
    }

    class InputRestrictionAndFlags: public InputRestriction {
        dsflags_t m_flags;
    public:
        inline InputRestrictionAndFlags(const InputRestriction& other,
                                        dsflags_t flags):
            InputRestriction(other),
            m_flags(flags) {}
        inline InputRestrictionAndFlags():
            InputRestrictionAndFlags({}, ds::NONE) {}

        inline const dsflags_t& flags() const {
            return m_flags;
        }
    };

    inline std::ostream& operator<<(std::ostream& o,
                                    const InputRestrictionAndFlags& v) {
        o << "{ escape_bytes: " << vec_to_debug_string(v.escape_bytes())
          << ", null_termination: " << (v.null_terminate() ? "true" : "false")
          << ", ds_flags: " << v.flags()
          << " }";
        return o;
    }

    inline InputRestriction operator|(const InputRestriction& a, const InputRestriction& b) {
        std::vector<uint8_t> merged;

        merged.insert(merged.end(), a.escape_bytes().begin(), a.escape_bytes().end());
        merged.insert(merged.end(), b.escape_bytes().begin(), b.escape_bytes().end());

        std::sort(merged.begin(), merged.end());
        merged.erase(std::unique(merged.begin(), merged.end()), merged.end());

        return InputRestriction {
            merged,
            a.null_terminate() || b.null_terminate(),
        };
    }

    inline InputRestriction& operator|=(InputRestriction& a, const InputRestriction& b) {
        a = a | b;
        return a;
    }
}
}
