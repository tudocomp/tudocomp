#pragma once

#include <tudocomp/util.hpp>

namespace tdc {
namespace io {
    /// Describes a set of restrictions placed on input data.
    ///
    /// Restrictions include illigal bytes in the input (called escape bytes here),
    /// and wether the input needs to be null terminated.
    class InputRestrictions {
        std::vector<uint8_t> m_escape_bytes;
        bool m_null_terminate;

        inline void sort_and_dedup() {
            std::sort(m_escape_bytes.begin(), m_escape_bytes.end());
            m_escape_bytes.erase(std::unique(m_escape_bytes.begin(), m_escape_bytes.end()), m_escape_bytes.end());
        }

        friend inline InputRestrictions operator|(const InputRestrictions& a,
                                                  const InputRestrictions& b);

    public:
        inline InputRestrictions(const std::vector<uint8_t>& escape_bytes = {},
                                 bool null_terminate = false):
            m_escape_bytes(escape_bytes),
            m_null_terminate(null_terminate) {
            sort_and_dedup();
        }

        inline const std::vector<uint8_t>& escape_bytes() const {
            return m_escape_bytes;
        }

        inline bool null_terminate() const {
            return m_null_terminate;
        }

        inline bool has_no_escape_restrictions() const {
            return m_escape_bytes.empty();
        }

        inline bool has_no_restrictions() const {
            return has_no_escape_restrictions() && (m_null_terminate == false);
        }

        inline bool has_escape_restrictions() const {
            return !has_no_escape_restrictions();
        }

        inline bool has_restrictions() const {
            return !has_no_restrictions();
        }
    };

    inline std::ostream& operator<<(std::ostream& o,
                                    const InputRestrictions& v) {
        o << "{ escape_bytes: " << vec_to_debug_string(v.escape_bytes())
          << ", null_termination: " << (v.null_terminate() ? "true" : "false")
          << " }";
        return o;
    }

    /// Merges two InpuTrestrictions to a combined set of restrictions.
    inline InputRestrictions operator|(const InputRestrictions& a, const InputRestrictions& b) {
        // Yes, kind of overkill here...

        std::vector<uint8_t> merged;

        merged.insert(merged.end(), a.escape_bytes().begin(), a.escape_bytes().end());
        merged.insert(merged.end(), b.escape_bytes().begin(), b.escape_bytes().end());

        auto r = InputRestrictions {
            merged,
            a.null_terminate() || b.null_terminate(),
        };

        r.sort_and_dedup();

        return r;
    }

    /// Merges two InpuTrestrictions to a combined set of restrictions.
    inline InputRestrictions& operator|=(InputRestrictions& a, const InputRestrictions& b) {
        a = a | b;
        return a;
    }

    inline bool operator==(const InputRestrictions& lhs,
                           const InputRestrictions& rhs) {
        return lhs.escape_bytes() == rhs.escape_bytes()
            && lhs.null_terminate() == rhs.null_terminate();
    }

    inline bool operator!=(const InputRestrictions& lhs,
                           const InputRestrictions& rhs) {
        return !(lhs == rhs);
    }
}

using InputRestrictions = io::InputRestrictions;

}
