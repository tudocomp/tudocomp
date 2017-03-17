#pragma once

#include <cstring>
#include <ctime>

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

    public:
        static constexpr size_t STR_BUFFER_SIZE = 64;

    private:
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

    public: //TODO private

        char title[STR_BUFFER_SIZE];

        unsigned long time_start;
        unsigned long time_end;
        ssize_t mem_off;
        ssize_t mem_current;
        ssize_t mem_peak;

        keyval* first_stat;

        Data* first_child;
        Data* next_sibling;

        inline Data()
            : first_stat(nullptr),
              first_child(nullptr),
              next_sibling(nullptr) {
        }

        ~Data() {
            if(first_stat) delete first_stat;
            if(first_child) delete first_child;
            if(next_sibling) delete next_sibling;
        }

        template<typename T>
        inline void log_stat(const char* key, const T& value) {
            keyval* kv = new keyval();

            {
                json::TValue<T> t(value);
                std::stringstream ss;
                t.str(ss);

                strncpy(kv->key, key, STR_BUFFER_SIZE);
                strncpy(kv->val, ss.str().c_str(), STR_BUFFER_SIZE);
            }

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

    public:
        inline json::Object to_json() const {
            json::Object obj;
            obj.set("title",     title);
            obj.set("timeStart", time_start);
            obj.set("timeEnd",   time_end);
            obj.set("memOff",    mem_off);
            obj.set("memPeak",   mem_peak);
            obj.set("memFinal",  mem_current);

            json::Array stats;
            keyval* kv = first_stat;
            while(kv) {
                json::Object pair;
                pair.set("key", std::string(kv->key));
                pair.set("value", std::string(kv->val));
                stats.add(pair);
                kv = kv->next;
            }
            obj.set("stats", stats);

            json::Array sub;

            Data* child = first_child;
            while(child) {
                sub.add(child->to_json());
                child = child->next_sibling;
            }

            obj.set("sub", sub);

            return obj;
        }
    };

    Stat* m_parent;
    Data* m_data;

    bool  m_track_memory;

    inline void append_child(Data* data) {
        if(m_data->first_child) {
            Data* last = data->first_child;
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

public:
    inline static void track_alloc(size_t bytes) {
        if(s_current && s_current->m_track_memory) {
            printf("[%s] alloc: %zu\n", s_current->m_data->title, bytes);
            s_current->track_alloc_internal(bytes);
        }
    }

    inline static void track_free(size_t bytes) {
        if(s_current && s_current->m_track_memory) {
            printf("[%s]  free: %zu\n", s_current->m_data->title, bytes);
            s_current->track_free_internal(bytes);
        }
    }

    inline Stat(const char* title) {
        m_parent = s_current;

        if(m_parent) m_parent->m_track_memory = false;
        m_data = new Data();
        if(m_parent) m_parent->m_track_memory = true;
        strncpy(m_data->title, title, Data::STR_BUFFER_SIZE);

        s_current = this;
        m_track_memory = true;

        m_data->mem_off = m_parent ? m_parent->m_data->mem_current : 0;
        m_data->mem_current = 0;
        m_data->mem_peak = 0;

        m_data->time_end = 0;
        m_data->time_start = current_time_millis();
        m_track_memory = true;
    }

    ~Stat() {
        // finish
        m_track_memory = false;
        m_data->time_end = current_time_millis();

        if(m_parent) {
            // add data to parent's data
            m_parent->append_child(m_data);
        } else {
            // if this was the root, delete data
            delete m_data;
        }

        // pop parent
        s_current = m_parent;
    }

    template<typename T>
    inline void log_stat(const char* key, const T& value) {
        m_track_memory = false;
        m_data->log_stat(key, value);
        m_track_memory = true;
    }

    inline void to_json(std::ostream& out) {
        m_data->time_end = current_time_millis();
        m_track_memory = false;
        m_data->to_json().str(out);
        m_track_memory = true;
    }
};

}
