#pragma once

#include <malloc_count/malloc_count.hpp>
#include <list>
#include <map>
#include <sstream>

#include <tudocomp/util/Json.hpp>

namespace tdc {

class Stat {

public:
    struct Phase {
        ulong start_time;
        ulong end_time;

        ssize_t mem_off;
        ssize_t mem_peak;
        ssize_t mem_final;
    };

    #define NULL_PHASE Phase{0, 0, 0, 0}

private:
    std::string m_title;
    Phase m_phase;

    std::list<Stat> m_sub;
    json::Array m_stats;

    static void (*begin_phase)(void);
    static Phase (*end_phase)(void);

    static void (*pause_phase)(void);
    static void (*resume_phase)(void);

public:
    inline Stat() : m_phase(NULL_PHASE) {
    }

    inline Stat(const std::string& title) : m_title(title), m_phase(NULL_PHASE) {
    }

    inline const std::string& title() const {
        return m_title;
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

    template<typename T>
    inline void add_stat(const std::string& key, const T& value) {
        pause();

        json::Object pair;
        pair.set("key", key);
        pair.set("value", value);
        m_stats.add(pair);

        resume();
    }

    inline json::Object to_json() const {
        json::Object obj;
        obj.set("title", m_title);
        obj.set("timeStart", m_phase.start_time);
        obj.set("timeEnd", m_phase.end_time);
        obj.set("memOff", m_phase.mem_off);
        obj.set("memPeak", m_phase.mem_peak);
        obj.set("memFinal", m_phase.mem_final);
        obj.set("stats", m_stats);

        json::Array sub;
        for(auto it = m_sub.begin(); it != m_sub.end(); ++it) {
            sub.add(it->to_json());
        }
        obj.set("sub", sub);

        return obj;
    }
};

}

