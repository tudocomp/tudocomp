#include <tudocomp_stat/malloc.hpp>
#include <tudocomp_stat/StatPhase.hpp>

#ifndef STATS_DISABLED

using tdc::StatPhase;

std::vector<std::function<StatPhase::ext_ptr_t()>>
    StatPhase::m_extension_registry;

StatPhase* StatPhase::s_current = nullptr;
uint16_t StatPhase::s_suppress_memory_tracking_state = 0;
uint16_t StatPhase::s_suppress_tracking_user_state = 0;

bool StatPhase::s_init = false;

#ifndef MALLOC_DISABLED

void StatPhase::force_malloc_override_link() {
    // Make sure the malloc override is actually linked into the using program.
    //
    // If malloc is never called explicitly, the override won't be linked in
    // a static linking scenario and in that case, memory tracking doesn't work.
    // Thus, here's an explicit call to make sure the link happens.
    //
    // At runtime, this is executed only once when the first StatPhase is
    // initialized.
    void* p = malloc(sizeof(char));
    {
        // make sure all of this isn't "optimized away"
        volatile char* c = (char*)p;
        *c = 0;
    }
    free(p);
}

void malloc_callback::on_alloc(size_t bytes) {
    StatPhase::track_alloc(bytes);
}

void malloc_callback::on_free(size_t bytes) {
    StatPhase::track_free(bytes);
}

#else

void StatPhase::force_malloc_override_link() {
}

#endif

#endif
