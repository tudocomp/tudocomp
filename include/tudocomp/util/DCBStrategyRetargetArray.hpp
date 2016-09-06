#ifndef _INCLUDED_DCB_STRATEGY_RETARGET_ARRAY_HPP_
#define _INCLUDED_DCB_STRATEGY_RETARGET_ARRAY_HPP_

#include <sdsl/int_vector.hpp>

#include <tudocomp/util.h>

namespace tudocomp {

class DCBStrategyRetargetArray {

private:
    size_t m_none;
    sdsl::int_vector<> m_waiting;

public:
    inline DCBStrategyRetargetArray(size_t len) : m_none(len) {
        m_waiting = sdsl::int_vector<>(len, m_none, bits_for(len));
    }

    inline void wait(size_t pos, size_t src) {
        while(m_waiting[src] != m_none) {
            src = m_waiting[src];
        }

        m_waiting[src] = pos;
    }

    inline bool next_waiting_for(size_t pos, size_t& out_waiting) {
        size_t waiting = m_waiting[pos];
        if(waiting == m_none) {
            return false;
        } else {
            out_waiting = waiting;
            m_waiting[pos] = m_none;
            return true;
        }
    }

};

}

#endif
