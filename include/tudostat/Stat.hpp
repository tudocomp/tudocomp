#ifndef _INCLUDED_STAT_HPP_
#define _INCLUDED_STAT_HPP_

#include <malloc_count/malloc_count.hpp>
#include <list>
#include <map>
#include <sstream>

namespace tudostat {

class Stat {

public:
    struct Phase {
        ulong start_time;
        ulong duration;

        size_t mem_off;
        size_t mem_peak;
    };

    #define NULL_PHASE Phase{0, 0, 0, 0}

private:
    std::string m_title;
    Phase m_phase;

    std::list<Stat> m_sub;

    std::map<std::string, long> m_stats_int;
    std::map<std::string, double> m_stats_real;

public:
    static void (*begin_phase)(void);
    static Phase (*end_phase)(void);

    static void (*pause_phase)(void);
    static void (*resume_phase)(void);

    inline Stat() : m_phase(NULL_PHASE) {
    }

    inline Stat(const std::string& title) : m_title(title), m_phase(NULL_PHASE) {
    }

    inline void set_title(const std::string& title) {
        m_title = title;
    }

    inline void begin() {
        m_phase = NULL_PHASE;
        if(begin_phase) {
            begin_phase();
        }
    }

    inline const Phase& end() {
        if(end_phase) {
            m_phase = end_phase();
            return m_phase;
        } else {
            return m_phase;
        }
    }

    inline void pause() const {
        if(pause_phase) pause_phase();
    }

    inline void resume() const {
        if(resume_phase) resume_phase();
    }

    inline const Phase& phase() const {
        return m_phase;
    }

    inline void add_sub(const Stat& stat) {
        pause();
        m_sub.push_back(stat);
        resume();
    }

    inline void add_stat(const std::string& key, int value) {
        add_stat(key, (long)value);
    }

    inline void add_stat(const std::string& key, unsigned int value) {
        add_stat(key, (long)value);
    }

    inline void add_stat(const std::string& key, size_t value) {
        add_stat(key, (long)value);
    }

    inline void add_stat(const std::string& key, long value) {
        pause();
        m_stats_int.emplace(key, value);
        resume();
    }

    inline void add_stat(const std::string& key, float value) {
        add_stat(key, (double)value);
    }

    inline void add_stat(const std::string& key, double value) {
        pause();
        m_stats_real.emplace(key, value);
        resume();
    }

    inline std::string to_json() const {
        std::stringstream ss;
        ss << "{\"title\": \"" << m_title << "\""
           << ", \"startTime\": " << m_phase.start_time
           << ", \"duration\": " << m_phase.duration
           << ", \"memOff\": " << m_phase.mem_off
           << ", \"memPeak\": " << m_phase.mem_peak;
        
        ss << ", \"stats\": {";

        size_t stats_total = m_stats_int.size() + m_stats_real.size();
        size_t i = 0;

        for(auto it = m_stats_int.begin(); it != m_stats_int.end(); it++) {
            ss << "\"" << it->first << "\": " << it->second;
            if(++i < stats_total) ss << ", ";
        }
        for(auto it = m_stats_real.begin(); it != m_stats_real.end(); it++) {
            ss << "\"" << it->first << "\": " << it->second;
            if(++i < stats_total) ss << ", ";
        }

        ss << "}, \"sub\": [";
        for(auto it = m_sub.begin(); it != m_sub.end(); it++) {
            ss << it->to_json();
            if(std::next(it) != m_sub.end()) ss << ", ";
        }
        ss << "]}";

        return ss.str();
    }
};

}

#endif

