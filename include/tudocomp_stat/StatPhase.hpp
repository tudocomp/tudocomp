#pragma once

#include <cstring>
#include <ctime>
#include <string>

#include <tudocomp_stat/Json.hpp>

#ifndef STATS_DISABLED

#include <tudocomp_stat/PhaseData.hpp>

#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

namespace tdc {

// Use clock_gettime in linux, clock_get_time in OS X.
inline void get_monotonic_time(struct timespec *ts){
#ifdef __MACH__
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_MONOTONIC, ts);
#endif
}

/// \brief Provides access to runtime and memory measurement in statistics
///        phases.
///
/// Phases are used to track runtime and memory allocations over the course
/// of the application. The measured data can be printed as a JSON string for
/// use in the tudocomp charter for visualization or third party applications.
class StatPhase {
private:
    static StatPhase* s_current;

    inline static unsigned long current_time_millis() {
        timespec t;
        get_monotonic_time(&t);

        return t.tv_sec * 1000L + t.tv_nsec / 1000000L;
    }

    StatPhase* m_parent = nullptr;
    PhaseData* m_data = nullptr;

    bool m_track_memory = false;
    bool m_disabled = false;

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

    inline void pause() {
        m_track_memory = false;
    }

    inline void resume() {
        m_track_memory = true;
    }

    inline void init(const char* title) {
        m_parent = s_current;

        if(m_parent) m_parent->pause();
        m_data = new PhaseData();
        if(m_parent) m_parent->resume();
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
    /// \brief Executes a lambda as a single statistics phase.
    ///
    /// The new phase is started as a sub phase of the current phase and will
    /// immediately become the current phase.
    ///
    /// In case the given lambda accepts a \c StatPhase reference parameter,
    /// the phase object will be passed to it for use during execution.
    ///
    /// \param title the phase title
    /// \param func  the lambda to execute
    /// \return the return value of the lambda
    template<typename F>
    inline static auto wrap(const char* title, F func) ->
        typename std::result_of<F(StatPhase&)>::type {

        StatPhase phase(title);
        return func(phase);
    }

    /// \brief Executes a lambda as a single statistics phase.
    ///
    /// The new phase is started as a sub phase of the current phase and will
    /// immediately become the current phase.
    ///
    /// In case the given lambda accepts a \c StatPhase reference parameter,
    /// the phase object will be passed to it for use during execution.
    ///
    /// \param title the phase title
    /// \param func  the lambda to execute
    /// \return the return value of the lambda
    template<typename F>
    inline static auto wrap(const char* title, F func) ->
        typename std::result_of<F()>::type {

        StatPhase phase(title);
        return func();
    }

    /// \brief Tracks a memory allocation of the given size for the current
    ///        phase.
    ///
    /// Use this only if memory is allocated with methods that do not result
    /// in calls of \c malloc but should still be tracked (e.g., when using
    /// direct kernel allocations like memory mappings).
    ///
    /// \param bytes the amount of allocated bytes to track for the current
    ///              phase
    inline static void track_alloc(size_t bytes) {
        if(s_current) s_current->track_alloc_internal(bytes);
    }

    /// \brief Tracks a memory deallocation of the given size for the current
    ///        phase.
    ///
    /// Use this only if memory is allocated with methods that do not result
    /// in calls of \c malloc but should still be tracked (e.g., when using
    /// direct kernel allocations like memory mappings).
    ///
    /// \param bytes the amount of freed bytes to track for the current phase
    inline static void track_free(size_t bytes) {
        if(s_current) s_current->track_free_internal(bytes);
    }

    /// \brief Pauses the tracking of memory allocations in the current phase.
    ///
    /// Memory tracking is paused until \ref pause_tracking is called or the
    /// phase object is destroyed.
    inline static void pause_tracking() {
        if(s_current) s_current->pause();
    }

    /// \brief Resumes the tracking of memory allocations in the current phase.
    ///
    /// This only has an effect if tracking has previously been paused using
    /// \ref pause_tracking.
    inline static void resume_tracking() {
        if(s_current) s_current->resume();
    }

    /// \brief Logs a user statistic for the current phase.
    ///
    /// User statistics will be stored in a special data block for a phase
    /// and is included in the JSON output.
    ///
    /// \param key the statistic key or name
    /// \param value the value to log (will be converted to a string)
    template<typename T>
    inline static void log(const char* key, const T& value) {
        if(s_current) s_current->log_stat(key, value);
    }

    /// \brief Creates a inert statistics phase without any effect.
    inline StatPhase() {
        m_disabled = true;
    }

    /// \brief Creates a new statistics phase.
    ///
    /// The new phase is started as a sub phase of the current phase and will
    /// immediately become the current phase.
    ///
    /// \param title the phase title
    inline StatPhase(const char* title) {
        pause();
        init(title);
        resume();
    }

    /// \brief Creates a new statistics phase.
    ///
    /// The new phase is started as a sub phase of the current phase and will
    /// immediately become the current phase.
    ///
    /// \param str the phase title
    inline StatPhase(const std::string& str) : StatPhase(str.c_str()) {
    }

    /// \brief Destroys and ends the phase.
    ///
    /// The phase's parent phase, if any, will become the current phase.
    inline ~StatPhase() {
        if (!m_disabled) {
            pause();
            finish();
        }
    }

    /// \brief Starts a new phase as a sibling, reusing the same object.
    ///
    /// This function behaves exactly as if the current phase was ended and
    /// a new phases was started immediately after.
    ///
    /// \param new_title the new phase title
    inline void split(const char* new_title) {
        if (!m_disabled) {
            pause();

            finish();
            PhaseData* old_data = m_data;

            init(new_title);
            if(old_data) {
                m_data->mem_off = old_data->mem_off + old_data->mem_current;
            }

            resume();
        }
    }

    /// \brief Starts a new phase as a sibling, reusing the same object.
    ///
    /// This function behaves exactly as if the current phase was ended and
    /// a new phases was started immediately after.
    ///
    /// \param new_title the new phase title
    inline void split(const std::string& new_title) {
        split(new_title.c_str());
    }

    /// \brief Logs a user statistic for this phase.
    ///
    /// User statistics will be stored in a special data block for a phase
    /// and is included in the JSON output.
    ///
    /// \param key the statistic key or name
    /// \param value the value to log (will be converted to a string)
    template<typename T>
    inline void log_stat(const char* key, const T& value) {
        if (!m_disabled) {
            pause();
            m_data->log_stat(key, value);
            resume();
        }
    }

    /// \brief Constructs the JSON representation of the measured data.
    ///
    /// It contains the subtree of phases beneath this phase.
    ///
    /// \return the \ref json::Object containing the JSON representation
    inline json::Object to_json() {
        if (!m_disabled) {
            m_data->time_end = current_time_millis();
            pause();
            json::Object obj = m_data->to_json();
            resume();
            return obj;
        } else {
            return json::Object();
        }
    }
};

}

#else

#include <tudocomp_stat/StatPhaseDummy.hpp>

namespace tdc {

    using StatPhase = StatPhaseDummy;

}

#endif

