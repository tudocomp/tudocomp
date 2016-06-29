#ifndef _INCLUDED_CLOCK_HPP_
#define _INCLUDED_CLOCK_HPP_

#include <time.h>

namespace tudostat {

class Clock {

private:
    unsigned long m_start;

public:
    static inline unsigned long now() {
        timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);

        return t.tv_sec * 1000L + t.tv_nsec / 1000000L;
    }

    inline Clock() : m_start(now()) {
    }

    ~Clock() {
    }

    inline unsigned long elapsed_time() const {
        return now() - m_start;
    }

    inline void restart() {
        m_start = now();
    }
};

}

#endif
