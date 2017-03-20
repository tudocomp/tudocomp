#pragma once

#include <cstring>
#include <ctime>
#include <string>

#include <tudocomp_stat/PhaseData.hpp>
#include <tudocomp_stat/StatPhaseDummy.hpp>
#include <tudocomp_stat/Json.hpp>

#ifndef STATS_DISABLED

#include <tudocomp_stat/PhaseData.hpp>

namespace tdc {

class StatPhase {
private:
    static StatPhase* s_current;

    inline static unsigned long current_time_millis() {
        timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);

        return t.tv_sec * 1000L + t.tv_nsec / 1000000L;
    }

    StatPhase* m_parent;
    PhaseData* m_data;

    bool  m_track_memory;

    inline void append_child(PhaseData* data) {
        if(m_data->first_child) {
            PhaseData* last = m_data->first_child;
            while(last->next_sibling) {
                last = last->next_sibling;
            }
            last->next_sibling = data;
        } else {
            m_data->first_child = data;
        }
    }

    inline void track_alloc_internal(size_t bytes) {
        if(m_track_memory) {
            m_data->mem_current += bytes;
            m_data->mem_peak = std::max(m_data->mem_peak, m_data->mem_current);
            if(m_parent) m_parent->track_alloc_internal(bytes);
        }
    }

    inline void track_free_internal(size_t bytes) {
        if(m_track_memory) {
            m_data->mem_current -= bytes;
            if(m_parent) m_parent->track_free_internal(bytes);
        }
    }

    inline void init(const char* title) {
        m_parent = s_current;

        if(m_parent) m_parent->m_track_memory = false;
        m_data = new PhaseData();
        if(m_parent) m_parent->m_track_memory = true;
        m_data->title(title);

        m_data->mem_off = m_parent ? m_parent->m_data->mem_current : 0;
        m_data->mem_current = 0;
        m_data->mem_peak = 0;

        m_data->time_end = 0;
        m_data->time_start = current_time_millis();

        s_current = this;
    }

    inline void finish() {
        m_data->time_end = current_time_millis();

        if(m_parent) {
            // add data to parent's data
            m_parent->append_child(m_data);
        } else {
            // if this was the root, delete data
            delete m_data;
            m_data = nullptr;
        }

        // pop parent
        s_current = m_parent;
    }

public:
    template<typename F>
    inline static auto wrap(const char* title, F func) ->
        typename std::result_of<F(StatPhase&)>::type {

        StatPhase phase(title);
        return func(phase);
    }

    template<typename F>
    inline static auto wrap(const char* title, F func) ->
        typename std::result_of<F()>::type {

        StatPhase phase(title);
        return func();
    }

    inline static void track_alloc(size_t bytes) {
        if(s_current) s_current->track_alloc_internal(bytes);
    }

    inline static void track_free(size_t bytes) {
        if(s_current) s_current->track_free_internal(bytes);
    }

    template<typename T>
    inline static void log(const char* key, const T& value) {
        if(s_current) s_current->log_stat(key, value);
    }

    inline StatPhase(const char* title) {
        m_track_memory = false;
        init(title);
        m_track_memory = true;
    }

    inline StatPhase(const std::string& str) : StatPhase(str.c_str()) {
    }

    inline ~StatPhase() {
        m_track_memory = false;
        finish();
    }

    inline void split(const char* new_title) {
        m_track_memory = false;

        finish();
        PhaseData* old_data = m_data;

        init(new_title);
        if(old_data) {
            m_data->mem_off = old_data->mem_off + old_data->mem_current;
        }

        m_track_memory = true;
    }

    inline void split(const std::string& new_title) {
        split(new_title.c_str());
    }

    template<typename T>
    inline void log_stat(const char* key, const T& value) {
        m_track_memory = false;
        m_data->log_stat(key, value);
        m_track_memory = true;
    }

    inline json::Object to_json() {
        m_data->time_end = current_time_millis();
        m_track_memory = false;
        json::Object obj = m_data->to_json();
        m_track_memory = true;
        return obj;
    }
};

}

#else

namespace tdc {

    using StatPhase = StatPhaseDummy;

}

#endif

