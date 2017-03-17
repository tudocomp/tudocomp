#pragma once

#include <ctime>
#include <list>

#include <tudocomp_stat/Json.hpp>

namespace tdc {

class Stat {
private:
    static Stat* s_current;

    inline static unsigned long current_time_millis() {
        timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);

        return t.tv_sec * 1000L + t.tv_nsec / 1000000L;
    }

    class Data {
        friend class Stat;

    private:
        std::string title;

        unsigned long time_start;
        unsigned long time_end;
        ssize_t mem_off;
        ssize_t mem_current;
        ssize_t mem_peak;

        json::Array stats;

        std::list<Data> children;

        template<typename T>
        inline void log_stat(const std::string& key, const T& value) {
            json::Object pair;
            pair.set("key", key);
            pair.set("value", value);
            stats.add(pair);
        }

    public:
        inline json::Object to_json() const {
            json::Object obj;
            obj.set("title",     title);
            obj.set("timeStart", time_start);
            obj.set("timeEnd",   time_end);
            obj.set("memOff",    mem_off);
            obj.set("memPeak",   mem_peak);
            obj.set("memFinal",  mem_current);
            obj.set("stats",     stats);

            json::Array sub;
            for(auto it = children.begin(); it != children.end(); ++it) {
                sub.add(it->to_json());
            }
            obj.set("sub", sub);

            return obj;
        }
    };

    Stat* m_parent;
    Data  m_data;

    bool  m_track_memory;

    inline void track_alloc_internal(size_t bytes) {
        if(m_track_memory) {
            m_data.mem_current += bytes;
            m_data.mem_peak = std::max(m_data.mem_peak, m_data.mem_current);
            if(m_parent) m_parent->track_alloc(bytes);
        }
    }

    inline void track_free_internal(size_t bytes) {
        if(m_track_memory) {
            m_data.mem_current -= bytes;
            if(m_parent) m_parent->track_free(bytes);
        }
    }

public:
    inline static void track_alloc(size_t bytes) {
        if(s_current) s_current->track_alloc_internal(bytes);
    }

    inline static void track_free(size_t bytes) {
        if(s_current) s_current->track_free_internal(bytes);
    }

    inline Stat(const std::string& title) {
        m_parent = s_current;
        s_current = this;

        m_data.title = title;

        m_data.mem_off = m_parent ? m_parent->m_data.mem_current : 0;
        m_data.mem_current = 0;
        m_data.mem_peak = 0;

        m_data.time_end = 0;
        m_data.time_start = current_time_millis();
        m_track_memory = true;

        // TODO: artificially un-track self?
    }

    ~Stat() {
        // finish
        m_data.time_end = current_time_millis();
        m_track_memory = false;

        // add data to parent's data
        if(m_parent) {
            m_parent->m_data.children.push_back(m_data);
        }

        // pop parent
        s_current = m_parent;

        // TODO: artificially re-track self?
    }

    template<typename T>
    inline void log_stat(const std::string& key, const T& value) {
        m_track_memory = false; // TODO: ?
        m_data.log_stat(key, value);
        m_track_memory = true; // TODO: ?
    }

    inline Data& data() {
        m_data.time_end = current_time_millis();
        return m_data;
    }
};

}
