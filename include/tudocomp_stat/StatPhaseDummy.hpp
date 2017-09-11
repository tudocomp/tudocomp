#pragma once

#include <cstring>
#include <ctime>

#include <tudocomp_stat/Json.hpp>

/// \cond INTERNAL

namespace tdc {

// same public interface as StatPhase, but doesn't do anything
// used for STATS_DISABLED
class StatPhaseDummy {
public:
    inline StatPhaseDummy() {
    }

    template<typename F>
    inline static auto wrap(const char* title, F func) ->
        typename std::result_of<F(StatPhaseDummy&)>::type {

        StatPhaseDummy phase;
        return func(phase);
    }

    template<typename F>
    inline static auto wrap(const char* title, F func) ->
        typename std::result_of<F()>::type {

        return func();
    }

    inline static void track_alloc(size_t bytes) {
    }

    inline static void track_free(size_t bytes) {
    }

    inline static void pause_tracking() {
    }

    inline static void resume_tracking() {
    }

    template<typename T>
    inline static void log(const char* key, const T& value) {
    }

    inline StatPhaseDummy(const char* title) {
    }

    inline StatPhaseDummy(const std::string& title) {
    }

    inline ~StatPhaseDummy() {
    }

    inline void split(const char* new_title) {
    }

    inline void split(const std::string& new_title) {
    }

    template<typename T>
    inline void log_stat(const char* key, const T& value) {
    }

    inline json::Object to_json() {
        return json::Object();
    }
};

}

/// \endcond

