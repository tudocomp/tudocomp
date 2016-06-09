#include <tudostat/Stat.hpp>
#include <malloc_count/malloc_count.hpp>

using namespace tudostat;

Stat::Phase end_phase_if() {
    auto mp = malloc_count::end_phase();

    Stat::Phase p;
    p.start_time = mp.time_start;
    p.duration = mp.time_delta;
    p.mem_off = mp.mem_off;
    p.mem_peak = mp.mem_peak;
    return p;
}

void (*Stat::begin_phase)(void) = malloc_count::begin_phase;
Stat::Phase (*Stat::end_phase)(void) = end_phase_if;
void (*Stat::pause_phase)(void) = malloc_count::pause_phase;
void (*Stat::resume_phase)(void) = malloc_count::resume_phase;

