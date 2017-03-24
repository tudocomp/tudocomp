#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/io/InputRestrictions.hpp>

namespace tdc {
namespace ds {
    using dsflags_t = unsigned int;

    constexpr dsflags_t NONE = 0x00;
    constexpr dsflags_t SA   = 0x01;
    constexpr dsflags_t ISA  = 0x02;
    constexpr dsflags_t LCP  = 0x04;
    constexpr dsflags_t PHI  = 0x08;
    constexpr dsflags_t PLCP = 0x10;

    using io::InputRestrictions;

    class InputRestrictionsAndFlags: public InputRestrictions {
        dsflags_t m_flags;
    public:
        inline InputRestrictionsAndFlags(const InputRestrictions& other,
                                        dsflags_t flags):
            InputRestrictions(other),
            m_flags(flags) {}
        inline InputRestrictionsAndFlags():
            InputRestrictionsAndFlags({}, ds::NONE) {}

        inline const dsflags_t& flags() const {
            return m_flags;
        }
    };

    inline std::ostream& operator<<(std::ostream& o,
                                    const InputRestrictionsAndFlags& v) {
        o << "{ escape_bytes: " << vec_to_debug_string(v.escape_bytes())
          << ", null_termination: " << (v.null_terminate() ? "true" : "false")
          << ", ds_flags: " << v.flags()
          << " }";
        return o;
    }
}
}
