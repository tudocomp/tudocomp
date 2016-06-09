#ifndef _INCLUDED_STAT_HPP_
#define _INCLUDED_STAT_HPP_

#include <malloc_count/malloc_count.hpp>
#include <list>
#include <sstream>

namespace tudostat {

class Stat {

public:
    struct Phase {
        ulong start_time;
        ulong duration;

        size_t mem_peak;
    };

    #define NULL_PHASE Phase{0, 0, 0}

private:
    std::string m_title;
    Phase m_stats;

public:
    static void (*begin_phase)(void);
    static Phase (*end_phase)(void);

    inline Stat() : m_stats(NULL_PHASE) {
    }

    inline Stat(const std::string& title) : m_title(title), m_stats(NULL_PHASE) {
    }

    inline void set_title(const std::string& title) {
        m_title = title;
    }

    inline void begin() {
        m_stats = NULL_PHASE;
        if(begin_phase) {
            begin_phase();
        }
    }

    inline const Phase& end() {
        if(end_phase) {
            m_stats = end_phase();
            return m_stats;
        } else {
            return m_stats;
        }
    }

    inline const Phase& stats() const {
        return m_stats;
    }

    inline std::string to_json() const {
        std::stringstream ss;
        ss << "{ title: '" << m_title << "'"
           << ", startTime: " << m_stats.start_time
           << ", duration: " << m_stats.duration
           << ", memPeak: " << m_stats.mem_peak
           << " }";
        return ss.str();
    }
};

}

#endif

