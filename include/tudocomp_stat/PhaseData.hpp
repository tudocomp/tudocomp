#pragma once

#include <cstring>
#include <string>
#include <tudocomp_stat/Json.hpp>

/// \cond INTERNAL

namespace tdc {

class PhaseData {
private:
    static constexpr size_t STR_BUFFER_SIZE = 64;

    template<typename T>
    static inline std::string to_string(const T& value) {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }

    struct keyval {
        keyval* next;
        char key[STR_BUFFER_SIZE];
        char val[STR_BUFFER_SIZE];

        inline keyval() : next(nullptr) {
        }

        ~keyval() {
            if(next) delete next;
        }
    };

    char m_title[STR_BUFFER_SIZE];

public:
    static void (*s_stxxl_start)(PhaseData*);
    static void (*s_stxxl_read_stats)(PhaseData*);
    static void (*s_stxxl_stop)(PhaseData*);

    unsigned long time_start;
    unsigned long time_end;
    ssize_t mem_off;
    ssize_t mem_current;
    ssize_t mem_peak;
    
    keyval* first_stat;
    
    void (*stxxl_start)(PhaseData*);
    void (*stxxl_read_stats)(PhaseData*);
    void (*stxxl_stop)(PhaseData*);
    void* stxxl_stats_begin;
    keyval* first_stat_stxxl;
    
    PhaseData* first_child;
    PhaseData* next_sibling;

    inline PhaseData()
        : first_stat(nullptr),
          stxxl_start(s_stxxl_start),
          stxxl_read_stats(s_stxxl_read_stats),
          stxxl_stop(s_stxxl_stop),
          stxxl_stats_begin(nullptr),          
          first_stat_stxxl(nullptr),        
          first_child(nullptr),
          next_sibling(nullptr) {
    }

    inline ~PhaseData() {
        if(first_stat) delete first_stat;
        if(first_stat_stxxl) delete first_stat_stxxl;
        if(first_child) delete first_child;
        if(next_sibling) delete next_sibling;        
        if(stxxl_stop) stxxl_stop(this);
    }

    inline const char* title() const {
        return m_title;
    }

    inline void title(const char* title) {
        strncpy(m_title, title, STR_BUFFER_SIZE);
    }

    template<typename T>
    inline void log_stat(const char* key, const T& value) {
        keyval* kv = new keyval();

        strncpy(kv->key, key, STR_BUFFER_SIZE);
        strncpy(kv->val, to_string(value).c_str(), STR_BUFFER_SIZE);

        if(first_stat) {
            keyval* last = first_stat;
            while(last->next) {
                last = last->next;
            }
            last->next = kv;
        } else {
            first_stat = kv;
        }
    }
    
    template<typename T>
    inline void log_stat_stxxl(const char* key, const T& value) {
        keyval* kv = new keyval();

        strncpy(kv->key, key, STR_BUFFER_SIZE);
        strncpy(kv->val, std::to_string(value).c_str(), STR_BUFFER_SIZE);

        if(first_stat_stxxl) {
            keyval* last = first_stat_stxxl;
            while(last->next) {
                last = last->next;
            }
            last->next = kv;
        } else {
            first_stat_stxxl = kv;
        }
    }

    inline json::Object to_json() const {
        json::Object obj;
        obj.set("title",     m_title);
        obj.set("timeStart", time_start);
        obj.set("timeEnd",   time_end);
        obj.set("memOff",    mem_off);
        obj.set("memPeak",   mem_peak);
        obj.set("memFinal",  mem_current);

        json::Array stats;
        keyval* kv = first_stat_stxxl;
        while(kv) {
            json::Object pair;
            pair.set("key", std::string(kv->key));
            pair.set("value", std::string(kv->val));
            stats.add(pair);
            kv = kv->next;
        }
        kv = first_stat;
        while(kv) {
            json::Object pair;
            pair.set("key", std::string(kv->key));
            pair.set("value", std::string(kv->val));
            stats.add(pair);
            kv = kv->next;
        }
        obj.set("stats", stats);

        json::Array sub;

        PhaseData* child = first_child;
        while(child) {
            sub.add(child->to_json());
            child = child->next_sibling;
        }

        obj.set("sub", sub);

        return obj;
    }
};

}

/// \endcond

