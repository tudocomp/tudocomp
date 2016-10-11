#ifndef _INCLUDED_STAT_HPP_
#define _INCLUDED_STAT_HPP_

#include <malloc_count/malloc_count.hpp>
#include <list>
#include <map>
#include <sstream>

/// \brief Contains the runtime statistics interface \ref Stat.
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

    std::map<std::string, long> m_stats_int;
    std::map<std::string, double> m_stats_real;

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

    #define JSON_INDENT 4

    inline void to_json(std::ostream& stream, unsigned int indent = 0) const {
        std::string indent_str(indent, ' ');
        std::string indent2_str(indent + JSON_INDENT, ' ');

        stream << indent_str << "{\n"
               << indent2_str << "\"title\": \"" << m_title << "\",\n"
               << indent2_str << "\"timeStart\": " << m_phase.start_time << ",\n"
               << indent2_str << "\"timeEnd\": " << m_phase.end_time << ",\n"
               << indent2_str << "\"memOff\": " << m_phase.mem_off << ",\n"
               << indent2_str << "\"memPeak\": " << m_phase.mem_peak << ",\n"
               << indent2_str << "\"memFinal\": " << m_phase.mem_final << ",\n";

        stream << indent2_str << "\"stats\": [";
        size_t stats_total = m_stats_int.size() + m_stats_real.size();
        if(stats_total == 0) {
            stream << "],\n";
        } else {
            stream << "\n";

            std::string indent3_str(indent + 2 * JSON_INDENT, ' ');
            size_t i = 0;

            #define STAT_LIST_JSON(lst)                                          \
            for(auto it = lst.begin(); it != lst.end(); it++) {                  \
                stream << indent3_str << "{ \"key\": \"" << it->first << "\", "; \
                stream << "\"value\": " << it->second << " }";                   \
                if(++i < stats_total) stream << ",";                             \
                stream << "\n";                                                  \
            }

            STAT_LIST_JSON(m_stats_int);
            STAT_LIST_JSON(m_stats_real);

            stream << indent2_str << "],\n";
        }

        stream << indent2_str << "\"sub\": [";
        if(m_sub.empty()) {
            stream << "]\n";
        } else {
            stream << indent2_str << "\n";
            for(auto it = m_sub.begin(); it != m_sub.end(); it++) {
                it->to_json(stream, indent + 2 * JSON_INDENT);
                if(std::next(it) != m_sub.end()) stream << ",";
                stream << "\n";
            }
            stream << indent2_str << "]\n";
        }

        stream << indent_str << "}";
    }

    inline std::string to_json() const {
        std::stringstream ss;
        to_json(ss);
        return ss.str();
    }
};

}

#endif

