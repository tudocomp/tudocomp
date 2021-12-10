#pragma once

#include <cstring>
#include <ctime>
#include <string>
#include <memory>

#include <tudocomp_stat/json.hpp>

#ifndef STATS_DISABLED

#include <tudocomp_stat/StatPhaseExtension.hpp>

#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

namespace tdc {
    using json = nlohmann::json;

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
    //////////////////////////////////////////
    // Memory tracking
    //////////////////////////////////////////

    static uint16_t s_suppress_memory_tracking_state;
    static uint16_t s_suppress_tracking_user_state;

    static bool s_init;
    static void force_malloc_override_link();

    struct suppress_memory_tracking {
        inline suppress_memory_tracking(suppress_memory_tracking const&) = delete;
        inline suppress_memory_tracking() {
            s_suppress_memory_tracking_state++;
        }
        inline ~suppress_memory_tracking() {
            s_suppress_memory_tracking_state--;
        }
        inline static bool is_paused() {
            return s_suppress_memory_tracking_state != 0;
        }
    };

    struct suppress_tracking_user {
        inline static void inc() {
            if(s_suppress_tracking_user_state++ == 0) {
                if(s_current) s_current->on_pause_tracking();
            }
        }
        inline static void dec() {
            if(s_suppress_tracking_user_state == 1) {
                if(s_current) s_current->on_resume_tracking();
            }
            --s_suppress_tracking_user_state;
        }

        inline suppress_tracking_user() {
            inc();
        }
        inline suppress_tracking_user(suppress_memory_tracking const&) = delete;
        inline suppress_tracking_user(suppress_tracking_user&&) {
        }
        inline ~suppress_tracking_user() {
            dec();
        }
        inline static bool is_paused() {
            return s_suppress_tracking_user_state != 0;
        }
    };

    inline bool currently_tracking_memory() {
        return !suppress_memory_tracking::is_paused()
            && !suppress_tracking_user::is_paused();
    }

    inline void track_alloc_internal(size_t bytes) {
        if(currently_tracking_memory()) {
            m_mem.current += bytes;
            m_mem.peak = std::max(m_mem.peak, m_mem.current);
            if(m_parent) m_parent->track_alloc_internal(bytes);
        }
    }

    inline void track_free_internal(size_t bytes) {
        if(currently_tracking_memory()) {
            m_mem.current -= bytes;
            if(m_parent) m_parent->track_free_internal(bytes);
        }
    }

    //////////////////////////////////////////
    // Extensions
    //////////////////////////////////////////

    using ext_ptr_t = std::unique_ptr<StatPhaseExtension>;
    static std::vector<std::function<ext_ptr_t()>> m_extension_registry;

public:
    template<typename E>
    static inline void register_extension() {
        if(s_current != nullptr) {
            throw std::runtime_error(
                "Extensions must be registered outside of any "
                "stat measurements!");
        } else {
            m_extension_registry.emplace_back([](){
                return std::make_unique<E>();
            });
        }
    }

private:
    std::unique_ptr<std::vector<ext_ptr_t>> m_extensions;

    //////////////////////////////////////////
    // Other StatPhase state
    //////////////////////////////////////////

    static StatPhase* s_current;
    StatPhase* m_parent = nullptr;

    double m_pause_time;

    struct {
        double start, end, paused;
    } m_time;

    struct {
        ssize_t off, current, peak;
    } m_mem;

    std::string m_title;
    std::unique_ptr<json> m_sub;
    std::unique_ptr<json> m_stats;

    bool m_disabled = false;

    inline static double current_time_millis() {
        timespec t;
        get_monotonic_time(&t);

        return double(t.tv_sec * 1000L) + double(t.tv_nsec) / double(1000000L);
    }

    inline void init(std::string&& title) {
        suppress_memory_tracking guard;

        if(!s_init) {
            force_malloc_override_link();
            s_init = true;
        }

        m_parent = s_current;

        m_title = std::move(title);

        // managed allocation of complex members
        m_extensions = std::make_unique<std::vector<ext_ptr_t>>();
        m_sub = std::make_unique<json>(json::array());
        m_stats = std::make_unique<json>();

        // initialize extensions
        for(auto ctor : m_extension_registry) {
            m_extensions->emplace_back(ctor());
        }

        // initialize basic data as the very last thing
        m_mem.off = m_parent ? m_parent->m_mem.current : 0;
        m_mem.current = 0;
        m_mem.peak = 0;

        m_time.end = 0;
        m_time.start = current_time_millis();
        m_time.paused = 0;

        // set as current
        s_current = this;
    }

    /// Finish the current Phase
    inline void finish() {
        suppress_memory_tracking guard;

        m_time.end = current_time_millis();

        // let extensions write data
        for(auto& ext : *m_extensions) {
            ext->write(*m_stats);
        }

        if(m_parent) {
            // propagate extensions to parent
            for(size_t i = 0; i < m_extensions->size(); i++) {
                (*(m_parent->m_extensions))[i]->propagate(*(*m_extensions)[i]);
            }

            // add data to parent's data
            m_parent->m_time.paused += m_time.paused;
            m_parent->m_sub->push_back(to_json());
        }

        // managed release of complex members
        m_extensions.release();
        m_sub.release();
        m_stats.release();

        // pop parent
        s_current = m_parent;
    }

    inline void on_pause_tracking() {
        m_pause_time = current_time_millis();

        // notify extensions
        for(auto& ext : *m_extensions) {
            ext->pause();
        }
    }

    inline void on_resume_tracking() {
        // notify extensions
        for(auto& ext : *m_extensions) {
            ext->resume();
        }

        m_time.paused += current_time_millis() - m_pause_time;
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
    inline static auto wrap(std::string&& title, F func) ->
        typename std::result_of<F(StatPhase&)>::type {

        StatPhase phase(std::move(title));
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
    inline static auto wrap(std::string&& title, F func) ->
        typename std::result_of<F()>::type {

        StatPhase phase(std::move(title));
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
    [[deprecated("Use suppress_tracking")]]
    inline static void pause_tracking() {
        suppress_tracking_user::inc();
    }

    /// \brief Resumes the tracking of memory allocations in the current phase.
    ///
    /// This only has an effect if tracking has previously been paused using
    /// \ref pause_tracking.
    [[deprecated("Use suppress_tracking")]]
    inline static void resume_tracking() {
        suppress_tracking_user::dec();
    }

    /// \brief Creates a guard that suppresses tracking as long as it exists.
    ///
    /// This should be used during more complex logging activity in order
    /// for it to not count against memory measures.
    inline static auto suppress_tracking() {
        return suppress_tracking_user();
    }

    /// \brief Suppress tracking while exeucting the lambda.
    ///
    /// This should be used during more complex logging activity in order
    /// for it to not count against memory measures.
    ///
    /// \param func  the lambda to execute
    /// \return the return value of the lambda
    template<typename F>
    inline static auto suppress_tracking(F func) ->
        typename std::result_of<F()>::type {

        suppress_tracking_user guard;
        return func();
    }

    /// \brief Logs a user statistic for the current phase.
    ///
    /// User statistics will be stored in a special data block for a phase
    /// and is included in the JSON output.
    ///
    /// \param key the statistic key or name
    /// \param value the value to log (will be converted to a string)
    template<typename T>
    inline static void log(std::string&& key, const T& value) {
        if(s_current) s_current->log_stat(std::move(key), value);
    }

    /// \brief Creates an inert statistics phase without any effect.
    inline StatPhase() {
        m_disabled = true;
    }

    /// \brief Creates a new statistics phase.
    ///
    /// The new phase is started as a sub phase of the current phase and will
    /// immediately become the current phase.
    ///
    /// \param title the phase title
    inline StatPhase(std::string&& title) {
        init(std::move(title));
    }

    /// \brief Destroys and ends the phase.
    ///
    /// The phase's parent phase, if any, will become the current phase.
    inline ~StatPhase() {
        if (!m_disabled) {
            finish();
        }
    }

    /// \brief Starts a new phase as a sibling, reusing the same object.
    ///
    /// This function behaves exactly as if the current phase was ended and
    /// a new phases was started immediately after.
    ///
    /// \param new_title the new phase title
    inline void split(std::string&& new_title) {
        if (!m_disabled) {
            const ssize_t offs = m_mem.off + m_mem.current;
            finish();
            init(std::move(new_title));
            m_mem.off = offs;
        }
    }

    /// \brief Logs a user statistic for this phase.
    ///
    /// User statistics will be stored in a special data block for a phase
    /// and is included in the JSON output.
    ///
    /// \param key the statistic key or name
    /// \param value the value to log (will be converted to a string)
    template<typename T>
    inline void log_stat(std::string&& key, const T& value) {
        if (!m_disabled) {
            suppress_memory_tracking guard;
            (*m_stats)[std::move(key)] = value;
        }
    }

    inline const std::string& title() const {
        return m_title;
    }

    /// \brief Constructs the JSON representation of the measured data.
    ///
    /// It contains the subtree of phases beneath this phase.
    ///
    /// \return the \ref json::Object containing the JSON representation
    inline json to_json() {
        suppress_memory_tracking guard;
        if (!m_disabled) {
            m_time.end = current_time_millis();

            // let extensions write data
            for(auto& ext : *m_extensions) {
                ext->write(*m_stats);
            }

            json obj;
            obj["title"] = m_title;
            obj["timeStart"] = m_time.start;
            obj["timeEnd"] = m_time.end;
            obj["timePaused"] = m_time.paused;

            const double dt = m_time.end - m_time.start;
            obj["timeDelta"] = dt;
            obj["timeRun"] = dt - m_time.paused;
            obj["memOff"] = m_mem.off;
            obj["memPeak"] = m_mem.peak;
            obj["memFinal"] = m_mem.current;
            obj["sub"] = *m_sub;

            /*
            obj["stats"] = m_stats; // TODO: opt into new format?
            */
            auto stats_array = json::array();
            for(auto it = m_stats->begin(); it != m_stats->end(); it++) {
                stats_array.push_back(json({
                    {"key", it.key()},
                    {"value", *it}
                }));
            }
            obj["stats"] = stats_array;

            return obj;
        } else {
            return json();
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
